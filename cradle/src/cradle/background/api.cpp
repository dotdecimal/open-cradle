#include <cradle/background/internals.hpp>

namespace cradle {

// JOBS

background_job_controller::~background_job_controller()
{
    this->cancel();
    delete data_;
}
void background_job_controller::reset()
{
    this->cancel();
    delete data_;
    data_ = 0;
}
background_job_state background_job_controller::state() const
{
    assert(data_ && data_->job);
    return data_->job->state;
}
float background_job_controller::progress() const
{
    assert(data_ && data_->job);
    return data_->job->progress;
}
void background_job_controller::cancel()
{
    if (data_ && data_->job)
        data_->job->cancel = true;
}

background_job_status
get_background_job_status(background_job_controller& controller)
{
    background_job_status status;
    status.state = controller.state();
    status.progress = controller.progress();
    return status;
}

template<class ExecutionLoop>
void queue_background_job(
    background_execution_pool& pool,
    background_job_ptr const& job_ptr,
    bool ensure_idle_thread_exists = false)
{
    // Add it to the queue and notify one waiting thread.
    background_job_queue& queue = *pool.queue;
    {
        boost::lock_guard<boost::mutex> lock(queue.mutex);
        inc_version(queue.version);
        queue.jobs.push(job_ptr);
        if (!job_ptr->hidden)
        {
            queue.job_info[&*job_ptr] = job_ptr->job->get_info();
            ++queue.reported_size;
        }
        // If requested, ensure that there will be an idle thread to pick up
        // the new job.
        if (ensure_idle_thread_exists &&
            queue.n_idle_threads < queue.jobs.size())
        {
            add_background_thread<ExecutionLoop>(pool);
        }
    }
    queue.cv.notify_one();
}

void queue_background_job(
    background_execution_system& system,
    background_job_queue_type queue,
    background_job_ptr const& job_ptr)
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
    // Queue the job.
    switch (queue)
    {
     case background_job_queue_type::CALCULATION:
        queue_background_job<background_job_execution_loop>(
            system.impl_->pools[int(queue)], job_ptr);
        break;
     case background_job_queue_type::DISK:
        queue_background_job<background_job_execution_loop>(
            system.impl_->pools[int(queue)], job_ptr);
        break;
     case background_job_queue_type::WEB_READ:
        queue_background_job<web_request_processing_loop>(
            system.impl_->pools[int(queue)], job_ptr);
        break;
     case background_job_queue_type::WEB_WRITE:
        queue_background_job<web_request_processing_loop>(
            system.impl_->pools[int(queue)], job_ptr);
        break;
     case background_job_queue_type::NOTIFICATION_WATCH:
        queue_background_job<web_request_processing_loop>(
            system.impl_->pools[int(queue)], job_ptr, true);
        break;
     case background_job_queue_type::REMOTE_CALCULATION:
        queue_background_job<web_request_processing_loop>(
            system.impl_->pools[int(queue)], job_ptr, true);
        break;
    }
}

void add_background_job(
    background_execution_system& system,
    background_job_queue_type queue,
    background_job_controller* controller,
    background_job_interface* job,
    background_job_flag_set flags, int priority)
{
    background_job_ptr ptr(
        new background_job_execution_data(job, priority,
            (flags & BACKGROUND_JOB_HIDDEN) ? true : false));
    if (controller)
    {
        // Set the controller to refer to the new job entry.
        delete controller->data_;
        controller->data_ = new background_job_controller_data;
        controller->data_->job = ptr;
    }
    queue_background_job(system, queue, ptr);
}

void add_untyped_background_job(
    untyped_background_data_ptr& ptr, background_execution_system& system,
    background_job_queue_type queue, background_job_interface* job,
    background_job_flag_set flags, int priority)
{
    auto* record = ptr.record();
    {
        boost::lock_guard<boost::mutex> lock(record->owner_cache->mutex);
        // Check that the pointer actually needs a job.
        // It's possible that another thread already added one.
        if (record->state == background_data_state::NOWHERE)
        {
            background_job_controller controller;
            add_background_job(system, queue, &controller, job, flags, priority);
            job = 0; // So it doesn't get deleted below.
            swap(*record->job, controller);
            record->state = background_data_state::COMPUTING;
        }
    }
    // We were supposed to assume ownership of the job, so if it wasn't
    // needed, delete it.
    delete job;
    ptr.update();
}

