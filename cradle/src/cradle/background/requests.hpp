#ifndef CRADLE_BACKGROUND_REQUESTS_HPP
#define CRADLE_BACKGROUND_REQUESTS_HPP

#include <cradle/background/api.hpp>
#include <cradle/io/services/core_services.hpp>

// This file defines a system for resolving untyped_requests (see
// cradle/common.hpp) in the background.

namespace cradle {

api(struct internal)
struct immutable_response
{
    string id;
};

struct background_execution_system;
struct background_request_resolution_data;
struct background_request_system_data;

struct background_request_system
{
    background_request_system() : data_(0) {}
    ~background_request_system();

    background_request_system_data* data_;
};

void initialize_background_request_system(
    background_request_system& request_system,
    alia__shared_ptr<background_execution_system> const& execution_system);

// Issue new requests in the system to the background threads.
std::vector<untyped_request>
issue_new_requests(background_request_system& request_system);

// Gather updates from the background into the system.
void gather_updates(background_request_system& request_system);

// Clear updates that haven't been claimed yet.
void clear_updates(background_request_system& request_system);

enum class request_preresolution_state
{
    // preresolution hasn't started
    UNINITIALIZED,
    // preresolution is in progress
    RESOLVING,
    // preresolution finished and was successful
    RESOLVED,
};

struct background_request_preresolution_data
{
    background_request_preresolution_data()
     : state(request_preresolution_state::UNINITIALIZED)
    {}

    request_preresolution_state state;
    std::vector<untyped_request> subrequests;
    std::vector<background_request_resolution_data> subrequest_resolutions;
    untyped_request preresolved_request;
};

// background_request_resolution_data defines the data nececssary to resolve
// an individual request in the background.
struct background_request_resolution_data
{
    // used in cases where we first have to preresolve subrequests
    background_request_preresolution_data preresolution;

    // used for resolving the actual request (after preresolution)
    // Note that different types of requests require different types of data to
    // enable their resolution, so this structure just stores a cradle::any.
    // During resolution, once the specific type of request if determined, the
    // needed data (if any) is stored within.
    any resolution;
};

enum class background_request_interest_type
{
    OBJECTIFIED_FORM,
    RESULT
};

// background_request_ptr represents one's interest in a request's result or
// objectified form.
// The 'objectified form' of a request is the equivalent request with all the
// remote requests replaced by remote object references.
//
// It's initialized with a reference to the request system and the request.
// The request system must remain alive as long as the pointer is alive.
//
// This uses the memory and disk caches where appropriate, so if multiple
// parties are interested in the same result and caching is considered
// worthwhile for that result, they'll share it.
//
struct background_request_ptr : noncopyable
{
    background_request_ptr() : system_(0), is_resolved_(false) {}

    ~background_request_ptr() { reset(); }

    // Reset the pointer to a new request.
    //
    // The requester ID must be unique within the request system, but it
    // doesn't have to be one-to-one with the request.
    //
    // :interest specifies whether you're interested in the objectified form
    // or the actual result.
    //
    // If :custom_context_id is given, it will be used as the Thinknode
    // context ID instead of the default one associated with :system.
    //
    void reset(
        background_request_system& system,
        id_interface const& requester_id,
        framework_context const& context,
        untyped_request const& request,
        background_request_interest_type interest);

    // a constructor that takes the same arguments as reset(), above
    background_request_ptr(
        background_request_system& system,
        id_interface const& id,
        framework_context const& context,
        untyped_request const& request,
        background_request_interest_type interest)
    { reset(system, id, context, request, interest); }

    // Reset to a default constructed pointer (i.e., referencing nothing).
    void reset();

    // Swap with another pointer.
    void swap_with(background_request_ptr& other);

    // Is the pointer initialized with some request?
    bool is_initialized() const { return system_ != 0; }

    // The rest of this interface should only be used on initialized pointers.

    // Get the ID that the requester provided.
    id_interface const& requester_id() const { return requester_id_.get(); }

    // Is the requester interested in the result or the objectified form?
    background_request_interest_type interest() const { return interest_; }

    // Get the framework context associated with this request.
    optional<framework_context> const& context() const
    { return context_; }

    // Has the result (or the objectified form, whichever is relevant) been
    // resolved?
    bool is_resolved() const { return is_resolved_; }

    // If is_resolved() returns true and interest() is RESULT, this will give
    // the resolved result.
    untyped_immutable const& result() const { return result_; }

