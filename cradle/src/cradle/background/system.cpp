#include <cradle/background/system.hpp>

#include <boost/algorithm/string.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/io/web_io.hpp>

namespace cradle {

void set_disk_cache(background_execution_system& system,
    alia__shared_ptr<disk_cache> const& disk_cache)
{
    system.impl_->disk_cache = disk_cache;
}

alia__shared_ptr<disk_cache> const&
get_disk_cache(background_execution_system& system)
{
    return system.impl_->disk_cache;
}

void record_failure(background_job_queue& queue, background_job_ptr& job,
    string msg, bool is_transient)
{
    job->state = background_job_state::FAILED;
    boost::lock_guard<boost::mutex> lock(queue.mutex);
    inc_version(queue.version);
    background_job_failure failure;
    failure.is_transient = is_transient;
    failure.message = msg; // != '\0' ? msg : "unknown error";
    failure.job = job;
    queue.failed_jobs.push_back(failure);
}

void background_job_execution_loop::operator()()
{
    while (1)
    {
        auto& queue = *queue_;

        // Wait until the queue has a job in it, and then grab the job.
        background_job_ptr job;
        size_t wake_up_counter;
        {
            boost::unique_lock<boost::mutex> lock(queue.mutex);
            inc_version(queue.version);
            ++queue.n_idle_threads;
            // If this queue is allocating threads on demand and there are
            // already a lot of idle threads, just end this one.

            while (queue.jobs.empty())
                queue.cv.wait(lock);
            job = queue.jobs.top();
            inc_version(queue.version);
            queue.jobs.pop();
            if (!job->hidden)
                --queue.reported_size;
            --queue.n_idle_threads;
            wake_up_counter = queue.wake_up_counter;

            // If it's already been instructed to cancel, cancel it.
            if (job->cancel)
            {
                job->state = background_job_state::CANCELED;
                queue.job_info.erase(&*job);
                continue;
            }
        }

        while (1)
        {
            // Instruct the job to gather its inputs.
            job->job->gather_inputs();

            if (job->job->inputs_ready())
                goto job_inputs_ready;

            {
                boost::lock_guard<boost::mutex> lock(queue.mutex);
                // If the wake_up_counter has changed, data became avaiable
                // while this job was checking its inputs, so try again.
                if (queue.wake_up_counter != wake_up_counter)
                {
                    wake_up_counter = queue.wake_up_counter;
                    continue;
                }
                {
                    inc_version(queue.version);
                    queue.waiting_jobs.push(job);
                    if (!job->hidden)
                        ++queue.reported_size;
                    break;
                }
            }
        }
        continue;

      job_inputs_ready:

        {
            boost::lock_guard<boost::mutex> lock(data_proxy_->mutex);
            data_proxy_->active_job = job;
        }

        try
        {
            job->state = background_job_state::RUNNING;
            background_job_check_in check_in(job);
            background_job_progress_reporter reporter(job);
            job->job->execute(check_in, reporter);
            job->state = background_job_state::FINISHED;
        }
        catch (background_job_canceled&)
        {
        }
        catch (cradle::exception& e)
        {
            string msg = "(bjc) " + job->job->get_info().description + string("\n") + string(e.what());
            record_failure(queue, job, msg, e.is_transient());
        }
        catch (std::bad_alloc&)
        {
            string msg = "(bj) " + job->job->get_info().description + string("\n") + string(" out of memory");
            record_failure(queue, job, msg, true);
        }
        catch (std::exception& e)
        {
            string msg = "(bjs) " + job->job->get_info().description + string("\n") + string(e.what());
            record_failure(queue, job, msg, false);
        }
        catch (...)
        {
            string msg = "(bj) " + job->job->get_info().description;
            record_failure(queue, job, msg, false);
        }

        {
            boost::unique_lock<boost::mutex> lock(queue.mutex);
            queue.job_info.erase(&*job);
            inc_version(queue.version);
        }

        {
            boost::lock_guard<boost::mutex> lock(data_proxy_->mutex);
            data_proxy_->active_job.reset();
        }
    }
}

void web_request_processing_loop::operator()()
{
    while (1)
    {
        auto& queue = *queue_;

        // Wait until the queue has a job in it, and then grab the job.
        background_job_ptr job;
        {
            boost::unique_lock<boost::mutex> lock(queue.mutex);
            inc_version(queue.version);
            ++queue.n_idle_threads;
            while (queue.jobs.empty())
                queue.cv.wait(lock);
            job = queue.jobs.top();
            inc_version(queue.version);
            queue.jobs.pop();
            if (!job->hidden)
                --queue.reported_size;
            --queue.n_idle_threads;

            // If it's already been instructed to cancel, cancel it.
            if (job->cancel)
            {
                job->state = background_job_state::CANCELED;
                queue.job_info.erase(&*job);
                continue;
            }
        }

        // If its inputs aren't ready, put it in the waiting queue.
        if (!job->job->inputs_ready())
        {
            boost::lock_guard<boost::mutex> lock(queue.mutex);
            inc_version(queue.version);
            queue.waiting_jobs.push(job);
            if (!job->hidden)
                ++queue.reported_size;
        }
        // Otherwise, execute it.
        else
        {
            {
                boost::lock_guard<boost::mutex> lock(data_proxy_->mutex);
                data_proxy_->active_job = job;
            }

            assert(dynamic_cast<background_web_job*>(job->job));
            auto web_job = static_cast<background_web_job*>(job->job);
            assert(web_job->system);

            try
            {
                job->state = background_job_state::RUNNING;
                background_job_check_in check_in(job);
                background_job_progress_reporter reporter(job);
                web_job->connection = connection_.get();
                job->job->execute(check_in, reporter);
                job->state = background_job_state::FINISHED;

                // The job is done, so clear out its reference to the
                // background execution system.
                // Otherwise we'll end up with circular references.
                web_job->system.reset();
            }
            catch (background_job_canceled&)
            {
            }
            catch (web_request_failure& failure)
            {
                switch (failure.response_code())
                {
                 case 401:
                    invalidate_authentication_data(*web_job->system,
                        background_authentication_state::NO_CREDENTIALS);
                    break;
                 case 483:
                    invalidate_authentication_data(*web_job->system,
                        background_authentication_state::SESSION_EXPIRED);
                    break;
                 case 484:
                    invalidate_authentication_data(*web_job->system,
                        background_authentication_state::SESSION_TIMED_OUT);
                    break;
                 case 481: // missing cookies
                 case 482: // invalid cookies
                    // These should never happen unless there's a bug.
                    assert(0);
                    // Fall through just in case.
                 default:
                    // Record if the failure was transient or a 5XX error code
                    // so that these calculations can be retried.
                    record_failure(queue, job, failure.what(),
                        failure.is_transient() ||
                        (failure.response_code() / 100) == 5);
                    break;
                }
            }
            catch (cradle::exception& e)
            {
                string msg = string(e.what()) + string("\n\ndebug details:\n") + "(wrc) " + job->job->get_info().description;
                record_failure(queue, job,  msg, e.is_transient());
            }
            catch (std::bad_alloc&)
            {
                string msg = "(wrc) " + job->job->get_info().description + string("\n out of memory");
                record_failure(queue, job, msg, true);
            }
            catch (std::exception& e)
            {
                string msg = string(e.what()) + string("\n\ndebug details:\n") + "(wrs) " + job->job->get_info().description;
                record_failure(queue, job, msg, false);
            }
            catch (...)
            {
                string msg = "(wrc) " + job->job->get_info().description;
                record_failure(queue, job, msg, false);
            }

            {
                boost::unique_lock<boost::mutex> lock(queue.mutex);
                queue.job_info.erase(&*job);
                inc_version(queue.version);
            }

            {
                boost::lock_guard<boost::mutex> lock(data_proxy_->mutex);
                data_proxy_->active_job.reset();
            }
        }
    }
}

template<class ExecutionLoop>
void initialize_pool(
    background_execution_pool& pool, unsigned initial_thread_count)
{
    pool.queue.reset(new background_job_queue);
    for (unsigned i = 0; i != initial_thread_count; ++i)
        add_background_thread<ExecutionLoop>(pool);
}

void static
initialize_system(background_execution_system_impl& system)
{
    // Only enable full concurrency in release mode.
    // I've had issues with running inside the debugger with too many threads,
    // and it's just easier to see what's going on with less concurrency.
    // (The app even feels faster in debug mode with fewer threads.)
  #ifdef _DEBUG
    bool const full_concurrency = false;
  #else
    bool const full_concurrency = true;
  #endif
    // Initialize all the queues.
    initialize_pool<background_job_execution_loop>(
        system.pools[int(background_job_queue_type::CALCULATION)],
        full_concurrency ? boost::thread::hardware_concurrency() : 1);
    initialize_pool<web_request_processing_loop>(
        system.pools[int(background_job_queue_type::WEB_READ)],
        full_concurrency ? 16 : 1);
    initialize_pool<web_request_processing_loop>(
        system.pools[int(background_job_queue_type::WEB_WRITE)], 1);
    initialize_pool<web_request_processing_loop>(
        system.pools[int(background_job_queue_type::NOTIFICATION_WATCH)], 1);
    initialize_pool<web_request_processing_loop>(
        system.pools[int(background_job_queue_type::REMOTE_CALCULATION)], 1);
    initialize_pool<background_job_execution_loop>(
        system.pools[int(background_job_queue_type::DISK)],
        full_concurrency ? 2 : 1);
    // Invalidate the session data.
    system.authentication.status =
        background_authentication_status(
            background_authentication_state::NO_CREDENTIALS);
    system.context.status =
        background_context_request_status(
            background_context_request_state::NO_REQUEST);
}

void static
shut_down_pool(background_execution_pool& pool)
{
    for (std::vector<alia__shared_ptr<background_execution_thread> >::iterator
        i = pool.threads.begin(); i != pool.threads.end(); ++i)
    {
        (*i)->thread.interrupt();
    }
}

bool static
is_pool_idle(background_execution_pool& pool)
{
    background_job_queue& queue = *pool.queue;
    boost::mutex::scoped_lock lock(queue.mutex);
    return
        queue.n_idle_threads == pool.threads.size() &&
        queue.jobs.empty() &&
        queue.waiting_jobs.empty();
}

void static
shut_down_system(background_execution_system_impl& system)
{
    // Shut down all the pools.
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        shut_down_pool(system.pools[i]);

    // If the user is authenticated, sign out.
    // The sessions would just time out anyway, but this way they won't count
    // against the limit, and other less active sessions (that the user stil
    // wants) can stick around.
    background_authentication_data& auth = system.authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);
    if (auth.status.state == background_authentication_state::SUCCEEDED)
    {
        try
        {
            // Sign out of the session.
            web_connection connection;
            null_check_in check_in;
            null_progress_reporter reporter;
            web_request request(web_request_method::DELETE,
                system.authentication.url, blob(), std::vector<string>());
            perform_web_request(check_in, reporter, connection,
                system.authentication.session_data, request);
        }
        catch (...)
        {
            // As noted above, if this fails, it doesn't really matter.
        }
    }
}

