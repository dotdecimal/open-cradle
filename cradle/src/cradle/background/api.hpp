#ifndef CRADLE_BACKGROUND_API_HPP
#define CRADLE_BACKGROUND_API_HPP

#include <cradle/common.hpp>
#include <alia/id.hpp>
#include <typeinfo>
#include <cradle/io/services/calc_service.hpp>
#include <boost/function.hpp>

namespace cradle {

// This file defines the interface for data retrieval and job creation within
// the background execution system. For an introduction to the system, see
// system.hpp.

enum class background_job_queue_type
{
    CALCULATION = 0,
    DISK,
    // Web jobs are split into two queues according to whether they are
    // writing data to a web service or just reading data from it.
    WEB_READ,
    WEB_WRITE,
    // Jobs in the following queues are long-lived web jobs that may run
    // indefinitely but consume very little bandwidth, so they each get their
    // own thread.
    NOTIFICATION_WATCH,
    REMOTE_CALCULATION,
    COUNT,
};

// This provides general information about a job.
struct background_job_info
{
    string description;
};

// All jobs executed as part of a background system must implement this
// interface.
struct background_job_interface
{
    virtual ~background_job_interface() {}

    virtual void gather_inputs() {}

    virtual bool inputs_ready() { return true; }

    virtual void execute(
        check_in_interface& check_in,
        progress_reporter_interface& reporter) = 0;

    virtual background_job_info get_info() const = 0;
};

// A background_job_controller is used for monitoring and controlling the
// progress of a job.

enum background_job_state
{
    QUEUED, RUNNING, FINISHED, FAILED, CANCELED
};

struct background_job_controller_data;

struct background_job_controller : noncopyable
{
    background_job_controller() : data_(0) {}
    ~background_job_controller();

    void reset();

    bool is_valid() const { return data_ != 0; }

    // Cancel the job.
    void cancel();

    background_job_state state() const;

    // If state() is RUNNING, this is the job's progress.
    float progress() const;

    background_job_controller_data* data_;
};

struct background_job_status
{
    background_job_state state;
    // valid if state is RUNNING
    float progress;
};

background_job_status
get_background_job_status(background_job_controller& controller);

void static inline
swap(background_job_controller& a, background_job_controller& b)
{ swap(a.data_, b.data_); }

// background_data_ptr represents one's interest in the result of a particular
// job. It's initialized with a reference to the execution system and a key
// identifying the result. If there are already other parties interested in
// the result, the pointer will immediately pick up whatever progress has
// already been made in computing that result. Otherwise, the owner must
// create a new job to produce the result and associate it with the pointer.

struct background_cache_record;
struct background_execution_system;

// background_data_state represents the state of the data referenced by a
// background_data_ptr.
enum class background_data_state
{
    // The data is nowhere, so you should create a job to compute it.
    NOWHERE,
    // The data isn't available yet, but there's a job associated with it.
    COMPUTING,
    // The data is available.
    READY
};

struct background_data_status
{
    background_data_state state;
    // valid if state is COMPUTING
    float progress;
};

// untyped_background_data_ptr provides all of the functionality of
// background_data_ptr without compile-time knowledge of the data type.
class untyped_background_data_ptr
{
 public:
    untyped_background_data_ptr()
      : r_(0)
    {
        status_.state = background_data_state::NOWHERE;
        status_.progress = 0;
    }

    untyped_background_data_ptr(background_execution_system& system,
        id_interface const& key)
    { acquire(system, key); }

    untyped_background_data_ptr(untyped_background_data_ptr const& other)
    { copy(other); }

    untyped_background_data_ptr& operator=(
        untyped_background_data_ptr const& other)
    { reset(); copy(other); return *this; }

    ~untyped_background_data_ptr() { reset(); }

    void reset();

    void reset(background_execution_system& system, id_interface const& key);

    bool is_initialized() const { return r_ != 0; }

    background_data_status const& status() const { return status_; }

    background_data_state state() const { return status_.state; }

    float progress() const { return status_.progress; }

    bool is_ready() const
    { return this->state() == background_data_state::READY; }

    bool is_nowhere() const
    { return this->state() == background_data_state::NOWHERE; }