void retry_background_job(
    background_execution_system& system,
    background_job_queue_type queue_index,
    background_job_execution_data* job_data)
{
    auto& queue = *system.impl_->pools[int(queue_index)].queue;
    // Find the job in the failure list, get the actual shared pointer, and
    // remove it from that list.
    background_job_ptr job_ptr;
    {
        boost::lock_guard<boost::mutex> lock(queue.mutex);
        inc_version(queue.version);
        for (auto i = queue.failed_jobs.begin();
            i != queue.failed_jobs.end(); ++i)
        {
            if (i->job.get() == job_data)
            {
                job_ptr = i->job;
                queue.failed_jobs.erase(i);
                break;
            }
        }
    }
    if (job_ptr)
        queue_background_job(system, queue_index, job_ptr);
}

// CACHING

void static
remove_from_eviction_list(
    background_cache& cache, background_cache_record* record);

void static
acquire_cache_record_no_lock(background_cache_record* record)
{
    ++record->ref_count;
    auto& evictions = record->owner_cache->eviction_list.records;
    if (record->eviction_list_iterator != evictions.end())
    {
        assert(record->ref_count == 1);
        remove_from_eviction_list(*record->owner_cache, record);
    }
}

background_data_status static
make_background_data_status(background_data_state state, float progress = 0)
{
    background_data_status status;
    status.state = state;
    status.progress = progress;
    return status;
}

background_cache_record* acquire_cache_record(
    background_execution_system& system, id_interface const& key)
{
    auto& cache = system.impl_->cache;
    boost::lock_guard<boost::mutex> lock(cache.mutex);
    auto i = cache.records.find(&key);
    if (i == cache.records.end())
    {
        background_cache_record r;
        r.owner_cache = &cache;
        r.eviction_list_iterator = cache.eviction_list.records.end();
        r.key.store(key);
        r.state = background_data_state::NOWHERE;
        r.progress = 0;
        r.ref_count = 0;
        r.job.reset(new background_job_controller);
        i = cache.records.insert(
                cache_record_map::value_type(&r.key.get(), r)).first;
    }
    background_cache_record* r = &i->second;
    acquire_cache_record_no_lock(r);
    return r;
}

void acquire_cache_record(background_cache_record* record)
{
    boost::lock_guard<boost::mutex> lock(record->owner_cache->mutex);
    acquire_cache_record_no_lock(record);
}

void static
add_to_eviction_list(background_cache& cache, background_cache_record* record)
{
    auto& list = cache.eviction_list;
    assert(record->eviction_list_iterator == list.records.end());
    record->eviction_list_iterator =
        list.records.insert(list.records.end(), record);
    if (record->data.ptr)
        list.total_size += record->data.ptr->deep_size();
}

void static
remove_from_eviction_list(background_cache& cache, background_cache_record* record)
{
    auto& list = cache.eviction_list;
    assert(record->eviction_list_iterator != list.records.end());
    list.records.erase(record->eviction_list_iterator);
    record->eviction_list_iterator = list.records.end();
    if (record->data.ptr)
        list.total_size -= record->data.ptr->deep_size();
}

void reduce_memory_cache_size(background_cache& cache, int desired_size)
{
    // We need to keep the jobs around until after the mutex is released
    // because they may recursively release other records.
    std::list<alia__shared_ptr<background_job_controller> > evicted_jobs;
    {
        boost::lock_guard<boost::mutex> lock(cache.mutex);
        while (!cache.eviction_list.records.empty() &&
            cache.eviction_list.total_size > desired_size * 0x100000)
        {
            auto const& i = cache.eviction_list.records.front();
            auto data_size = i->data.ptr ? i->data.ptr->deep_size() : 0;
            evicted_jobs.push_back(i->job);
            cache.records.erase(&i->key.get());
            cache.eviction_list.records.pop_front();
            cache.eviction_list.total_size -= data_size;
        }
    }
    for (auto const& i : evicted_jobs)
    {
        if (i->is_valid())
            i->cancel();
    }
}