void wake_up_waiting_jobs(background_job_queue& queue)
{
    boost::lock_guard<boost::mutex> lock(queue.mutex);
    inc_version(queue.version);
    ++queue.wake_up_counter;
    while (!queue.waiting_jobs.empty())
    {
        queue.jobs.push(queue.waiting_jobs.top());
        queue.waiting_jobs.pop();
        queue.cv.notify_one();
    }
}

background_execution_system::background_execution_system()
{
    impl_ = new background_execution_system_impl;
    initialize_system(*impl_);
}
background_execution_system::~background_execution_system()
{
    shut_down_system(*impl_);
    delete impl_;
}

void static
update_status(keyed_data<background_execution_pool_status>& status,
    background_execution_pool& pool)
{
    background_job_queue& queue = *pool.queue;
    boost::mutex::scoped_lock lock(queue.mutex);
    auto thread_count = pool.threads.size();
    refresh_keyed_data(status,
        combine_ids(get_id(queue.version), make_id(thread_count)));
    if (!is_valid(status))
    {
        background_execution_pool_status new_status;
        new_status.thread_count = thread_count;
        for (auto const& f : queue.failed_jobs)
        {
            background_job_failure_report report;
            report.job = f.job.get();
            report.message = f.message;
            new_status.transient_failures.push_back(report);
        }
        new_status.queued_job_count = queue.reported_size;
        new_status.idle_thread_count = queue.n_idle_threads;
        new_status.job_info = queue.job_info;
        set(status, new_status);
    }
}

