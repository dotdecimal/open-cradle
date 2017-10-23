#ifndef CRADLE_BACKGROUND_SYSTEM_HPP
#define CRADLE_BACKGROUND_SYSTEM_HPP

#include <cradle/common.hpp>
#include <cradle/io/web_io.hpp>
#include <list>
#include <alia/data_graph.hpp>
#include <cradle/background/api.hpp>

// A background_execution_system is a flexible means of executing jobs in
// background threads. It also provides a cache so that the results of those
// jobs can be remembered and shared (in memory) if multiple parties are
// interested in them.
//
// It supports three different types of jobs: pure calculations, web queries,
// and disk jobs.
//
// For pure calculations, it maintains a pool of worker threads (one for each
// processor core in the system). Individual jobs are assumed to be
// single-threaded, so each worker thread simply grabs jobs off the queue and
// executes them one at a time.
//
// For web queries, it's assumed that more concurrency is always better, so
// the system allocates threads as needed to ensure that all pending queries
// can execute immediately.
//
// A small, fixed number of threads service disk jobs, as it's assumed that
// they'll mostly be contending for the same resource.
//
// In all cases, jobs are allowed to be dependent on the results of other
// jobs (even different types of jobs). A job's execution is deferred until
// all its dependencies are ready.

// This file provides the interface for creating and managing a
// background_system as a whole. The API for data retrieval and job creation
// can be found in api.hpp.

namespace cradle {

struct background_execution_system_impl;

struct background_execution_system : noncopyable
{
    background_execution_system();
    ~background_execution_system();

    background_execution_system_impl* impl_;
};

struct background_job_execution_data;

// JOBS MANAGEMENT / STATUS INTERFACE

// Clear all the jobs pending execution in the system.
void clear_pending_jobs(background_execution_system& system);

// Clear all the jobs in the system, including those that are currently
// executing.
void clear_all_jobs(background_execution_system& system);

// Clears out any jobs in the system that have been canceled.
void clear_canceled_jobs(background_execution_system& system);

struct background_job_failure_report
{
    // the job that failed
    background_job_execution_data* job;
    // the associated error message
    string message;
};

struct background_execution_pool_status
{
    size_t queued_job_count, thread_count, idle_thread_count;
    std::list<background_job_failure_report> transient_failures;
    std::map<background_job_execution_data*,background_job_info> job_info;
};

size_t static inline
get_active_thread_count(background_execution_pool_status const& status)
{ return status.thread_count - status.idle_thread_count; }

size_t static inline
get_total_job_count(background_execution_pool_status const& status)
{
    return get_active_thread_count(status) + status.queued_job_count +
        status.transient_failures.size();
}

struct background_execution_system_status
{
    keyed_data<background_execution_pool_status> pools[
        unsigned(background_job_queue_type::COUNT)];
};

// Update a view of the status of a background execution system.
void update_status(background_execution_system_status& status,
    background_execution_system& system);

// Get a list of jobs that have failed permanently since the last check.
// (This also clears the system's internal list.)
std::list<background_job_failure_report>
get_permanent_failures(background_execution_system& system);

// Get a snapshot of the memory cache contents.
struct memory_cache_entry_info
{
    raw_type_info type;
    size_t data_size; // in bytes
};
struct memory_cache_snapshot
{
    // cache entries that are currently in use
    std::vector<memory_cache_entry_info> in_use;
    // cache entries that are no longer in use and will be evicted when
    // necessary
    std::vector<memory_cache_entry_info> pending_eviction;
};
memory_cache_snapshot
get_memory_cache_snapshot(background_execution_system& system);

// DISK CACHE INTERFACE

struct disk_cache;

// Associate a disk cache with the given background execution system.
void set_disk_cache(background_execution_system& system,
    alia__shared_ptr<disk_cache> const& disk_cache);

// Get the disk cache associated with a background execution system.
alia__shared_ptr<disk_cache> const&
get_disk_cache(background_execution_system& system);

// AUTHENTICATION MANAGEMENT INTERFACE

// Set the authentication info for web requests.
// This results in an authentication request being made.
void set_authentication_info(
    alia__shared_ptr<background_execution_system> const& system,
    string const& framework_api_url,
    web_authentication_credentials const& credentials);

void set_authentication_token(
    alia__shared_ptr<background_execution_system> const& system,
    string const& token);

enum class background_authentication_state
{
    IN_PROGRESS,
    SUCCEEDED,
    NO_CREDENTIALS,
    FAILED_TO_CONNECT,
    INVALID_CREDENTIALS,
    SESSION_EXPIRED,
    SESSION_TIMED_OUT
};

bool is_failure(background_authentication_state state);

// Get a user-friendly description of the given state.
char const*
get_description(background_authentication_state state);

struct background_authentication_status
{
    background_authentication_state state;
    // If state is FAILED_TO_CONNECT, this is the error message.
    string message;

    background_authentication_status() {}
    background_authentication_status(
        background_authentication_state state,
        string const& message = "")
      : state(state), message(message)
    {}
};

// Get the result of the last authentication request.
background_authentication_status
get_authentication_status(background_execution_system& system);

// Clear out the authentication info associated with the given system.
void clear_authentication_info(background_execution_system& system);

struct framework_usage_info;
struct context_request_parameters;

// REALM / CONTEXT MANAGEMENT INTERFACE

// Set the info needed to request the context for this system.
// This will also cause a background job to be invoked to request the actual
// context ID.
void set_context_request_parameters(
    alia__shared_ptr<background_execution_system> const& system,
    framework_usage_info const& framework,
    context_request_parameters const& parameters);

enum class background_context_request_state
{
    NO_REQUEST, // no request has been made, or its results were cleared
    IN_PROGRESS,
    SUCCEEDED,
    FAILED_TO_CONNECT,
    NOT_FOUND,
    INVALID_RESPONSE
};

bool is_failure(background_context_request_state state);

// Get a user-friendly description of the given state.
char const*
get_description(background_context_request_state state);

struct background_context_request_status
{
    background_context_request_state state;
    // If state is FAILED_TO_CONNECT, this is the error message.
    string message;

    background_context_request_status() {}
    background_context_request_status(
        background_context_request_state state,
        string const& message = "")
      : state(state), message(message)
    {}
};

// Get the status of the last context request.
background_context_request_status
get_context_request_status(background_execution_system& system);

// Directly set the framework context response associated with this system.
void set_framework_context(background_execution_system& system,
    framework_context const& context);

// Clear out any framework context that was associated with this system.
void clear_framework_context(background_execution_system& system);

// Clear the memory cache for this system.
void clear_memory_cache(background_execution_system& system);

// Purge evicted items from the memory cache until it falls below a specified
// size (in MB).
void reduce_memory_cache_size(background_execution_system& system, int desired_size);

// Get the service framework context associated with this system.
void get_context_request_result(
    background_execution_system& system,
    background_context_request_status* status,
    framework_context* response);

// MUTABLE DATA CACHING

// Process updates received by the mutable cache system.
void process_mutable_cache_updates(background_execution_system& system);

// Clear out the mutable data cache.
// This will perform a global refresh on the mutable data system.
void clear_mutable_data_cache(background_execution_system& system);

}

#endif
