#ifndef CRADLE_BACKGROUND_INTERNALS_HPP
#define CRADLE_BACKGROUND_INTERNALS_HPP

#include <cradle/background/system.hpp>
#include <cradle/background/api.hpp>
#include <queue>
#include <boost/unordered_map.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <cradle/io/web_io.hpp>
#include <cradle/disk_cache.hpp>
#include <cradle/thread_utilities.hpp>
#include <cradle/io/services/core_services.hpp>

// This file defines various shared internals for the background execution
// system. For more information about the system in general, see system.hpp
// and api.hpp.

namespace cradle {

struct background_job_execution_data
{
    background_job_execution_data(
        background_job_interface* job, int priority, bool hidden)
      : job(job), priority(priority),
        state(background_job_state::QUEUED), progress(0), cancel(false),
        hidden(hidden)
    {}
    ~background_job_execution_data()
    { delete job; }

    // the job itself, owned by this structure
    background_job_interface* job;

    // if this is true, the job won't be included in status reports
    bool hidden;

    int priority;

    // the current state of the job
    volatile background_job_state state;
    // the progress of the job
    volatile float progress;

    // if this is set, the job will be canceled next time it checks in
    volatile bool cancel;
};

typedef alia__shared_ptr<background_job_execution_data> background_job_ptr;

struct background_job_controller_data
{
    background_job_ptr job;
};

struct background_job_sorter
{
    bool operator()(
        background_job_ptr const& a,
        background_job_ptr const& b)
    { return a->priority > b->priority; }
};

struct background_job_canceled
{
};

struct background_job_check_in : check_in_interface
{
    background_job_check_in(background_job_ptr const& job)
      : job(job)
    {}
    void operator()()
    {
        if (job->cancel)
        {
            job->state = background_job_state::CANCELED;
            throw background_job_canceled();
        }
    }
    background_job_ptr job;
};

struct background_job_progress_reporter : progress_reporter_interface
{
    background_job_progress_reporter(background_job_ptr const& job)
      : job(job)
    {}
    void operator()(float progress)
    {
        job->progress = progress;
    }
    background_job_ptr job;
};

typedef std::priority_queue<background_job_ptr,std::vector<background_job_ptr>,
    background_job_sorter> job_priority_queue;

struct background_job_failure
{
    // the job that failed
    background_job_ptr job;
    // Was it a transient failure?
    // This indicates whether or not it's worth retrying the job.
    bool is_transient;
    // the associated error message
    string message;
};

struct background_job_queue
{
    // used to track changes in the queue
    local_identity version;
    // jobs that might be ready to run
    job_priority_queue jobs;
    // jobs that are waiting on dependencies
    job_priority_queue waiting_jobs;
    // counts how many times jobs have been woken up
    size_t wake_up_counter;
    // jobs that have failed
    std::list<background_job_failure> failed_jobs;
    // this provides info about all jobs in the queue
    std::map<background_job_execution_data*,background_job_info> job_info;
    // for controlling access to the job queue
    boost::mutex mutex;
    // for signalling when new jobs arrive
    boost::condition_variable cv;
    // # of threads currently monitoring this queue for work
    size_t n_idle_threads;
    // reported size of the queue
    // Internally, this is maintained as being the number of jobs in either
    // the jobs queue or the waiting_jobs queue that aren't marked as hidden.
    size_t reported_size;

    background_job_queue()
      : wake_up_counter(0)
      , n_idle_threads(0)
      , reported_size(0)
    {}
};

// Move all jobs in the waiting queue back to the main queue.
void wake_up_waiting_jobs(background_job_queue& queue);

// This is used for communication between the threads in a thread pool and
// outside entities.
struct background_thread_data_proxy
{
    // protects access to this data
    boost::mutex mutex;
    // the job currently being executed in this thread (if any)
    background_job_ptr active_job;
};

struct background_execution_thread
{
    background_execution_thread() {}

    template<class Function>
    background_execution_thread(Function function,
        alia__shared_ptr<background_thread_data_proxy> const& data_proxy)
      : thread(function), data_proxy(data_proxy)
    {}

    boost::thread thread;

    alia__shared_ptr<background_thread_data_proxy> data_proxy;
};

// A background_execution_pool combines a queue of jobs with a pool of threads
// that are intended to execute those jobs.
struct background_execution_pool
{
    alia__shared_ptr<background_job_queue> queue;
    std::vector<alia__shared_ptr<background_execution_thread> > threads;
};

struct background_cache;

struct background_cache_record
{
    // These remain constant for the life of the record.
    background_cache* owner_cache;
    owned_id key;

    // All of the following fields are protected by the cache mutex.
    // The only exception is that the state field can be polled for
    // informational purposes. However, before accessing any other fields
    // based on the value of state, you should acquire the mutex and recheck
    // state.

    volatile background_data_state state;
    volatile float progress;