void update_status(background_execution_system_status& status,
    background_execution_system& system)
{
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        update_status(status.pools[i], system.impl_->pools[i]);
}

void static
get_permanent_failures(std::list<background_job_failure_report>& failures,
    background_execution_pool& pool)
{
    background_job_queue& queue = *pool.queue;
    boost::mutex::scoped_lock lock(queue.mutex);
    for (auto i = queue.failed_jobs.begin(); i != queue.failed_jobs.end(); )
    {
        if (!i->is_transient)
        {
            // Record it.
            background_job_failure_report report;
            report.job = i->job.get();
            report.message = i->message;
            failures.push_back(report);
            // Erase it.
            i = queue.failed_jobs.erase(i);
        }
        else
            ++i;
    }
}

std::list<background_job_failure_report>
get_permanent_failures(background_execution_system& system)
{
    std::list<background_job_failure_report> failures;
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        get_permanent_failures(failures, system.impl_->pools[i]);
    return failures;
}

size_t canceled_job_count(background_job_queue& queue)
{
    boost::mutex::scoped_lock lock(queue.mutex);
    job_priority_queue copy = queue.jobs;
    size_t count = 0;
    while (!copy.empty())
    {
        auto top = copy.top();
        if (top->cancel)
            ++count;
        copy.pop();
    }
    return count;
}