    // If is_resolved() returns true and interest() is OBJECTIFIED_FORM, this
    // will give the objectified form.
    optional<untyped_request> const& objectified_form() const
    { return objectified_form_; }

    // Update the pointer's status to reflect progress made in the background.
    void update();

 private:
    background_request_system* system_;
    framework_context context_;
    owned_id requester_id_;
    background_request_interest_type interest_;
    bool is_resolved_;
    untyped_immutable result_;
    background_job_controller controller_;
    optional<untyped_request> objectified_form_;
};

void static inline
swap(background_request_ptr& a, background_request_ptr& b)
{ a.swap_with(b); }

// request_objects are proper CRADLE types that mirror the request type.
// These can be used for external representation/identification.

struct function_request_object;
struct field_request_object;
struct union_request_object;

api(union internal)
union request_object
{
    value immediate;
    function_request_object function;
    std::vector<request_object> array;
    std::map<string,request_object> structure;
    field_request_object field;
    union_request_object union_;
    request_object some;
    request_object required;
    request_object isolated;
    request_object remote;
    string object;
    string immutable;
    request_object meta;
};

api(struct internal)
struct function_request_object
{
    string account, app, function;
    std::vector<request_object> args;
    omissible<int> level;
};

api(struct internal)
struct field_request_object
{
    request_object record;
    string field;
};

api(struct internal)
struct union_request_object
{
    request_object member_request;
    string member_name;
};

api(struct internal)
struct resolved_immutable_id
{
    string id;
};

request_object
as_request_object(untyped_request const& request);

// META-LIKE REQUESTS (a.k.a., calculation requests by ID)
//
// This is a limited form of Thinknode's meta request functionality that allows
// dry runs to be done.
//
// The caller supplies the ID of a request generator calculation, similar to
// what would be passed into a Thinknode meta calculation. (It's not exactly
// the same because this generator is expected to directly return an actual
// calculation request, whereas Thinknode would allow it to return another meta
// request.)
//
// This will wait for that generator calculation to finish and then issue its
// result as another calculation.
//
// The dry_run flag determines whether or not the calculation is issued as a
// dry run. In either case, the result is an optional calculation ID.
// (If the calculation is not a dry run, the result should always have an ID.)
//
// This takes in a pointer to the execution system as a parameter so that it
// can use the disk cache.
//
optional<string>
perform_meta_request_by_id(
    alia__shared_ptr<background_execution_system> const& bg,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    string const& generator_id,
    bool dry_run);

// REQUEST BY COMPOSITION
//
// This is another alternative to Thinknode's meta request functionality which
// uses a locally generated request but tries to be as efficient as possible
// about submitting it to Thinknode. It's more responsive than other methods
// in cases where the client is repeatedly submitting many similar requests to
// Thinknode.
//
// In this method, the caller supplies a Thinknode request that was generated
// using a composition_cache and as_compact_thinknode_request. These requests
// contain 'let' variables that represent repeated subrequests, so rather than
// submitting the entire request, these subrequests are submitted individually
// and their calculation IDs are substituted into higher-level requests in
// place of the 'variable' requests used to reference them. This method has
// the advantage that it can leverage memory and disk caching to avoid
// resubmitting subrequests that have previously been submitted.
//
// The return value is a structure that includes not only the ID of the
// calculation but also information that may be useful for tracking the
// progress of the calculation tree.
//
// The dry_run flag determines whether or not the calculation is issued as a
// dry run. The return value is optional because a dry run might return none
// (the calculation doesn't exist), but a regular submission will always
// return a value.
//
// This takes in a pointer to the execution system as a parameter so that it
// can use the memory and disk caches.

api(struct internal)
struct reported_calculation_info
{
    // the Thinknode ID of the calculation
    string id;
    // a label for the calculation - Currently, this is just the function name.
    string label;
};

api(struct internal)
struct let_calculation_submission_info
{
    // the ID of the top-level calculation
    string main_calc_id;
    // info on any subcalculations whose progress we're interested in
    std::vector<reported_calculation_info> reported_subcalcs;
    // IDs of any other subcalculations
    std::vector<string> other_subcalc_ids;
};

optional<let_calculation_submission_info>
submit_let_calculation_request(
    alia__shared_ptr<background_execution_system> const& bg,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    augmented_calculation_request const& request,
    bool dry_run);

}

#endif