void release_cache_record(background_cache_record* record)
{
    auto& cache = *record->owner_cache;
    {
        boost::lock_guard<boost::mutex> lock(cache.mutex);
        --record->ref_count;
        if (record->ref_count == 0)
            add_to_eviction_list(cache, record);
    }
}

background_job_controller*
get_job_interface(background_cache_record* record)
{
    boost::lock_guard<boost::mutex> lock(record->owner_cache->mutex);
    return record->job.get();
}

void update_background_data_progress(
    background_execution_system& system, id_interface const& key,
    float progress)
{
    auto& cache = system.impl_->cache;

    {
        boost::lock_guard<boost::mutex> lock(cache.mutex);

        auto i = cache.records.find(&key);
        if (i == cache.records.end())
            return;

        background_cache_record* r = &i->second;
        r->progress = progress;
    }
}

void set_cached_data(
    background_execution_system& system, id_interface const& key,
    untyped_immutable const& data)
{
    auto& cache = system.impl_->cache;

    {
        boost::lock_guard<boost::mutex> lock(cache.mutex);

        auto i = cache.records.find(&key);
        if (i == cache.records.end())
            return;

        background_cache_record* r = &i->second;
        r->data = data;
        r->state = background_data_state::READY;
        r->progress = 0;
        // Ideally, the job controller should be reset here, since we don't
        // really need it anymore, but this causes some tricky synchronization
        // issues with the UI code that's observing it.
        //r->job->reset();
    }

    // Setting this data could've made it possible for any of the waiting
    // calculation jobs to run.
    wake_up_waiting_jobs(*system.impl_->pools[
        int(background_job_queue_type::CALCULATION)].queue);
}

void
reset_cached_data(background_execution_system& system, id_interface const& key)
{
    auto& cache = system.impl_->cache;

    {
        boost::lock_guard<boost::mutex> lock(cache.mutex);

        auto i = cache.records.find(&key);
        if (i == cache.records.end())
            return;

        background_cache_record* r = &i->second;
        r->state = background_data_state::NOWHERE;
    }
}

string get_key_string(background_cache_record* record)
{
    return to_string(record->key);
}

// BACKGROUND DATA POINTERS

void untyped_background_data_ptr::reset()
{
    if (r_)
    {
        release_cache_record(r_);
        r_ = 0;
    }
    status_ = make_background_data_status(background_data_state::NOWHERE);
    key_.clear();
    data_ = untyped_immutable();
}

void untyped_background_data_ptr::reset(
    background_execution_system& system, id_interface const& key)
{
    if (!key_.matches(key))
    {
        this->reset();
        this->acquire(system, key);
    }
}

void untyped_background_data_ptr::acquire(
    background_execution_system& system, id_interface const& key)
{
    r_ = acquire_cache_record(system, key);
    status_ = make_background_data_status(background_data_state::NOWHERE);
    update();
    key_.store(key);
}

void untyped_background_data_ptr::update()
{
    if (status_.state != background_data_state::READY)
    {
        status_.state = r_->state;
        status_.progress = r_->progress;
        if (status_.state == background_data_state::READY)
        {
            boost::lock_guard<boost::mutex> lock(r_->owner_cache->mutex);
            data_ = r_->data;
        }
    }
}

void untyped_background_data_ptr::copy(
    untyped_background_data_ptr const& other)
{
    r_ = other.r_;
    if (r_)
        acquire_cache_record(r_);
    status_ = other.status_;
    key_ = other.key_;
    data_ = other.data_;
}

void untyped_background_data_ptr::swap(untyped_background_data_ptr& other)
{
    using std::swap;
    swap(r_, other.r_);
    swap(status_, other.status_);
    swap(data_, other.data_);
    swap(key_, other.key_);
}

// MUTABLE DATA CACHING