void clear_pending_jobs(background_execution_pool& pool)
{
    auto& queue = *pool.queue;
    boost::mutex::scoped_lock lock(queue.mutex);
    inc_version(queue.version);
    queue.jobs = job_priority_queue();
    queue.waiting_jobs = job_priority_queue();
}

void clear_pending_jobs(background_execution_system& system)
{
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        clear_pending_jobs(system.impl_->pools[i]);
}

void clear_all_jobs(background_execution_pool& pool)
{
    clear_pending_jobs(pool);

    for (auto& i : pool.threads)
    {
        auto& active_job = i->data_proxy->active_job;
        if (active_job)
            active_job->cancel = true;
    }

    {
        auto& queue = *pool.queue;
        boost::mutex::scoped_lock lock(queue.mutex);
        inc_version(queue.version);
        queue.failed_jobs.clear();
    }
}

void clear_all_jobs(background_execution_system& system)
{
    // Give web writes a chance to finish.
    auto& web_write_pool =
        system.impl_->pools[unsigned(background_job_queue_type::WEB_WRITE)];
    int sleep_count = 0;
    while (!is_pool_idle(web_write_pool) && sleep_count < 30)
    {
        boost::this_thread::sleep_for(boost::chrono::seconds(1));
        ++sleep_count;
    }

    // Now clear the jobs.
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        clear_all_jobs(system.impl_->pools[i]);
}

void clear_canceled_jobs(
    background_job_queue& queue,
    job_priority_queue& pqueue)
{
    job_priority_queue filtered;
    while (!pqueue.empty())
    {
        auto job = pqueue.top();
        if (job->cancel)
        {
            if (!job->hidden)
                --queue.reported_size;
            queue.job_info.erase(&*job);
        }
        else
            filtered.push(job);
        pqueue.pop();
    }
    pqueue = filtered;
}

void clear_canceled_jobs(background_execution_pool& pool)
{
    auto& queue = *pool.queue;
    boost::mutex::scoped_lock lock(queue.mutex);
    clear_canceled_jobs(queue, queue.jobs);
    clear_canceled_jobs(queue, queue.waiting_jobs);
}

void clear_canceled_jobs(background_execution_system& system)
{
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
        clear_canceled_jobs(system.impl_->pools[i]);
}

void reduce_memory_cache_size(background_execution_system& system, int desired_size)
{
    reduce_memory_cache_size(system.impl_->cache, desired_size);
}

void clear_memory_cache(background_execution_system& system)
{
    reduce_memory_cache_size(system.impl_->cache, 0);
}

memory_cache_snapshot
get_memory_cache_snapshot(background_execution_system& system)
{
    auto& cache = system.impl_->cache;
    boost::lock_guard<boost::mutex> lock(cache.mutex);
    memory_cache_snapshot snapshot;
    snapshot.in_use.reserve(cache.records.size());
    for (auto const& record : cache.records)
    {
        auto const& data = record.second.data;
        if (is_initialized(data))
        {
            memory_cache_entry_info info;
            info.type = data.ptr->type_info();
            info.data_size = data.ptr->deep_size();
            // Put the entry's info the appropriate list depending on whether
            // or not its in the eviction list.
            if (record.second.eviction_list_iterator !=
                cache.eviction_list.records.end())
            {
                snapshot.pending_eviction.push_back(info);
            }
            else
            {
                snapshot.in_use.push_back(info);
            }
        }
    }
    return snapshot;
}

// GENERAL WEB REQUESTS