    bool is_computing() const
    { return this->state() == background_data_state::COMPUTING; }

    // Everything below here should only be called if the pointer is
    // initialized...

    // Update this pointer's view of the underlying record's state.
    void update();

    id_interface const& key() const { return key_.get(); }

    background_cache_record* record() const { return r_; }

    // These provide access to the actual data.
    // Only use this if state is READY.
    untyped_immutable const& data() const { return data_; }

    void swap(untyped_background_data_ptr& other);

 private:
    void copy(untyped_background_data_ptr const& other);

    void acquire(background_execution_system& system, id_interface const& key);

    owned_id key_;

    background_cache_record* r_;
    background_data_status status_;

    // This is a local copy of the data pointer. Actually acquiring this
    // pointer requires synchronization, but once it's acquired, it can be
    // used freely without synchronization.
    untyped_immutable data_;
};

void static inline
swap(untyped_background_data_ptr& a, untyped_background_data_ptr& b)
{ a.swap(b); }

// background_data_ptr<T> wraps untyped_background_data_ptr to provide access
// to background data of a known type.
template<class T>
class background_data_ptr
{
 public:
    background_data_ptr() : data_(0) {}

    background_data_ptr(untyped_background_data_ptr& untyped)
      : untyped_(untyped)
    { refresh_typed(); }

    background_data_ptr(
        background_execution_system& system, id_interface const& key)
    { reset(system, key); }

    void reset();

    void reset(background_execution_system& system, id_interface const& key)
    { untyped_.reset(system, key); refresh_typed(); }

    bool is_initialized() const { return untyped_.is_initialized(); }

    background_data_status const& status() const { return untyped_.status(); }

    background_data_state state() const { return untyped_.state(); }

    float progress() const { return untyped_.progress(); }

    bool is_ready() const { return state() == background_data_state::READY; }

    bool is_nowhere() const
    { return state() == background_data_state::NOWHERE; }

    bool is_computing() const
    { return state() == background_data_state::COMPUTING; }

    // Everything below here should only be called if the pointer is
    // initialized...

    // Update this pointer's view of the underlying record's state.
    void update();

    id_interface const& key() const { return untyped_.key(); }

    // Access the underlying untyped pointer.
    untyped_background_data_ptr const& untyped() const { return untyped_; }
    untyped_background_data_ptr& untyped() { return untyped_; }

    // When the underlying untyped pointer changes, this must be called to
    // update the typed data in response to those changes.
    void refresh_typed();

    // These provide access to the actual data.
    // Only use these if state is READY.
    T const& operator*() const { return *data_; }
    T const* operator->() const { return data_; }
    T const* data() const { return data_; }

    void swap(background_data_ptr& other);

 private:
    untyped_background_data_ptr untyped_;