untyped_immutable
get_cached_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    boost::function<void()> const& dispatch_job)
{
    auto& cache = system.impl_->mutable_cache;
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
    auto& record = i->second;

    // If there's no associated value or job, invoke the callback.
    if (!is_initialized(record.value) && !record.has_job &&
        record.watch_count == 0)
    {
        dispatch_job();
        // Record that we now have a job.
        record.has_job = true;
    }

    // Return the associated value. (It may or may not be initialized.)
    return record.value;
}

void
refresh_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    mutable_refresh_flag_set flags)
{
    auto& cache = system.impl_->mutable_cache;
    auto record = cache.records.find(&entity_id);
    // If there's no associated record, there's nothing to refresh.
    if (record == cache.records.end())
        return;
    record->second.has_job =
        (flags & MUTABLE_REFRESH_NO_JOB_NEEDED) ? true : false;
    record->second.value = untyped_immutable();
    // Also increment the cache's update_id to reflect the fact that this
    // entity no longer has an up-to-date value.
    inc_version(cache.update_id);
}

void set_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    untyped_immutable const& new_value, mutable_value_source source)
{
    auto& cache = system.impl_->mutable_cache;
    assert(is_initialized(new_value));
    // Create an update message and queue it.
    mutable_cache_update update;
    update.entity_id.store(entity_id);
    update.value = new_value;
    update.source = source;
    push(cache.updates, update);
}

value_id_by_reference<local_id>
get_mutable_cache_update_id(background_execution_system& system)
{
    auto& cache = system.impl_->mutable_cache;
    return get_id(cache.update_id);
}

// mutable_entity_watch

mutable_entity_watch::mutable_entity_watch(mutable_entity_watch const& other)
{
    if (other.is_active())
        this->watch(other.system_, other.entity_id());
}

mutable_entity_watch::mutable_entity_watch(mutable_entity_watch&& other)
{
    swap(other.system_, this->system_);
    swap(other.refresh_id_, this->refresh_id_);
    swap(other.entity_id_, this->entity_id_);
}

mutable_entity_watch&
mutable_entity_watch::operator=(mutable_entity_watch const& other)
{
    this->reset();
    if (other.is_active())
        this->watch(other.system_, other.entity_id());
    return *this;
}

mutable_entity_watch&
mutable_entity_watch::operator=(mutable_entity_watch&& other)
{
    this->reset();
    swap(other.system_, this->system_);
    swap(other.refresh_id_, this->refresh_id_);
    swap(other.entity_id_, this->entity_id_);
    return *this;
}

bool mutable_entity_watch::is_active() const
{
    return system_ &&
        // Check that the cache hasn't been refreshed since this watch started.
        this->refresh_id_.matches(get_id(system_->impl_->mutable_cache.refresh_id));
}

void
mutable_entity_watch::watch(
    alia__shared_ptr<background_execution_system> const& system,
    id_interface const& entity_id,
    boost::function<background_job_interface*()> const& job_creator)
{
    this->reset();
    if (this->watch(system, entity_id))
        set_mutable_entity_watch_job(*system, entity_id, job_creator());
}

bool
mutable_entity_watch::watch(
    alia__shared_ptr<background_execution_system> const& system,
    id_interface const& entity_id)
{
    system_ = system;
    refresh_id_.store(get_id(system->impl_->mutable_cache.refresh_id));
    entity_id_.store(entity_id);
    return add_mutable_entity_watch(*system, entity_id);
}

void mutable_entity_watch::reset()
{
    if (this->is_active())
    {
        remove_mutable_entity_watch(*system_, entity_id_.get());
    }
    system_.reset();
    refresh_id_.clear();
    entity_id_.clear();
}

// OTHER MISCELLANY

bool get_session_and_context(background_execution_system& system,
    web_session_data* session, framework_context* context)
{
    {
        background_authentication_status status;
        get_authentication_result(system, &status, session);
        if (status.state != background_authentication_state::SUCCEEDED)
        {
            return false;
        }
    }

    {
        background_context_request_status status;
        get_context_request_result(system, &status, context);
        if (status.state != background_context_request_state::SUCCEEDED)
        {
            return false;
        }
    }

    return true;
}

}