background_web_request_job::background_web_request_job(
    alia__shared_ptr<background_execution_system> const& bg,
    id_interface const& id,
    web_request const& request,
    dynamic_type_interface const* result_interface)
    : request(request)
    , result_interface(result_interface)
{
    this->system = bg;
    this->id.store(id);
}

bool background_web_request_job::inputs_ready()
{
    background_authentication_status status;
    get_authentication_result(*this->system, &status, &this->session);
    return status.state == background_authentication_state::SUCCEEDED;
}

void background_web_request_job::execute(check_in_interface& check_in,
    progress_reporter_interface& reporter)
{
    web_response response =
        perform_web_request(check_in, reporter, *this->connection, session,
            request);
    check_in();
    auto value = parse_json_response(response);
    auto immutable = result_interface->value_to_immutable(value);
    set_cached_data(*this->system, this->id.get(), immutable);
}

background_job_info background_web_request_job::get_info() const
{
    background_job_info info;
    info.description =
        boost::to_upper_copy(to_string(request.method)) + " " + request.url;
    return info;
}

// FRAMEWORK CONTEXT

void set_context_request_parameters(
    alia__shared_ptr<background_execution_system> const& system,
    framework_usage_info const& framework,
    context_request_parameters const& parameters)
{
    background_context_request_data& data = system->impl_->context;
    boost::lock_guard<boost::mutex> lock(data.mutex);

    // Update the request ID and invalidate the old info.
    inc_version(data.id);
    data.status =
        background_context_request_status(
            background_context_request_state::IN_PROGRESS);

    // Start a context request job.
    background_context_request* job = new background_context_request;
    job->system = system;
    job->id.store(get_id(data.id));
    job->framework = framework;
    job->parameters = parameters;
    add_background_job(*system, background_job_queue_type::WEB_READ,
        &data.job_controller, job);
}

background_context_request_status
get_context_request_status(background_execution_system& system)
{
    background_context_request_data& data = system.impl_->context;
    boost::lock_guard<boost::mutex> lock(data.mutex);
    return data.status;
}

void get_context_request_result(
    background_execution_system& system,
    background_context_request_status* status,
    framework_context* response)
{
    background_context_request_data& data = system.impl_->context;
    boost::lock_guard<boost::mutex> lock(data.mutex);
    *status = data.status;
    *response = data.context;
}

void static
wake_up_web_jobs(background_execution_system& system)
{
    wake_up_waiting_jobs(
        *system.impl_->pools[
            int(background_job_queue_type::WEB_READ)].queue);
    wake_up_waiting_jobs(
        *system.impl_->pools[
            int(background_job_queue_type::WEB_WRITE)].queue);
    wake_up_waiting_jobs(
        *system.impl_->pools[
            int(background_job_queue_type::REMOTE_CALCULATION)].queue);
    wake_up_waiting_jobs(
        *system.impl_->pools[
            int(background_job_queue_type::NOTIFICATION_WATCH)].queue);
}

void record_context_request_success(background_execution_system& system,
    id_interface const& id, framework_context const& context)
{
    {
        background_context_request_data& data = system.impl_->context;
        boost::lock_guard<boost::mutex> lock(data.mutex);

        // Only use this session data if it's associated with the most recent
        // request info.
        if (get_id(data.id) == id)
        {
            data.status =
                background_context_request_status(
                    background_context_request_state::SUCCEEDED);
            data.context = context;
        }
    }
    wake_up_web_jobs(system);
}

void record_context_request_failure(background_execution_system& system,
    id_interface const& id, background_context_request_state failure_type,
    string const& message)
{
    background_context_request_data& data = system.impl_->context;
    boost::lock_guard<boost::mutex> lock(data.mutex);

    // Only record the failure if it's associated with the most recent
    // request info.
    if (get_id(data.id) == id)
    {
        data.status =
            background_context_request_status(failure_type, message);
    }
}

bool is_failure(background_context_request_state state)
{
    return
        state != background_context_request_state::NO_REQUEST &&
        state != background_context_request_state::IN_PROGRESS &&
        state != background_context_request_state::SUCCEEDED;
}

char const*
get_description(background_context_request_state state)
{
    switch (state)
    {
     case background_context_request_state::NO_REQUEST:
        return "No realm was specified.";
     case background_context_request_state::NOT_FOUND:
        return "The requested realm doesn't exist or you don't have access.";
     case background_context_request_state::FAILED_TO_CONNECT:
        return "Astroid is unable to connect to thinknode.";
     case background_context_request_state::INVALID_RESPONSE:
        return "Astroid was unable to understand the response from thinknode.";
     case background_context_request_state::IN_PROGRESS:
        return "Connecting...";
     case background_context_request_state::SUCCEEDED:
     default:
        return "Success!";
    }
}