    // typed pointer to the data
    T const* data_;
};

template<class T>
void swap(background_data_ptr<T>& a, background_data_ptr<T>& b)
{ a.swap(b); }

struct background_job_flag_tag {};
typedef flag_set<background_job_flag_tag> background_job_flag_set;
ALIA_DEFINE_FLAG(background_job, 0x01, BACKGROUND_JOB_HIDDEN)

// Add a job to the execution system's queue and associate it with the given
// data pointer.
// 'priority' controls the priority of the job. A higher number means higher
// priority. 0 is taken to be the default/neutral priority.
void add_untyped_background_job(
    untyped_background_data_ptr& ptr, background_execution_system& system,
    background_job_queue_type queue, background_job_interface* job,
    background_job_flag_set flags = NO_FLAGS, int priority = 0);

template<class Result>
void add_background_job(
    background_data_ptr<Result>& ptr, background_execution_system& system,
    background_job_queue_type queue, background_job_interface* job,
    background_job_flag_set flags = NO_FLAGS, int priority = 0);

// update_background_data_status() is used by background jobs to report
// progress made in computing individual results.
void update_background_data_progress(
    background_execution_system& system, id_interface const& key,
    float progress);

// set_cached_data() is used by background jobs to transmit the data that they
// produce into the background caching system.
void set_cached_data(
    background_execution_system& system, id_interface const& key,
    untyped_immutable const& value);

// Reset an immutable data entry.
// This must be called if the job associated with the data is canceled and ends up not
// retrieving the value. It clears out the record of that job having run and allows it to
// be restarted.
void
reset_cached_data(background_execution_system& system, id_interface const& key);

// swap_in_cached_data() is similar, but it consumes the passed value.
template<class T>
void swap_in_cached_data(
    background_execution_system& system, id_interface const& key, T& value);

struct web_request;

struct framework_context;

struct background_job_execution_data;

// Retry a failed background job.
void retry_background_job(
    background_execution_system& system,
    background_job_queue_type queue,
    background_job_execution_data* job_data);

// MUTABLE DATA CACHING

// This implements a basic system for shared caching of mutable data and
// allows for selective or global refreshing of that data.

// To use the system, application code must be able to provide IDs (via the
// alia::id_interface) that consistently and uniquely identify the mutable
// entities that they're attempting to cache. (Unlike most other uses of
// alia::id_interface, where IDs identify specific immutable values, in this
// case, the ID identifies an 'entity' that might be associated with many
// different values over time.)

// Note that there is no ownership tracking for mutable results and no garbage
// collection performed on the mutable cache. Unused entries are only cleared
// out when a global refresh occurs. Since mutable data tends to be very small
// (by design) this is considered adequate.

// There IS, however, ownership tracking of WATCHED mutable results.
// Continuous jobs can be dispatched specifically to watch a mutable result.
// Individual interest in watching results is specifically tracked and jobs
// are only kept around as long as there is interest in their results.

// Get the latest value associated with a mutable entity (identified by ID).
// This will return an uninitialized value if there is no associated value.
//
// Additionally, if there's no value and no job has been dispatched to retrieve
// one, this will call the designated callback to create a job to do so.
// (This interface currently assumes that if you're interested in the state of
// an entity, then you also know how to get that state.)
//
untyped_immutable
get_cached_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    boost::function<void()> const& dispatch_job);

// Request a refresh for a particular mutable entity.
// Flags for this function...
ALIA_DEFINE_FLAG_TYPE(mutable_refresh)
// If you intend to dispatch your own job immediately after calling this, you
// should specify this flag so that other interested parties don't dispatch
// their own jobs.
ALIA_DEFINE_FLAG(mutable_refresh, 0x1, MUTABLE_REFRESH_NO_JOB_NEEDED)
void
refresh_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    mutable_refresh_flag_set flags = NO_FLAGS);

// Update the value associated wtih a mutable entity.
enum class mutable_value_source
{
    RETRIEVAL,
    WATCH
};
void set_mutable_value(
    background_execution_system& system, id_interface const& entity_id,
    untyped_immutable const& new_value, mutable_value_source source);

// Get the ID corresponding to the latest update of the mutable data cache.
value_id_by_reference<local_id>
get_mutable_cache_update_id(background_execution_system& system);

// This represents one's interest in watching a mutable entity's value.
struct mutable_entity_watch
{
    // structors and assignment operators
    mutable_entity_watch() {}
    ~mutable_entity_watch() { reset(); }
    mutable_entity_watch(mutable_entity_watch const& other);
    mutable_entity_watch(mutable_entity_watch&& other);
    mutable_entity_watch&
    operator=(mutable_entity_watch const& other);
    mutable_entity_watch&
    operator=(mutable_entity_watch&& other);

    // Start watching an entity. The caller must supply a callback to create
    // the job to watch the entity in case this is the first watcher.
    void
    watch(
        alia__shared_ptr<background_execution_system> const& system,
        id_interface const& entity_id,
        boost::function<background_job_interface*()> const& job_creator);

    // Stop watching the entity. - If no entity was being watched, this is a
    // noop.
    void reset();

    // Is this currently watching anything?
    bool is_active() const;

    // Get the ID of the entity that this is watching. - This should only be
    // called if is_active() returns true.
    id_interface const& entity_id() const
    {
        assert(this->is_active());
        return entity_id_.get();
    }

 private:
    bool
    watch(
        alia__shared_ptr<background_execution_system> const& system,
        id_interface const& entity_id);

    alia__shared_ptr<background_execution_system> system_;
    owned_id refresh_id_;
    owned_id entity_id_;
};

}

#include <cradle/background/api.ipp>

#endif