    // This is a count of how many active pointers reference this data.
    // If this is 0, the data is just hanging around because it was recently
    // used, in which case eviction_list_iterator points to this record's
    // entry in the eviction list.
    unsigned ref_count;

    std::list<background_cache_record*>::iterator eviction_list_iterator;

    // If state is COMPUTING, this is the associated job.
    alia__shared_ptr<background_job_controller> job;

    // If state is READY, this is the associated data.
    untyped_immutable data;
};

typedef boost::unordered_map<id_interface const*,background_cache_record,
    id_interface_pointer_hash,id_interface_pointer_equality_test>
    cache_record_map;

struct cache_record_eviction_list
{
    std::list<background_cache_record*> records;
    size_t total_size;
    cache_record_eviction_list() : total_size(0) {}
};

struct background_cache : noncopyable
{
    cache_record_map records;
    cache_record_eviction_list eviction_list;
    boost::mutex mutex;
};

// Add a job for the background execution system to execute.
// If controller is 0, it's ignored.
// 'priority' controls the priority of the job. A higher number means higher
// priority. 0 is taken to be the default/neutral priority.
void add_background_job(
    background_execution_system& system,
    background_job_queue_type queue,
    background_job_controller* controller,
    background_job_interface* job,
    background_job_flag_set flags = NO_FLAGS,
    int priority = 0);

// SYNCHRONIZED QUEUE

// A synchronized_queue provides synchronized access (via a mutex) to a queue
// of items. It's designed to collect updates from background threads and
// allows the foreground thread to check for and process those updates in bulk.

template<class Item>
struct synchronized_queue
{
    std::queue<Item> items;
    boost::mutex mutex;
};

// Add an item to the given queue.
template<class Item>
void push(synchronized_queue<Item>& queue, Item const& item)
{
    boost::lock_guard<boost::mutex> lock(queue.mutex);
    queue.items.push(item);
}

// Clear out all the items in the given queue.
template<class Item>
void clear(synchronized_queue<Item>& queue)
{
    boost::lock_guard<boost::mutex> lock(queue.mutex);
    queue.items.clear();
}

// Process all the items in the given queue by running them through the
// provided handler function and then popping them from the queue.
template<class Item, class Handler>
void process_queue_items(synchronized_queue<Item>& queue, Handler const& handler)
{
    boost::lock_guard<boost::mutex> lock(queue.mutex);
    while (!queue.items.empty())
    {
        handler(queue.items.front());
        queue.items.pop();
    }
}

// MUTABLE DATA CACHING

// the data that the mutable cache system stores about an entity
struct mutable_cache_record
{
    // the ID of the entity
    owned_id entity_id;
    // the latest value associated with this record - This may be
    // uninitialized if there's no value yet.
    untyped_immutable value;
    // Is there currently a job dispatched to retrieve a value for this
    // entity?
    bool has_job;
    // a count of the number of mutable_entity_watches that are currently
    // referencing this entity
    unsigned watch_count;
    // a controller for job that's been dispatched to watch this value - Iff
    // :watch_count is non-zero, this should be valid.
    alia__shared_ptr<background_job_controller> watch_job;

    mutable_cache_record() : has_job(false), watch_count(0) {}
};

// mutable_cache_record_map associates entities (by ID) with their cache
// records.
typedef boost::unordered_map<id_interface const*,mutable_cache_record,
    id_interface_pointer_hash,id_interface_pointer_equality_test>
    mutable_cache_record_map;

// mutable_cache_update represents the update messages passed from background
// threads into the mutable cache system
struct mutable_cache_update
{
    // the entity that this message relates to
    owned_id entity_id;
    // the new value for the entity
    untyped_immutable value;
    // the source of the new value
    mutable_value_source source;
};

typedef synchronized_queue<mutable_cache_update> mutable_cache_update_queue;

struct mutable_cache
{
    // used to track the state of the mutable_cache - Any time the value of
    // any record within the cache is updated, this ID changes.
    // This is intended for observers to use in detecting changes that came in
    // from the background.
    local_identity update_id;
    // the cache records
    mutable_cache_record_map records;
    // used to track global refeshes on the cache - A global refresh invalidates watches
    // on the cache, so this ID allows watches to detect when they're outdated.
    local_identity refresh_id;
    // the update queue
    mutable_cache_update_queue updates;
};

// Add a watch on the specified mutable entity. - This is only meant to be
// called from within a mutable_entity_watch since that provides proper
// ownership semantics for the watch. This returns true if this is the first
// watch added for this entity.
bool
add_mutable_entity_watch(
    background_execution_system& system,
    id_interface const& entity_id);

// Set the job for watching a mutable entity.
void
set_mutable_entity_watch_job(
    background_execution_system& system,
    id_interface const& entity_id,
    background_job_interface* job);

// Remove a watch on the specified mutable entity. - This is only meant to be
// called from within a mutable_entity_watch since that provides proper
// ownership semantics for the watch.
void
remove_mutable_entity_watch(
    background_execution_system& system,
    id_interface const& entity_id);

// FRAMEWORK CONTEXT

struct background_context_request_data
{
    // This identifies the currently active context info.
    local_identity id;
    // The status of the most recent request.
    background_context_request_status status;
    // If the request succeeded, this is the context.
    framework_context context;
    // The controller for authentication jobs.
    background_job_controller job_controller;
    // This protects all of the above.
    boost::mutex mutex;
};

// GENERAL WEB REQUESTS

struct background_web_job : background_job_interface
{
    alia__shared_ptr<background_execution_system> system;
    web_connection* connection;
};

struct background_web_request_job : background_web_job
{
    background_web_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        id_interface const& id,
        web_request const& request,
        dynamic_type_interface const* result_interface);