void set_framework_context(background_execution_system& system,
    framework_context const& context)
{
    background_context_request_data& state = system.impl_->context;
    boost::lock_guard<boost::mutex> lock(state.mutex);
    state.context = context;
}

void clear_framework_context(background_execution_system& system)
{
    background_context_request_data& data = system.impl_->context;
    boost::lock_guard<boost::mutex> lock(data.mutex);
    data.status =
        background_context_request_status(
            background_context_request_state::NO_REQUEST);
}

bool background_context_request::inputs_ready()
{
    background_authentication_status status;
    get_authentication_result(*this->system, &status, &this->session);
    return status.state == background_authentication_state::SUCCEEDED;
}

void background_context_request::execute(
    check_in_interface& check_in, progress_reporter_interface& reporter)
{
    // Enable this to bypass the context request.
    //record_context_request_success(*this->system, this->id.get(),
    //    framework_context(this->framework, "no_context"));
    //return;

    // No explicit app version supplied, get the app installed in the selected realm
    if (this->parameters.app_version.length() == 0)
    {
        auto app_request =
            make_get_request(
            construct_realm_app_request_url(this->framework),
            no_headers);
        web_response app_response;
        try
        {
            app_response =
                perform_web_request(check_in, reporter, *this->connection,
                this->session, app_request);
        }
        catch (web_request_failure& failure)
        {
            record_context_request_failure(
                *this->system, this->id.get(),
                failure.response_code() == 404 ?
                background_context_request_state::NOT_FOUND :
            background_context_request_state::FAILED_TO_CONNECT,
                failure.what());
            return;
        }
        catch (std::exception& e)
        {
            record_context_request_failure(*this->system, this->id.get(),
                background_context_request_state::FAILED_TO_CONNECT,
                e.what());
            return;
        }
        catch (...)
        {
            record_context_request_failure(*this->system, this->id.get(),
                background_context_request_state::FAILED_TO_CONNECT,
                "Unknown error");
            return;
        }
        std::vector<realm_app_response> app_typed_response;
        try
        {
            from_value(&app_typed_response, parse_json_response(app_response));
            for (auto installed_app : app_typed_response)
            {
                if (installed_app.status != "installed")
                {
                    continue;
                }

                if (installed_app.app == this->parameters.app_name)
                {
                    this->parameters.app_version = installed_app.version;
                }
            }
        }
        catch (std::exception const& e)
        {
            record_context_request_failure(*this->system, this->id.get(),
                background_context_request_state::INVALID_RESPONSE,
                e.what());
            return;
        }
    }

    auto request =
        make_get_request(
            construct_context_request_url(this->framework, this->parameters),
            no_headers);
    web_response response;
    try
    {
        response =
            perform_web_request(check_in, reporter, *this->connection,
                this->session, request);
    }
    catch (web_request_failure& failure)
    {
        record_context_request_failure(
            *this->system, this->id.get(),
            failure.response_code() == 404 ?
                background_context_request_state::NOT_FOUND :
                background_context_request_state::FAILED_TO_CONNECT,
            failure.what());
        return;
    }
    catch (std::exception& e)
    {
        record_context_request_failure(*this->system, this->id.get(),
            background_context_request_state::FAILED_TO_CONNECT,
            e.what());
        return;
    }
    catch (...)
    {
        record_context_request_failure(*this->system, this->id.get(),
            background_context_request_state::FAILED_TO_CONNECT,
            "Unknown error");
        return;
    }
    context_response typed_response;
    try
    {
        from_value(&typed_response, parse_json_response(response));
    }
    catch (std::exception const& e)
    {
        record_context_request_failure(*this->system, this->id.get(),
            background_context_request_state::INVALID_RESPONSE,
            e.what());
        return;
    }
    record_context_request_success(*this->system, this->id.get(),
        framework_context(this->framework, typed_response.id));
}

background_job_info background_context_request::get_info() const
{
    background_job_info info;
    info.description = "context request";
    return info;
}

// AUTHENTICATION

void set_authentication_info(
    alia__shared_ptr<background_execution_system> const& system,
    string const& framework_api_url,
    web_authentication_credentials const& credentials)
{
    background_authentication_data& auth = system->impl_->authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);

    // Update the authentication ID and invalidate the old info.
    inc_version(auth.id);
    auth.status = background_authentication_state::IN_PROGRESS;

    // Start an authentication job.
    background_authentication_request* job =
        new background_authentication_request;
    job->system = system;
    job->id.store(get_id(auth.id));
    job->framework_api_url = framework_api_url;
    job->credentials = credentials;
    add_background_job(*system, background_job_queue_type::WEB_READ,
        &auth.job_controller, job);
}

void set_authentication_token(
    alia__shared_ptr<background_execution_system> const& system,
    string const& token)
{
    {
        background_authentication_data& auth = system->impl_->authentication;
        boost::lock_guard<boost::mutex> lock(auth.mutex);

        // Update the authentication ID and invalidate the old info.
        inc_version(auth.id);

        // Not sure what this is used for but set it to note that login was
        // handled in the launcher
        auth.url = "token from launcher";

        auth.status =
            background_authentication_status(
                background_authentication_state::SUCCEEDED);
        auth.session_data = web_session_data(token);
    }
    wake_up_web_jobs(*system);
}

void clear_authentication_info(background_execution_system& system)
{
    invalidate_authentication_data(system,
        background_authentication_state::NO_CREDENTIALS);
}

background_authentication_status
get_authentication_status(background_execution_system& system)
{
    background_authentication_data& auth = system.impl_->authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);
    return auth.status;
}

void get_authentication_result(background_execution_system& system,
    background_authentication_status* status,
    web_session_data* session)
{
    background_authentication_data& auth = system.impl_->authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);
    *status = auth.status;
    *session = auth.session_data;
}

void record_authentication_success(background_execution_system& system,
    id_interface const& id, web_session_data const& session_data,
    string const& url)
{
    {
        background_authentication_data& auth = system.impl_->authentication;
        boost::lock_guard<boost::mutex> lock(auth.mutex);

        // Only use this session data if it's associated with the most recent
        // authentication info.
        if (get_id(auth.id) == id)
        {
            auth.status =
                background_authentication_status(
                    background_authentication_state::SUCCEEDED);
            auth.session_data = session_data;
            auth.url = url;
        }
    }
    wake_up_web_jobs(system);
}

void record_authentication_failure(background_execution_system& system,
    id_interface const& id, background_authentication_state failure_type,
    string const& message)
{
    background_authentication_data& auth = system.impl_->authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);

    // Only record the failure if it's associated with the most recent
    // authentication info.
    if (get_id(auth.id) == id)
    {
        auth.status.state = failure_type;
        auth.status.message = message;
    }
}

bool is_failure(background_authentication_state state)
{
    return
        state != background_authentication_state::NO_CREDENTIALS &&
        state != background_authentication_state::IN_PROGRESS &&
        state != background_authentication_state::SUCCEEDED;
}

char const*
get_description(background_authentication_state state)
{
    switch (state)
    {
     case background_authentication_state::NO_CREDENTIALS:
        return "No credentials were provided.";
     case background_authentication_state::INVALID_CREDENTIALS:
        return "The username or password you entered is incorrect.";
     case background_authentication_state::FAILED_TO_CONNECT:
        return "Astroid is unable to connect to thinknode.";
     case background_authentication_state::SESSION_EXPIRED:
        return "Your session has expired.";
     case background_authentication_state::SESSION_TIMED_OUT:
        return "Your session has timed out due to inactivity.";
     case background_authentication_state::IN_PROGRESS:
        return "Authenticating...";
     case background_authentication_state::SUCCEEDED:
        return "Success!";
     default:
        throw exception("invalid background_authentication_state");
    }
}

void invalidate_authentication_data(background_execution_system& system,
    background_authentication_state failure_type)
{
    background_authentication_data& auth = system.impl_->authentication;
    boost::lock_guard<boost::mutex> lock(auth.mutex);
    auth.status.state = failure_type;
    auth.status.message.clear();
}