    bool inputs_ready();
    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter);
    background_job_info get_info() const;

    owned_id id;
    web_request request;
    dynamic_type_interface const* result_interface;

    web_session_data session;
};

// AUTHENTICATION

struct background_authentication_request : background_web_job
{
    bool inputs_ready();
    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter);
    background_job_info get_info() const;

    owned_id id;
    web_authentication_credentials credentials;
    string framework_api_url;
};

void record_authentication_failure(background_execution_system& system,
    id_interface const& id, background_authentication_state failure_type,
    string const& message);

void record_authentication_success(background_execution_system& system,
    id_interface const& id, web_session_data const& session_data,
    string const& url);

// If a normal request fails because the session has been invalidated, this
// is called.
void invalidate_authentication_data(background_execution_system& system,
    background_authentication_state failure_type);

void get_authentication_result(background_execution_system& system,
    background_authentication_status* status,
    web_session_data* session);

struct background_authentication_data
{
    // This identifies the currently active authentication info.
    local_identity id;
    // The status of the most recent authentication request.
    background_authentication_status status;
    // If the request succeeded, these are the cookies it got.
    web_session_data session_data;
    // The URL used for the most recent successful request.
    string url;
    // The controller for authentication jobs.
    background_job_controller job_controller;
    // This protects all of the above.
    boost::mutex mutex;
};

// FRAMEWORK CONTEXT

struct background_context_request : background_web_job
{
    bool inputs_ready();
    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter);
    background_job_info get_info() const;

    owned_id id;
    web_session_data session;
    framework_usage_info framework;
    context_request_parameters parameters;
};

void record_context_request_failure(background_execution_system& system,
    id_interface const& id, background_context_request_state failure_type,
    string const& message);

void record_context_request_success(background_execution_system& system,
    id_interface const& id, web_session_data const& session_data);

// ACTUAL EXECUTION SYSTEM DEFINITION

struct background_execution_system_impl
{
    background_execution_pool pools[
        unsigned(background_job_queue_type::COUNT)];

    background_cache cache;

    alia__shared_ptr<cradle::disk_cache> disk_cache;

    background_authentication_data authentication;
    background_context_request_data context;

    web_io_system web_io;

    cradle::mutable_cache mutable_cache;
};

template<class ExecutionLoop>
void add_background_thread(background_execution_pool& pool)
{
    alia__shared_ptr<background_thread_data_proxy> data_proxy(
        new background_thread_data_proxy);
    ExecutionLoop fn(pool.queue, data_proxy);
    alia__shared_ptr<background_execution_thread> thread(
            new background_execution_thread(fn, data_proxy));
    pool.threads.push_back(thread);
    //lower_thread_priority(thread->thread);
}

struct background_job_execution_loop
{
    background_job_execution_loop(
        alia__shared_ptr<background_job_queue> const& queue,
        alia__shared_ptr<background_thread_data_proxy> const& data_proxy)
      : queue_(queue), data_proxy_(data_proxy)
    {}
    void operator()();
 private:
    alia__shared_ptr<background_job_queue> queue_;
    alia__shared_ptr<background_thread_data_proxy> data_proxy_;
};

void record_failure(background_job_execution_data& job, char const* msg,
    bool transient_failure);

struct web_request_processing_loop
{
    web_request_processing_loop(
        alia__shared_ptr<background_job_queue> const& queue,
        alia__shared_ptr<background_thread_data_proxy> const& data_proxy)
      : queue_(queue)
      , data_proxy_(data_proxy)
      , connection_(new web_connection)
    {}
    void operator()();
 private:
    alia__shared_ptr<background_job_queue> queue_;
    alia__shared_ptr<background_thread_data_proxy> data_proxy_;
    alia__shared_ptr<web_connection> connection_;
};

// This is called from background web jobs to get the session and context
// objects from the background execution system.
// If it returns true, both objects are available and have been written to the
// pointed-to locations. Otherwise, the objects aren't available yet.
bool get_session_and_context(background_execution_system& system,
    web_session_data* session, framework_context* context);

// Purge evicted items from the memory cache until it falls below a specified
// size (in MB).
void reduce_memory_cache_size(background_cache& cache, int desired_size);

}

#endif