void static
try_authentication(
    background_execution_system& system, id_interface const& id,
    web_connection& connection, web_request const& request,
    web_authentication_credentials const& user_info,
    web_session_data* session_data)
{
    // Enable this to bypass authentication.
    //record_authentication_success(system, id, *session_data, request.url);
    //return;
    try
    {
        *session_data =
            authenticate_web_user(connection, request, user_info);
        record_authentication_success(system, id, *session_data, request.url);
    }
    catch (web_request_failure& failure)
    {
        record_authentication_failure(
            system, id,
            failure.response_code() == 401 ?
                background_authentication_state::INVALID_CREDENTIALS :
                background_authentication_state::FAILED_TO_CONNECT,
            failure.what());
    }
    catch (std::exception& e)
    {
        record_authentication_failure(
            system, id, background_authentication_state::FAILED_TO_CONNECT,
            e.what());
    }
    catch (...)
    {
        record_authentication_failure(
            system, id, background_authentication_state::FAILED_TO_CONNECT,
            "Unknown error");
    }
}

bool background_authentication_request::inputs_ready()
{
    return true;
}

void background_authentication_request::execute(
    check_in_interface& check_in,
    progress_reporter_interface& reporter)
{
    auto url = this->framework_api_url + "/cas/login";
    web_session_data session;
    try_authentication(*system, id.get(), *this->connection,
        make_get_request(url, no_headers), this->credentials, &session);
}

background_job_info background_authentication_request::get_info() const
{
    background_job_info info;
    info.description = "authentication request";
    return info;
}

// MUTABLE DATA CACHING

// Look up an entity in the mutable data cache and return a reference to it.
// If it doesn't already exist, create it.
static mutable_cache_record&
find_or_create_mutable_cache_record(
    mutable_cache& cache,
    id_interface const& entity_id)
{
    // Look up the associated record.
    auto i = cache.records.find(&entity_id);
    // If doesn't exist yet, create it.
    if (i == cache.records.end())
    {
        mutable_cache_record new_record;
        new_record.has_job = false;
        new_record.entity_id.store(entity_id);
        i = cache.records.insert(
                mutable_cache_record_map::value_type(
                    &new_record.entity_id.get(), new_record)).first;
    }
    return i->second;
}

void process_mutable_cache_updates(background_execution_system& system)
{
    auto& cache = system.impl_->mutable_cache;
    process_queue_items(cache.updates,
        [&](mutable_cache_update const& update)
        {
            auto& record =
                find_or_create_mutable_cache_record(
                    cache,
                    update.entity_id.get());
            // Record the new value. Note that we ignore updates from normal
            // retrievals if the mutable entity is being explicitly watched.
            if (update.source == mutable_value_source::WATCH ||
                record.watch_count == 0)
            {
                record.value = update.value;
            }
            // If this update came from a normal retrieval, that means the
            // job finished, so clear the job flag.
            if (update.source == mutable_value_source::RETRIEVAL)
            {
                record.has_job = false;
            }
            // And record that the cache has updated.
            inc_version(cache.update_id);
        });
}

void clear_mutable_data_cache(background_execution_system& system)
{
    auto& cache = system.impl_->mutable_cache;
    cache.records.clear();
    inc_version(cache.update_id);
    inc_version(cache.refresh_id);
}

bool
add_mutable_entity_watch(
    background_execution_system& system,
    id_interface const& entity_id)
{
    auto& cache = system.impl_->mutable_cache;
    auto& record = find_or_create_mutable_cache_record(cache, entity_id);
    ++record.watch_count;
    return record.watch_count == 1;
}

void
set_mutable_entity_watch_job(
    background_execution_system& system,
    id_interface const& entity_id,
    background_job_interface* job)
{
    auto& cache = system.impl_->mutable_cache;
    auto i = cache.records.find(&entity_id);
    assert(i != cache.records.end());
    if (i != cache.records.end())
    {
        auto& record = i->second;
        assert(!record.watch_job);
        record.watch_job.reset(new background_job_controller);
        add_background_job(
            system,
            background_job_queue_type::NOTIFICATION_WATCH,
            record.watch_job.get(),
            job,
            BACKGROUND_JOB_HIDDEN);
    }
}

void
remove_mutable_entity_watch(
    background_execution_system& system,
    id_interface const& entity_id)
{
    auto& cache = system.impl_->mutable_cache;
    auto i = cache.records.find(&entity_id);
    assert(i != cache.records.end());
    if (i != cache.records.end())
    {
        auto& record = i->second;
        assert(record.watch_count > 0);
        --record.watch_count;
        if (record.watch_count == 0)
        {
            assert(record.watch_job);
            if (record.watch_job)
            {
                record.watch_job->cancel();
            }
            record.watch_job.reset();
        }
    }
}

}
