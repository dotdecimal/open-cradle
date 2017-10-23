#include <cradle/background/requests.hpp>

#include <json/json.h>
#include <queue>

#include <boost/algorithm/string.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include <cradle/background/api.hpp>
#include <cradle/background/internals.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/io/file.hpp>
#include <cradle/io/services/calc_internals.hpp>

namespace cradle {

void static
reset(background_request_resolution_data& resolution)
{
    resolution.preresolution = background_request_preresolution_data();
    resolution.resolution = cradle::any();
}

// Cast the given resolution data to the given type.
template<class Data>
static Data&
cast_resolution_data(background_request_resolution_data& data)
{
    // There's no separate initialization process, so the first time this is
    // called, we'll see uninitialized data.
    if (!get_value_pointer(data.resolution))
    {
        // Initialize it.
        data.resolution = Data();
    }
    return unsafe_any_cast<Data>(data.resolution);
}

request_object static
thinknode_request_as_request_object(calculation_request const& request)
{
    switch (request.type)
    {
     case calculation_request_type::ARRAY:
        return
            make_request_object_with_array(
                map(thinknode_request_as_request_object,
                    as_array(request).items));
     case calculation_request_type::FUNCTION:
      {
        auto const& fn = as_function(request);
        return
            make_request_object_with_function(
                function_request_object(
                    fn.account,
                    fn.app,
                    fn.name,
                    map(thinknode_request_as_request_object, fn.args),
                    fn.level));
      }
     case calculation_request_type::OBJECT:
        return
            make_request_object_with_structure(
                map(thinknode_request_as_request_object,
                    as_object(request).properties));
     case calculation_request_type::PROPERTY:
        return
            make_request_object_with_field(
                field_request_object(
                    thinknode_request_as_request_object(
                        as_property(request).object),
                    cast<string>(as_value(as_property(request).field))));
     case calculation_request_type::REFERENCE:
        return make_request_object_with_object(as_reference(request));
     case calculation_request_type::VALUE:
        return make_request_object_with_immediate(as_value(request));
     case calculation_request_type::META:
        return
            make_request_object_with_meta(
                thinknode_request_as_request_object(
                    as_meta(request).generator));
     default:
        assert(0);
        throw exception("internal error: unhandled Thinknode request type");
    }
}

request_object
as_request_object(untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return
            make_request_object_with_immediate(
                as_immediate(request).ptr->as_value());
     case request_type::FUNCTION:
      {
        auto const& spec = as_function(request);
        return
            make_request_object_with_function(
                function_request_object(
                    spec.function->implementation_info.account_id,
                    spec.function->implementation_info.app_id,
                    spec.function->api_info.name,
                    map(as_request_object, spec.args),
                    spec.function->implementation_info.level));
      }
     case request_type::REMOTE_CALCULATION:
        return
            make_request_object_with_remote(
                as_request_object(as_remote_calc(request)));
     case request_type::META:
        return
            make_request_object_with_meta(
                as_request_object(as_meta(request)));
     case request_type::OBJECT:
         return make_request_object_with_object(as_object(request));
     case request_type::IMMUTABLE:
        return make_request_object_with_immutable(as_immutable(request));
     case request_type::ARRAY:
        return
            make_request_object_with_array(
                map(as_request_object, as_array(request)));
     case request_type::STRUCTURE:
        return
            make_request_object_with_structure(
                map(as_request_object, as_structure(request).fields));
     case request_type::PROPERTY:
        return
            make_request_object_with_field(
                field_request_object(
                    as_request_object(as_property(request).record),
                    as_property(request).field));
     case request_type::UNION:
        return
            make_request_object_with_union_(
                union_request_object(
                    as_request_object(as_union(request).member_request),
                    as_union(request).member_name));
     case request_type::SOME:
        return
            make_request_object_with_some(
                as_request_object(as_some(request).value));
     case request_type::REQUIRED:
        return
            make_request_object_with_required(
                as_request_object(as_required(request).optional_value));
     case request_type::ISOLATED:
        return
            make_request_object_with_isolated(
                as_request_object(as_isolated(request)));
     default:
        assert(0);
        throw exception("internal error: invalid request type");
    }
}

// Check if the given request contains any remote calculations.
bool static
contains_remote_calculations(untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return false;
     case request_type::FUNCTION:
        for (auto const& arg : as_function(request).args)
        {
            if (contains_remote_calculations(arg))
                return true;
        }
        return false;
     case request_type::REMOTE_CALCULATION:
        return true;
     case request_type::META:
        return true;
     case request_type::OBJECT:
        return false;
     case request_type::IMMUTABLE:
        return false;
     case request_type::ARRAY:
        for (auto const& item : as_array(request))
        {
            if (contains_remote_calculations(item))
                return true;
        }
        return false;
     case request_type::STRUCTURE:
        for (auto const& field : as_structure(request).fields)
        {
            if (contains_remote_calculations(field.second))
                return true;
        }
        return false;
     case request_type::PROPERTY:
        return
            contains_remote_calculations(
                as_property(request).record);
     case request_type::UNION:
        return
            contains_remote_calculations(
                as_union(request).member_request);
     case request_type::SOME:
        return
            contains_remote_calculations(
                as_some(request).value);
     case request_type::REQUIRED:
        return
            contains_remote_calculations(
                as_required(request).optional_value);
     case request_type::ISOLATED:
        return contains_remote_calculations(as_isolated(request));
     default:
        assert(0);
        return false;
    }
}

// ID INTERFACE

struct request_id : id_interface
{
 public:
    request_id(
        untyped_request const& request)
      : request_(request)
    {}

    id_interface* clone() const
    { return new request_id(request_); }

    void deep_copy(id_interface* copy) const
    { *static_cast<request_id*>(copy) = *this; }

    bool equals(id_interface const& other) const
    { return request_ == static_cast<request_id const&>(other).request_; }

    bool less_than(id_interface const& other) const
    { return false; }

    void stream(std::ostream& o) const
    { o << as_request_object(request_); }

    size_t hash() const
    { return invoke_hash(request_); }

 private:
    untyped_request request_;
};

// Make a request ID.
request_id static
make_request_id(untyped_request const& request)
{
    return request_id(request);
}

// DISK UTILITIES

void static
write_to_disk_cache(
    background_execution_system& bg,
    framework_context const& context,
    request_object const& object,
    value const& v)
{
    try
    {
        auto key =
            context.context_id + "/" +
            value_to_base64_string(to_value(object));
        auto& cache = *get_disk_cache(bg);
        int64_t entry = initiate_insert(cache, key);
        uint32_t crc;
        write_value_file(get_path_for_id(cache, entry), v, &crc);
        finish_insert(cache, entry, crc);
    }
    catch (...)
    {
        // If writing to the disk cache fails, it doesn't really matter.
    }
}

struct untyped_disk_read_job : background_job_interface
{
    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        uint32_t file_crc;
        value v;
        read_value_file(&v, path, &file_crc);
        if (file_crc != expected_crc)
            throw crc_error();
        set_cached_data(*bg, id.get(),
            result_interface->value_to_immutable(v));
    }
    background_job_info get_info() const
    {
        background_job_info info;
        info.description = to_string(path);
        return info;
    }
    alia__shared_ptr<background_execution_system> bg;
    dynamic_type_interface const* result_interface;
    owned_id id;
    file_path path;
    uint32_t expected_crc;
};

bool static
is_failed_disk_read(untyped_background_data_ptr& ptr)
{
    if (!ptr.is_computing())
        return false;

    auto* record = ptr.record();
    boost::lock_guard<boost::mutex> lock(record->owner_cache->mutex);
    auto* job = record->job.get();
    return job->state() == background_job_state::FAILED &&
        dynamic_cast<untyped_disk_read_job*>(job->data_->job->job);
}

// RESOLUTION UTILITIES

void static
initialize_if_needed(
    alia__shared_ptr<background_execution_system> const& bg,
    untyped_background_data_ptr& ptr,
    untyped_request const& request)
{
    if (!ptr.is_initialized())
        ptr.reset(*bg, make_request_id(request));
}

// Update a single background_data_ptr. If this returns true, the caller
// should start a job to actually generate the associated data.
// The main purpose of this is to provide disk cache lookup.
bool static
update_background_pointer(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    dynamic_type_interface const* result_interface,
    untyped_background_data_ptr& ptr,
    boost::function<request_object ()> const& object_generator,
    bool use_disk_cache = true)
{
    // If the result's not available, try loading it from the disk cache.
    if (ptr.is_nowhere() && use_disk_cache)
    {
        auto key = context.context_id + "/" +
            value_to_base64_string(to_value(object_generator()));

        auto& disk_cache = *get_disk_cache(*bg);
        int64_t entry;
        uint32_t entry_crc;
        if (entry_exists(disk_cache, key, &entry, &entry_crc))
        {
            record_usage(disk_cache, entry);
            untyped_disk_read_job* job = new untyped_disk_read_job;
            job->bg = bg;
            job->result_interface = result_interface;
            job->id.store(ptr.key());
            job->path = get_path_for_id(disk_cache, entry);
            job->expected_crc = entry_crc;
            add_untyped_background_job(ptr, *bg,
                background_job_queue_type::DISK, job);
        }
    }

    // If there's no associated job, start one.
    // Also start one if the previous one failed and it was trying to
    // retrieve the data from the disk cache.
    if (ptr.is_nowhere() || is_failed_disk_read(ptr))
        return true;

    ptr.update();

    return false;
}

bool static
is_trivial(untyped_request const& request);

// RESOLUTION INTERFACE - The following functions provide the basic interface
// for resolving a request.

// Note that not all requests require resolution data to be resolved.
// This is why resolution data is always passed by pointer.
// If is_trivial returns true for a request, then no data is required.
// If this is the case, you can pass a null pointer.

// Update (and initialize if necessary) a request resolution.
// Iff foreground_only is true, this will only attempt foreground operations to
// resolve the request. (If the request requires background resolution, it will
// simply not get resolved.)
void static
update_resolution(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data* resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest);

// Has the
bool static
result_is_resolved(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// Get the result of the given request.
untyped_immutable static
get_result(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// For requests that need to be preresolved to a different type of request,
// this determines if the preresolved result is ready.
bool static
is_preresolved(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// For requests that need to be preresolved to a different type of request,
// this gets the preresolved result.
untyped_request static
get_preresolved_result(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// Is the objectification process complete for this request?
bool static
objectification_complete(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// Get the objectified form of the request.
// Only call this if objectification_complete() returns true.
untyped_request static
get_objectified_form(
    background_request_resolution_data* resolution,
    untyped_request const& request);

// LIST RESOLUTION - utilities for resolving lists of requests

typedef std::vector<background_request_resolution_data> list_resolution_data;

// Update (and initialize if necessary) a list of request resolutions.
void static
update_resolution_list(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests,
    bool foreground_only,
    background_request_interest_type interest)
{
    size_t n_requests = requests.size();
    if (resolutions.empty())
        resolutions.resize(n_requests);
    for (size_t i = 0; i != n_requests; ++i)
    {
        update_resolution(bg, context, &resolutions[i], requests[i],
            foreground_only, interest);
    }
}

bool static
result_is_resolved(
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    for (size_t i = 0; i != n_requests; ++i)
    {
        if (!result_is_resolved(&resolutions[i], requests[i]))
            return false;
    }
    return true;
}

// Get the results of a list of requests.
std::vector<untyped_immutable> static
get_request_list_results(
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    std::vector<untyped_immutable> results(n_requests);
    for (size_t i = 0; i != n_requests; ++i)
        results[i] = get_result(&resolutions[i], requests[i]);
    return results;
}

// Determine if a list resolution is at the point where the objectified form
// of the list is available.
bool static
list_objectification_complete(
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    for (size_t i = 0; i != n_requests; ++i)
    {
        if (!objectification_complete(&resolutions[i], requests[i]))
            return false;
    }
    return true;
}

// Get the objectified form of a list.
std::vector<untyped_request> static
get_list_objectified_form(
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    std::vector<untyped_request> objectified(n_requests);
    for (size_t i = 0; i != n_requests; ++i)
        objectified[i] = get_objectified_form(&resolutions[i], requests[i]);
    return objectified;
}

// Count the number of non-trivial requests in a list.
size_t static
count_nontrivial_requests(std::vector<untyped_request> const& requests)
{
    size_t count = 0;
    for (auto const& request : requests)
    {
        if (!is_trivial(request))
            ++count;
    }
    return count;
}

// MAP RESOLUTION - utilities for resolving a map of requests

typedef std::vector<background_request_resolution_data> map_resolution_data;

// Update (and initialize if necessary) a list of request resolutions.
void static
update_resolution_map(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    map_resolution_data& resolutions,
    std::map<string,untyped_request> const& requests,
    bool foreground_only,
    background_request_interest_type interest)
{
    size_t n_requests = requests.size();
    if (resolutions.empty())
        resolutions.resize(n_requests);
    size_t i = 0;
    for (auto const& request : requests)
    {
        update_resolution(bg, context, &resolutions[i++], request.second,
            foreground_only, interest);
    }
}

bool static
result_is_resolved(
    map_resolution_data& resolutions,
    std::map<string,untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    size_t i = 0;
    for (auto const& request : requests)
    {
        if (!result_is_resolved(&resolutions[i++], request.second))
            return false;
    }
    return true;
}

// Get the results of a list of requests.
std::map<string,untyped_immutable> static
get_request_map_results(
    map_resolution_data& resolutions,
    std::map<string,untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    std::map<string,untyped_immutable> results;
    size_t i = 0;
    for (auto const& request : requests)
    {
        results[request.first] =
            get_result(&resolutions[i++], request.second);
    }
    return results;
}

// Determine if a list resolution is at the point where the objectified form
// of the map is available.
bool static
map_objectification_complete(
    map_resolution_data& resolutions,
    std::map<string,untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    size_t i = 0;
    for (auto const& request : requests)
    {
        if (!objectification_complete(&resolutions[i++], request.second))
            return false;
    }
    return true;
}

// Get the objectified form of a map.
std::map<string,untyped_request> static
get_map_objectified_form(
    map_resolution_data& resolutions,
    std::map<string,untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    std::map<string,untyped_request> objectified;
    size_t i = 0;
    for (auto const& request : requests)
    {
        objectified[request.first] =
            get_objectified_form(&resolutions[i++], request.second);
    }
    return objectified;
}

// Count the number of non-trivial requests in a map.
size_t static
count_nontrivial_requests(std::map<string,untyped_request> const& requests)
{
    size_t count = 0;
    for (auto const& request : requests)
    {
        if (!is_trivial(request.second))
            ++count;
    }
    return count;
}

// LOCAL CALCULATIONS - resolving a local calculation

// a job for computing the result of a local calculation
struct local_calculation_job : background_job_interface
{
    local_calculation_job(
        alia__shared_ptr<background_execution_system> const& bg,
        framework_context const& context,
        untyped_request const& request)
      : bg_(bg)
      , context_(context)
      , request_(request)
    {}

    void gather_inputs()
    {
        update_resolution_list(bg_, context_, arg_resolutions_,
            as_function(request_).args, false,
            background_request_interest_type::RESULT);
    }

    bool inputs_ready()
    {
        return result_is_resolved(arg_resolutions_, as_function(request_).args);
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        auto const& calc = as_function(request_);

        // Execute the function.
        auto result =
            calc.function->execute(check_in, reporter,
                get_request_list_results(arg_resolutions_, calc.args));

        // Write the result to the memory cache.
        set_cached_data(*bg_, make_request_id(request_), result);

        // Also cache the result to disk if desired.
        if (is_disk_cached(*calc.function))
        {
            write_to_disk_cache(*bg_, context_, as_request_object(request_),
                request_.result_interface->immutable_to_value(result));
        }
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = as_function(request_).function->api_info.name;
        return info;
    }

 private:
    alia__shared_ptr<background_execution_system> bg_;
    framework_context context_;
    untyped_request request_;
    list_resolution_data arg_resolutions_;
};

size_t static
count_nontrivial_args(function_request_info const& calc)
{
    return count_nontrivial_requests(calc.args);
}

bool static
is_function_trivial(function_request_info const& calc)
{
    return is_trivial(*calc.function);
}

bool static
is_calc_trivial(function_request_info const& calc)
{
    return is_function_trivial(calc) && count_nontrivial_args(calc) == 0;
}

bool static
is_foreground_calc(function_request_info const& calc)
{
    return is_function_trivial(calc) || calc.force_foreground_resolution;
}

void static
update_local_calculation(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data* resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    auto const& calc = as_function(request);

    // Since there's no real work to be done (at this level) to generate the
    // objectified form, we can always do that in the foreground.
    if (is_foreground_calc(calc) ||
        interest == background_request_interest_type::OBJECTIFIED_FORM)
    {
        // Try some shortcuts for cases where the arguments aren't too
        // complicated.
        switch (count_nontrivial_args(calc))
        {
         case 0:
            // Everything is trivial, so there's nothing to do.
            break;
         case 1:
            // If there's exactly one nontrivial argument, we can pass through
            // to it.
            for (auto const& arg : calc.args)
            {
                if (!is_trivial(arg))
                {
                    update_resolution(bg, context, resolution, arg,
                        foreground_only, interest);
                }
            }
            break;
         default:
            // The shortcuts won't work, so do the general procedure.
            update_resolution_list(bg, context,
                cast_resolution_data<list_resolution_data>(*resolution),
                as_function(request).args, foreground_only, interest);
            break;
        }
    }
    else
    {
        auto& data_ptr =
            cast_resolution_data<untyped_background_data_ptr>(*resolution);
        initialize_if_needed(bg, data_ptr, request);
        if (!foreground_only)
        {
            if (update_background_pointer(bg, context,
                    request.result_interface,
                    data_ptr,
                    [&]() { return as_request_object(request); },
                    is_disk_cached(*calc.function)))
            {
                auto* job = new local_calculation_job(bg, context, request);
                add_untyped_background_job(data_ptr, *bg,
                    background_job_queue_type::CALCULATION, job);
            }
        }
    }
}

bool static
local_calculation_result_is_resolved(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& calc = as_function(request);
    if (is_foreground_calc(calc))
    {
        switch (count_nontrivial_args(calc))
        {
         case 0:
            return true;
         case 1:
            for (auto const& arg : calc.args)
            {
                if (!is_trivial(arg))
                    return result_is_resolved(resolution, arg);
            }
            assert(0);
         default:
            return
                result_is_resolved(
                    cast_resolution_data<list_resolution_data>(*resolution),
                    as_function(request).args);
        }
    }
    else
    {
        auto& data_ptr =
            cast_resolution_data<untyped_background_data_ptr>(*resolution);
        return data_ptr.is_ready();
    }
}

untyped_immutable static
get_local_calculation_result(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& calc = as_function(request);

    if (is_foreground_calc(calc))
    {
        if (count_nontrivial_args(calc) < 2)
        {
            // This just works because there's at most one argument that will
            // actually use the resolution data.
            std::vector<untyped_immutable> arg_results;
            arg_results.reserve(calc.args.size());
            for (auto const& arg : calc.args)
                arg_results.push_back(get_result(resolution, arg));
            // Now execute the function.
            null_check_in check_in;
            null_progress_reporter reporter;
            return calc.function->execute(check_in, reporter, arg_results);
        }
        else
        {
            // Otherwise, do the general process.
            auto args =
                get_request_list_results(
                    cast_resolution_data<list_resolution_data>(*resolution),
                    calc.args);
            // Actually execute the function.
            {
                null_check_in check_in;
                null_progress_reporter reporter;
                return calc.function->execute(check_in, reporter, args);
            }
        }
    }
    else
    {
        auto& data_ptr =
            cast_resolution_data<untyped_background_data_ptr>(*resolution);
        return data_ptr.data();
    }
}

struct object_resolution_data
{
    // Object resolution requires resolving the immutable ID and then the
    // data, so we need data pointers for both of those.
    untyped_background_data_ptr immutable_id, data;
};

bool static
object_data_result_is_resolved(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& data_ptr =
        cast_resolution_data<object_resolution_data>(resolution_data);
    return data_ptr.data.is_ready();
}

bool static
immutable_data_result_is_resolved(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& data_ptr =
        cast_resolution_data<untyped_background_data_ptr>(resolution_data);
    return data_ptr.is_ready();
}

untyped_immutable static
get_object_data_result(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& data_ptr =
        cast_resolution_data<object_resolution_data>(resolution_data);
    return data_ptr.data.data();
}

untyped_immutable static
get_immutable_data_result(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& data_ptr =
        cast_resolution_data<untyped_background_data_ptr>(resolution_data);
    return data_ptr.data();
}


bool static
local_calc_objectification_complete(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& calc = as_function(request);
    // Do the shortcuts if possible.
    if (count_nontrivial_args(calc) < 2)
    {
        // This just works because there's at most one argument that will
        // actually use the resolution data.
        for (auto const& arg : calc.args)
        {
            if (!objectification_complete(resolution, arg))
                return false;
        }
        return true;
    }
    else // Otherwise, do the general process.
    {
        return
            list_objectification_complete(
                cast_resolution_data<list_resolution_data>(*resolution),
                calc.args);
    }
}

untyped_request static
get_local_calc_objectified_form(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& calc = as_function(request);
    bool execute_in_foreground = is_foreground_calc(calc);

    // The objectified calc is just the same calc with the arguments in
    // objectified form, so most this function is just concerned with
    // objectifying the arguments.
    function_request_info objectified_calc;
    objectified_calc.force_foreground_resolution =
        calc.force_foreground_resolution;
    objectified_calc.function = calc.function;

    // Do the shortcuts if possible.
    if (count_nontrivial_args(calc) < 2)
    {
        // This just works because there's at most one argument that will
        // actually use the resolution data.
        objectified_calc.args.reserve(calc.args.size());
        for (auto const& arg : calc.args)
        {
            objectified_calc.args.push_back(
                get_objectified_form(resolution, arg));
        }
    }
    else // Otherwise, do the general process.
    {
        objectified_calc.args =
            get_list_objectified_form(
                cast_resolution_data<list_resolution_data>(*resolution),
                calc.args);
    }

    return replace_request_contents(request, objectified_calc);
}

// ARRAY REQUEST - resolving an array request

value static
merge_items(
    std::vector<untyped_immutable> const& immutable_items,
    std::vector<untyped_request> const& requests)
{
    size_t n_items = immutable_items.size();
    assert(requests.size() == n_items);
    value_list values(n_items);
    for (size_t i = 0; i != n_items; ++i)
    {
        values[i] =
            requests[i].result_interface->immutable_to_value(
                immutable_items[i]);
    }
    return value(values);
}

void static
update_array_request(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data* resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    auto const& items = as_array(request);
    switch (count_nontrivial_requests(items))
    {
     case 0:
        // Everything is trivial, so there's nothing to do.
        break;
     case 1:
        // If there's exactly one nontrivial item, we can pass through to it.
        for (auto const& item : items)
        {
            if (!is_trivial(item))
            {
                update_resolution(bg, context, resolution, item,
                    foreground_only, interest);
            }
        }
        break;
     default:
        update_resolution_list(bg, context,
            cast_resolution_data<list_resolution_data>(*resolution),
            as_array(request), foreground_only, interest);
        break;
    }
}

bool static
array_request_result_is_resolved(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& items = as_array(request);
    switch (count_nontrivial_requests(items))
    {
     case 0:
        return true;
     case 1:
        for (auto const& item : items)
        {
            if (!is_trivial(item))
                return result_is_resolved(resolution, item);
        }
        assert(0);
     default:
        return
            result_is_resolved(
                cast_resolution_data<list_resolution_data>(*resolution),
                as_array(request));
    }
}

untyped_immutable static
get_array_request_result(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& items = as_array(request);
    if (count_nontrivial_requests(items) < 2)
    {
        // This just works because there's at most one item that will
        // actually use the resolution data.
        std::vector<untyped_immutable> item_results;
        item_results.reserve(items.size());
        for (auto const& item : items)
            item_results.push_back(get_result(resolution, item));
        auto merged = merge_items(item_results, items);
        return request.result_interface->value_to_immutable(merged);
    }
    else
    {
        auto merged =
            merge_items(
                get_request_list_results(
                    cast_resolution_data<list_resolution_data>(*resolution),
                    items),
                items);
        return request.result_interface->value_to_immutable(merged);
    }
}

bool static
array_objectification_complete(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& items = as_array(request);
    if (count_nontrivial_requests(items) < 2)
    {
        // This just works because there's at most one item that will
        // actually use the resolution data.
        std::vector<untyped_immutable> item_results;
        item_results.reserve(items.size());
        for (auto const& item : items)
        {
            if (!objectification_complete(resolution, item))
                return false;
        }
        return true;
    }
    else
    {
        return
            list_objectification_complete(
                cast_resolution_data<list_resolution_data>(*resolution),
                as_array(request));
    }
}

untyped_request static
get_array_objectified_form(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& items = as_array(request);
    if (count_nontrivial_requests(items) < 2)
    {
        // This just works because there's at most one item that will
        // actually use the resolution data.
        std::vector<untyped_request> objectified_items;
        objectified_items.reserve(items.size());
        for (auto const& item : items)
        {
            objectified_items.push_back(
                get_objectified_form(resolution, item));
        }
        return replace_request_contents(request, objectified_items);
    }
    else
    {
        auto objectified_list =
            get_list_objectified_form(
                cast_resolution_data<list_resolution_data>(*resolution),
                as_array(request));
        return replace_request_contents(request, objectified_list);
    }
}

// STRUCTURE REQUEST - resolving a structure request

size_t static
count_nontrivial_fields(structure_request_info const& info)
{
    return count_nontrivial_requests(info.fields);
}

void static
update_structure_request(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data* resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    auto const& info = as_structure(request);
    switch (count_nontrivial_fields(info))
    {
     case 0:
        // Everything is trivial, so there's nothing to do.
        break;
     case 1:
        // If there's exactly one nontrivial field, we can pass through
        // to it.
        for (auto const& field : info.fields)
        {
            if (!is_trivial(field.second))
            {
                update_resolution(bg, context, resolution, field.second,
                    foreground_only, interest);
            }
        }
        break;
     default:
        update_resolution_map(bg, context,
            cast_resolution_data<map_resolution_data>(*resolution),
            as_structure(request).fields, foreground_only, interest);
        break;
    }
}

bool static
structure_request_result_is_resolved(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& info = as_structure(request);
    switch (count_nontrivial_fields(info))
    {
     case 0:
        return true;
     case 1:
        for (auto const& field : info.fields)
        {
            if (!is_trivial(field.second))
                return result_is_resolved(resolution, field.second);
        }
        assert(0);
     default:
        return
            result_is_resolved(
                cast_resolution_data<map_resolution_data>(*resolution),
                as_structure(request).fields);
    }
}

untyped_immutable static
get_structure_request_result(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& info = as_structure(request);
    if (count_nontrivial_fields(info) < 2)
    {
        // This just works because there's at most one field that will
        // actually use the resolution data.
        std::map<string,untyped_immutable> field_results;
        for (auto const& field : info.fields)
        {
            field_results[field.first] =
                get_result(resolution, field.second);
        }
        return info.constructor->construct(field_results);
    }
    else
    {
        return
            as_structure(request).constructor->construct(
                get_request_map_results(
                    cast_resolution_data<map_resolution_data>(*resolution),
                    info.fields));
    }
}

bool static
structure_objectification_complete(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& info = as_structure(request);
    if (count_nontrivial_fields(info) < 2)
    {
        // This just works because there's at most one field that will
        // actually use the resolution data.
        for (auto const& field : info.fields)
        {
            if (!objectification_complete(resolution, field.second))
                return false;
        }
        return true;
    }
    else
    {
        return
            map_objectification_complete(
                cast_resolution_data<map_resolution_data>(*resolution),
                info.fields);
    }
}

untyped_request static
get_structure_objectified_form(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    auto const& info = as_structure(request);
    structure_request_info objectified_info;
    objectified_info.constructor = info.constructor;
    if (count_nontrivial_fields(info) < 2)
    {
        // This just works because there's at most one field that will
        // actually use the resolution data.
        for (auto const& field : info.fields)
        {
            objectified_info.fields[field.first] =
                get_objectified_form(resolution, field.second);
        }
    }
    else
    {
        objectified_info.fields =
            get_map_objectified_form(
                cast_resolution_data<map_resolution_data>(*resolution),
                info.fields);
    }
    return replace_request_contents(request, objectified_info);
}


// DATA - resolving ids for objects (thinknode iss objects) to immutable ids
//                  when resolving object ids the
//                  data will also be upgraded if needed
// Retrieving immutable data from thinknode

string
parse_response_header(string const& response_header, string const& field)
{
    std::istringstream resp(response_header);
    string header;
    size_t index;
    while (std::getline(resp, header) && header != "\r")
    {
        index = header.find(':', 0);
        if(index != string::npos)
        {
            if (field == boost::algorithm::trim_copy(header.substr(0, index)))
            {
                return boost::algorithm::trim_copy(header.substr(index + 1));
            }
        }
    }

    throw exception("Unable to find field " + field + " in response header: " +
        response_header);
}

optional<string>
parse_json_response_body(string const& response_body, string const& field)
{
    Json::Value root;
    Json::Reader reader;
    bool parsing_Successful = reader.parse(response_body.c_str(), root);

    if (!parsing_Successful)
    {
        // This calculation has no json parsable text.
        return none;
    }

    if (root.isMember(field))
    {
        auto value = root.get(field, "").asString();
        return value;
    }

    throw exception("Unable to find field " + field + " in response body: " +
        response_body);
}


string static
make_calc_status_url(framework_context const& context, string const& id)
{
    return context.framework.api_url + "/calc/" + id + "/status?context=" +
        context.context_id;
}

struct immutable_id_resolve_request : background_web_job
{
    immutable_id_resolve_request(
        alia__shared_ptr<background_execution_system> const& bg,
        framework_context const& context,
        dynamic_type_interface const* type_interface,
        id_interface const& id,
        untyped_request const& request)
      : context(context)
      , type_interface(type_interface)
      , request(request)
    {
        this->system = bg;
        this->id.store(id);
    }

    bool inputs_ready()
    {
        // We want to use the context associated with this request, which may
        // be different from the one associated with the system.
        framework_context unused;
        return get_session_and_context(*this->system, &this->session, &unused);
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        auto web_request =
            make_get_request(
                context.framework.api_url + "/iss/" + as_object(request) +
                    "/immutable?context=" + context.context_id,
                make_header_list("Accept: application/json"));

        web_response response;

        while(1)
        {
            try
            {
                response =
                    perform_web_request(check_in, reporter, *this->connection,
                        session, web_request);
                break;
            }
            catch (web_request_failure& failure)
            {
                if (failure.response_code() == 202) // Upgrade isn't ready
                {
                    auto ref_id =
                        parse_response_header(
                            failure.response_header(),
                            "Thinknode-Reference-Id");

                    null_progress_reporter null_reporter;
                    // This means that the ID referred to a calculation result
                    // that wasn't ready yet, so wait for it to be ready...
                    wait_for_remote_calculation(check_in, null_reporter,
                        *this->connection, this->context, this->session,
                        ref_id);
                    // ... and try again.
                    continue;
                }
                else
                {
                    auto status_query =
                        make_get_request(
                            make_calc_status_url(context, as_object(request)),
                            no_headers);
                    null_progress_reporter null_reporter;
                    auto status_response =
                        perform_web_request(check_in, null_reporter, *this->connection, session,
                            status_query);

                    calculation_status status;
                    from_value(&status, parse_json_response(status_response));

                    if (status.type == calculation_status_type::CANCELED)
                    {
                        reset_cached_data(*this->system, this->id.get());
                        return;
                    }
                    else if (failure.response_code() == 204 &&
                        status.type == calculation_status_type::FAILED)
                    {
                        auto failed = as_failed(status);
                        auto current_sub_calc_id =
                            parse_json_response_body(failed.message, "id");
                        // Loop through all the dependent calcs until we reach the bottom
                        while (1 && current_sub_calc_id)
                        {
                            auto sub_status_query =
                                make_get_request(
                                    make_calc_status_url(context, get(current_sub_calc_id)),
                                    no_headers);
                            auto sub_status_response =
                                perform_web_request(check_in, null_reporter, *this->connection,
                                    session, sub_status_query);

                            from_value(&status, parse_json_response(sub_status_response));
                            failed = as_failed(status);
                            if (failed.error == "failed_dependency")
                            {
                                current_sub_calc_id =
                                    parse_json_response_body(failed.message, "id");
                            }
                            else
                            {
                                // We reached the bottom of the calculation chain.
                                break;
                            }
                        }

                        throw exception(failed.message);
                    }
                    else
                    {
                        throw;
                    }
                }
            }
        }
        check_in();

        // Getting the string of the immutable id to store in the memory cache
        immutable<string> tmp;
        auto val = parse_json_response(response);
        immutable_response is = from_value<immutable_response>(val);
        swap_in(tmp, is.id);
        set_cached_data(*this->system, this->id.get(), erase_type(tmp));

        check_in();
        write_to_disk_cache(*this->system, this->context,
            as_request_object(request), to_value(get(tmp)));
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description =
            "resolve object to immutable\n" +
            context.framework.api_url + "/iss/" + as_object(request) +
            "/immutable?context=" + context.context_id;
        return info;
    }

    framework_context context;
    dynamic_type_interface const* type_interface;
    owned_id id;
    untyped_request request;

    web_session_data session;
};

struct immutable_data_request : background_web_job
{
    immutable_data_request(
        alia__shared_ptr<background_execution_system> const& bg,
        framework_context const& context,
        dynamic_type_interface const* type_interface,
        id_interface const& id,
        untyped_request const& request)
      : context(context)
      , type_interface(type_interface)
      , request(request)
    {
        this->system = bg;
        this->id.store(id);
    }

    bool inputs_ready()
    {
        // We want to use the context associated with this request, which may
        // be different from the one associated with the system.
        framework_context unused;
        return get_session_and_context(*this->system, &this->session, &unused);
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        auto web_request =
            make_get_request(
                context.framework.api_url + "/iss/immutable/" +
                    as_immutable(request) + "?context=" + context.context_id,
                make_header_list("Accept: application/octet-stream"));

        auto response =
            perform_web_request(check_in, reporter, *this->connection,
                session, web_request);

        auto value = parse_msgpack_response(response);
        auto immutable = type_interface->value_to_immutable(value);
        set_cached_data(*this->system, this->id.get(), immutable);

        check_in();
        write_to_disk_cache(*this->system, this->context,
            as_request_object(request), value);
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description =
            "immutable data retrieval\n" + as_immutable(request) + "\n" +
            context.framework.api_url + "/iss/immutable/" +
            as_immutable(request) + "?context=" + context.context_id;
        return info;
    }

    framework_context context;
    dynamic_type_interface const* type_interface;
    owned_id id;
    untyped_request request;

    web_session_data session;
};

void static
retrieve_immutable_data(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    untyped_background_data_ptr& resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    if (interest != background_request_interest_type::RESULT)
        return;

    initialize_if_needed(bg, resolution, request);
    if (!foreground_only)
    {
        if (update_background_pointer(bg, context, request.result_interface,
                resolution,
                [&]() { return as_request_object(request); }))
        {
            add_untyped_background_job(resolution, *bg,
                background_job_queue_type::WEB_READ,
                new immutable_data_request(
                    bg, context, request.result_interface, resolution.key(),
                    request));
        }
    }
}

void static
resolve_iss_object_id(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    object_resolution_data& resolution,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    if (interest != background_request_interest_type::RESULT)
        return;

    initialize_if_needed(bg, resolution.immutable_id, request);
    if (!foreground_only)
    {
        static dynamic_type_implementation<string> id_result_interface;
        if (update_background_pointer(bg, context, &id_result_interface,
                resolution.immutable_id,
                [&]() { return as_request_object(request); }))
        {
            add_untyped_background_job(resolution.immutable_id, *bg,
                background_job_queue_type::WEB_READ,
                new immutable_id_resolve_request(bg, context,
                    request.result_interface, resolution.immutable_id.key(),
                    request));
        }
    }

    // The rest of this function is concerned with getting the data, so if we
    // don't have the ID, there's nothing to do.
    if (!resolution.immutable_id.is_ready())
    {
        return;
    }

    // Now that we have the immutable ID we can construct the request to
    // retrieve the immutable data
    auto data_request =
        make_untyped_request(request_type::IMMUTABLE,
            get(cast_immutable<string>(resolution.immutable_id.data())),
            request.result_interface);

    // And process that request.
    retrieve_immutable_data(bg, context, resolution.data, data_request,
        foreground_only, interest);
}

void static
update_immutable_data_resolution(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data& resolution_data,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    if (interest != background_request_interest_type::RESULT)
        return;

    auto& resolution =
        cast_resolution_data<untyped_background_data_ptr>(resolution_data);

    initialize_if_needed(bg, resolution, request);
    retrieve_immutable_data(bg, context, resolution, request, foreground_only,
        interest);
}

void static
update_object_data_resolution(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data& resolution_data,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    if (interest != background_request_interest_type::RESULT)
        return;

    auto& resolution =
        cast_resolution_data<object_resolution_data>(resolution_data);

    initialize_if_needed(bg, resolution.immutable_id, request);
    resolve_iss_object_id(bg, context, resolution, request, foreground_only,
        interest);
}

// REMOTE CALCS - resolving remote calculation requests

// STore object resolution data here
struct remote_calc_resolution_data
{
    untyped_background_data_ptr id;
    object_resolution_data obj_res;
};

string static
get_calculation_description(calculation_request const& calc)
{
    string description = to_string(calc.type);
    if (calc.type == calculation_request_type::FUNCTION)
    {
        description += "\n" +
            as_function(calc).app + "/" +
            as_function(calc).name;
    }
    return description;
}

struct remote_calc_id_request : background_web_job
{
    remote_calc_id_request(
        alia__shared_ptr<background_execution_system> const& bg,
        framework_context const& context,
        id_interface const& id,
        calculation_request const& calculation)
      : context(context)
      , calculation(calculation)
    {
        this->system = bg;
        this->id.store(id);
    }

    bool inputs_ready()
    {
        // We want to use the context associated with this request, which may
        // be different from the one associated with the system.
        framework_context unused;
        return get_session_and_context(*this->system, &this->session, &unused);
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        string remote_id =
            request_remote_calculation(
                check_in, *this->connection,
                context, session, this->calculation);
        check_in();

        immutable<string> tmp;
        swap_in(tmp, remote_id);
        set_cached_data(*this->system, this->id.get(), erase_type(tmp));

        write_to_disk_cache(*this->system, this->context,
            make_request_object_with_remote(
                thinknode_request_as_request_object(calculation)),
            to_value(get(tmp)));
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description =
            "calculation submission\n" +
            get_calculation_description(calculation);
        return info;
    }

    framework_context context;
    owned_id id;
    calculation_request calculation;

    web_session_data session;
};

struct background_data_progress_reporter : progress_reporter_interface
{
    void operator()(float progress)
    {
        update_background_data_progress(*this->system, *this->id, progress);
    }
    background_execution_system* system;
    id_interface const* id;
};

// Update the resolution of a remote calculation.
// Note that this works for both METAs and actual REMOTE_CALCULATIONs.
void static
update_remote_calculation(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data& resolution_data,
    untyped_request const& request,
    bool foreground_only,
    background_request_interest_type interest)
{
    auto& resolution =
        cast_resolution_data<remote_calc_resolution_data>(resolution_data);

    if (!resolution.id.is_initialized())
    {
        resolution.id.reset(*bg, make_request_id(request));
    }

    if (!resolution.id.is_ready() && !foreground_only)
    {
        static dynamic_type_implementation<string> type_interface;
        if (update_background_pointer(bg, context, &type_interface,
                resolution.id,
                [&]() { return as_request_object(request); }))
        {
            add_untyped_background_job(resolution.id, *bg,
                background_job_queue_type::REMOTE_CALCULATION,
                new remote_calc_id_request(bg, context, resolution.id.key(),
                    as_thinknode_request(request)));
        }
    }

    // The rest of this function is concerned with getting the data, so if
    // we're not interested in the data or we don't have the ID, there's
    // nothing to do.
    if (interest != background_request_interest_type::RESULT ||
        !resolution.id.is_ready())
    {
        return;
    }

    // Construct the request for the calculation result.
    string const& object_id = get(cast_immutable<string>(resolution.id.data()));
    auto object_request =
        make_untyped_request(request_type::OBJECT, object_id,
            request.result_interface);

    // Now that we have the ISS object ID for the calculation result, we can
    // resolve it to an immutable ID and then retrieve the data
    resolve_iss_object_id(bg, context, resolution.obj_res, object_request,
        foreground_only, interest);
}

// Check if a remote calculation is finished resolved.
// Note that this works for both METAs and actual REMOTE_CALCULATIONs.
bool static
remote_calculation_result_is_resolved(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& resolution =
        cast_resolution_data<remote_calc_resolution_data>(resolution_data);
    return resolution.obj_res.data.is_ready();
}

// Get the result of a remote calculation.
// Note that this works for both METAs and actual REMOTE_CALCULATIONs.
untyped_immutable static
get_remote_calculation_result(
    background_request_resolution_data& resolution_data,
    untyped_request const& request)
{
    auto& resolution =
        cast_resolution_data<remote_calc_resolution_data>(resolution_data);
    return resolution.obj_res.data.data();
}

// PRERESOLUTION

// Check if the given request contains any subrequests that need to be
// preresolved.
bool static
requires_preresolution(untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return false;
     case request_type::FUNCTION:
        for (auto const& arg : as_function(request).args)
        {
            if (requires_preresolution(arg))
                return true;
        }
        return false;
     case request_type::REMOTE_CALCULATION:
        return requires_preresolution(as_remote_calc(request));
     case request_type::META:
        return requires_preresolution(as_meta(request));
     case request_type::OBJECT:
        return false;
     case request_type::IMMUTABLE:
        return false;
     case request_type::ARRAY:
        for (auto const& item : as_array(request))
        {
            if (requires_preresolution(item))
                return true;
        }
        return false;
     case request_type::STRUCTURE:
        for (auto const& field : as_structure(request).fields)
        {
            if (requires_preresolution(field.second))
                return true;
        }
        return false;
     case request_type::PROPERTY:
        return
            requires_preresolution(
                as_property(request).record);
     case request_type::UNION:
        return
            requires_preresolution(
                as_union(request).member_request);
     case request_type::SOME:
        return
            requires_preresolution(
                as_some(request).value);
     case request_type::REQUIRED:
        return
            requires_preresolution(
                as_required(request).optional_value);
     case request_type::ISOLATED:
        // This is what actually needs to be preresolved.
        return true;
     default:
        assert(0);
        return false;
    }
}

void static
collect_preresolved_subrequests(
    std::vector<untyped_request>& subrequests,
    untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        break;
     case request_type::FUNCTION:
        for (auto const& arg : as_function(request).args)
            collect_preresolved_subrequests(subrequests, arg);
        break;
     case request_type::REMOTE_CALCULATION:
        collect_preresolved_subrequests(subrequests, as_remote_calc(request));
        break;
     case request_type::META:
        collect_preresolved_subrequests(subrequests, as_meta(request));
     case request_type::OBJECT:
        break;
     case request_type::IMMUTABLE:
        break;
     case request_type::ARRAY:
        for (auto const& item : as_array(request))
            collect_preresolved_subrequests(subrequests, item);
        break;;
     case request_type::STRUCTURE:
        for (auto const& field : as_structure(request).fields)
            collect_preresolved_subrequests(subrequests, field.second);
        break;
     case request_type::PROPERTY:
        return collect_preresolved_subrequests(subrequests,
            as_property(request).record);
     case request_type::UNION:
        return collect_preresolved_subrequests(subrequests,
            as_union(request).member_request);
     case request_type::SOME:
        return collect_preresolved_subrequests(subrequests,
            as_some(request).value);
     case request_type::REQUIRED:
        return collect_preresolved_subrequests(subrequests,
            as_required(request).optional_value);
     case request_type::ISOLATED:
        // This is what actually needs to be preresolved.
        subrequests.push_back(as_isolated(request));
        break;
     default:
        assert(0);
        break;
    }
}

void static
update_preresolution_list(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    list_resolution_data& resolutions,
    std::vector<untyped_request> const& requests,
    bool foreground_only)
{
    size_t n_requests = requests.size();
    if (resolutions.empty())
        resolutions.resize(n_requests);
    for (size_t i = 0; i != n_requests; ++i)
    {
        auto const& request = requests[i];
        update_resolution(
            bg, context, &resolutions[i], request, foreground_only,
            // If it's a remote request, we're going to reinsert it by
            // reference, so we only need the objectified form.
            // Local requests are inserted by value, so we need the result.
            (request.type == request_type::REMOTE_CALCULATION ||
             request.type == request_type::META ||
             request.type == request_type::OBJECT ||
             request.type == request_type::IMMUTABLE) ?
                background_request_interest_type::OBJECTIFIED_FORM :
                background_request_interest_type::RESULT);
    }
}

bool static
is_preresolved(
    std::vector<background_request_resolution_data>& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    for (size_t i = 0; i != n_requests; ++i)
    {
        if (!is_preresolved(&resolutions[i], requests[i]))
            return false;
    }
    return true;
}

std::vector<untyped_request> static
get_preresolved_request_list_results(
    std::vector<background_request_resolution_data>& resolutions,
    std::vector<untyped_request> const& requests)
{
    size_t n_requests = requests.size();
    std::vector<untyped_request> results(n_requests);
    for (size_t i = 0; i != n_requests; ++i)
        results[i] = get_preresolved_result(&resolutions[i], requests[i]);
    return results;
}

untyped_request static
replace_preresolved_subrequests(
    std::vector<untyped_request>::const_iterator& results,
    untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return request;
     case request_type::FUNCTION:
      {
        auto const& spec = as_function(request);
        function_request_info new_spec;
        new_spec.force_foreground_resolution =
            spec.force_foreground_resolution;
        new_spec.function = spec.function;
        new_spec.args.reserve(spec.args.size());
        for (auto const& arg : spec.args)
        {
            new_spec.args.push_back(
                replace_preresolved_subrequests(results, arg));
        }
        return replace_request_contents(request, new_spec);
      }
     case request_type::REMOTE_CALCULATION:
        return
            replace_request_contents(request,
                replace_preresolved_subrequests(results,
                    as_remote_calc(request)));
     case request_type::META:
        return
            replace_request_contents(request,
                replace_preresolved_subrequests(results,
                    as_meta(request)));
     case request_type::OBJECT:
        return request;
     case request_type::IMMUTABLE:
        return request;
     case request_type::ARRAY:
      {
        auto const& items = as_array(request);
        std::vector<untyped_request> new_items;
        new_items.reserve(items.size());
        for (auto const& item : items)
        {
            new_items.push_back(
                replace_preresolved_subrequests(results, item));
        }
        return replace_request_contents(request, new_items);
      }
     case request_type::STRUCTURE:
      {
        auto const& info = as_structure(request);
        structure_request_info new_info;
        new_info.constructor = info.constructor;
        for (auto const& field : info.fields)
        {
            new_info.fields[field.first] =
                replace_preresolved_subrequests(results, field.second);
        }
        return replace_request_contents(request, new_info);
      }
     case request_type::PROPERTY:
      {
        auto const& info = as_property(request);
        property_request_info new_info;
        new_info.extractor = info.extractor;
        new_info.field = info.field;
        new_info.record =
            replace_preresolved_subrequests(results, info.record);
        return replace_request_contents(request, new_info);
      }
     case request_type::UNION:
      {
        auto const& info = as_union(request);
        union_request_info new_info;
        new_info.constructor = info.constructor;
        new_info.member_name = info.member_name;
        new_info.member_request =
            replace_preresolved_subrequests(results, info.member_request);
        return replace_request_contents(request, new_info);
      }
     case request_type::SOME:
      {
        auto const& info = as_some(request);
        some_request_info new_info;
        new_info.value =
            replace_preresolved_subrequests(results, info.value);
        new_info.wrapper = info.wrapper;
        return replace_request_contents(request, new_info);
      }
     case request_type::REQUIRED:
      {
        auto const& info = as_required(request);
        required_request_info new_info;
        new_info.optional_value =
            replace_preresolved_subrequests(results,
                info.optional_value);
        new_info.unwrapper = info.unwrapper;
        return replace_request_contents(request, new_info);
      }
     case request_type::ISOLATED:
      {
        auto replacement = *results;
        ++results;
        return replacement;
      }
     default:
        assert(0);
        return request;
    }
}

// Attempt to preresolve any isolated subrequests within a request.
void static
preresolve_subrequests(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_preresolution_data* preresolution,
    untyped_request const& request,
    bool foreground_only)
{
    // If the request has value-identified subrequests, we have to resolve
    // those first.
    if (requires_preresolution(request))
    {
        switch (preresolution->state)
        {
         case request_preresolution_state::UNINITIALIZED:
            collect_preresolved_subrequests(
                preresolution->subrequests, request);
            preresolution->state = request_preresolution_state::RESOLVING;
            // Intentional fallthrough.
         case request_preresolution_state::RESOLVING:
          {
            update_preresolution_list(bg, context,
                preresolution->subrequest_resolutions,
                preresolution->subrequests,
                foreground_only);
            if (!is_preresolved(
                    preresolution->subrequest_resolutions,
                    preresolution->subrequests))
            {
                return;
            }
            // Gather the subrequest results.
            auto subrequest_results =
                get_preresolved_request_list_results(
                    preresolution->subrequest_resolutions,
                    preresolution->subrequests);
            std::vector<untyped_request>::const_iterator
                results_iterator = subrequest_results.begin();
            preresolution->preresolved_request =
                replace_preresolved_subrequests(results_iterator, request);
            preresolution->state =
                request_preresolution_state::RESOLVED;
            // We can clear out this data now.
            preresolution->subrequests.clear();
            preresolution->subrequest_resolutions.clear();
          }
        }
    }
}

// Get the preresolved form of a request.
// If the return value is null, preresolution isn't finished and/or something
// wasn't ready.
static untyped_request const*
get_preresolved_request(
    background_request_preresolution_data* preresolution,
    untyped_request const& original_request)
{
    if (requires_preresolution(original_request))
    {
        switch (preresolution->state)
        {
         case request_preresolution_state::RESOLVED:
            return &preresolution->preresolved_request;
         default:
            return 0;
        }
    }
    else
        return &original_request;
}

// GENERAL RESOLUTION

bool static
is_trivial(untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return true;
     case request_type::FUNCTION:
        return is_calc_trivial(as_function(request));
     case request_type::REMOTE_CALCULATION:
     case request_type::META:
     case request_type::OBJECT:
        return false;
     case request_type::IMMUTABLE:
        return false;
     case request_type::ARRAY:
        return count_nontrivial_requests(as_array(request)) == 0;
     case request_type::STRUCTURE:
        return count_nontrivial_fields(as_structure(request)) == 0;
     case request_type::PROPERTY:
        return is_trivial(as_property(request).record);
     case request_type::UNION:
        return is_trivial(as_union(request).member_request);
     case request_type::SOME:
        return is_trivial(as_some(request).value);
     case request_type::REQUIRED:
        return is_trivial(as_required(request).optional_value);
     case request_type::ISOLATED:
        return false;
     default:
        assert(0);
        return false;
    }
}

untyped_immutable static
resolve_trivial_request(untyped_request const& request)
{
    return get_result(0, request);
}

void static
update_resolution(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    background_request_resolution_data* resolution,
    untyped_request const& original_request,
    bool foreground_only,
    background_request_interest_type interest)
{
    preresolve_subrequests(bg, context, &resolution->preresolution,
        original_request, foreground_only);
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    if (!preresolved_request)
        return;
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::IMMEDIATE:
        break;
     case request_type::FUNCTION:
        update_local_calculation(bg, context, resolution, request,
            foreground_only, interest);
        break;
     case request_type::REMOTE_CALCULATION:
        update_remote_calculation(bg, context, *resolution, request,
            foreground_only, interest);
        break;
     case request_type::META:
        update_remote_calculation(bg, context, *resolution, request,
            foreground_only, interest);
        break;
     case request_type::OBJECT:
        update_object_data_resolution(bg, context, *resolution, request,
            foreground_only, interest);
        break;
     case request_type::IMMUTABLE:
        update_immutable_data_resolution(bg, context, *resolution, request,
            foreground_only, interest);
        break;
     case request_type::ARRAY:
        update_array_request(bg, context, resolution, request,
            foreground_only, interest);
        break;
     case request_type::STRUCTURE:
        update_structure_request(bg, context, resolution, request,
            foreground_only, interest);
        break;
     case request_type::PROPERTY:
        update_resolution(bg, context, resolution,
            as_property(request).record, foreground_only, interest);
        break;
     case request_type::UNION:
        update_resolution(bg, context, resolution,
            as_union(request).member_request, foreground_only, interest);
        break;
     case request_type::SOME:
        update_resolution(bg, context, resolution, as_some(request).value,
            foreground_only, interest);
        break;
     case request_type::REQUIRED:
        update_resolution(bg, context, resolution,
            as_required(request).optional_value, foreground_only, interest);
        break;
     case request_type::ISOLATED:
        // These should already have been eliminated by now.
        assert(0);
        break;
     default:
        assert(0);
    }
}

bool static
result_is_resolved(
    background_request_resolution_data* resolution,
    untyped_request const& original_request)
{
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    if (!preresolved_request)
        return false;
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return true;
     case request_type::REMOTE_CALCULATION:
        return remote_calculation_result_is_resolved(*resolution, request);
     case request_type::META:
        return remote_calculation_result_is_resolved(*resolution, request);
     case request_type::FUNCTION:
        return local_calculation_result_is_resolved(resolution, request);
     case request_type::OBJECT:
        return object_data_result_is_resolved(*resolution, request);
     case request_type::IMMUTABLE:
        return immutable_data_result_is_resolved(*resolution, request);
     case request_type::ARRAY:
        return array_request_result_is_resolved(resolution, request);
     case request_type::STRUCTURE:
        return structure_request_result_is_resolved(resolution, request);
     case request_type::PROPERTY:
        return result_is_resolved(resolution, as_property(request).record);
     case request_type::UNION:
        return result_is_resolved(resolution, as_union(request).member_request);
     case request_type::SOME:
        return result_is_resolved(resolution, as_some(request).value);
     case request_type::REQUIRED:
        return result_is_resolved(resolution,
            as_required(request).optional_value);
     case request_type::ISOLATED:
        return true;
     default:
        assert(0);
        return false;
    }
}

untyped_immutable static
get_result(
    background_request_resolution_data* resolution,
    untyped_request const& original_request)
{
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    assert (preresolved_request);
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::IMMEDIATE:
        return as_immediate(request);
     case request_type::REMOTE_CALCULATION:
        return get_remote_calculation_result(*resolution, request);
     case request_type::META:
        return get_remote_calculation_result(*resolution, request);
     case request_type::FUNCTION:
        return get_local_calculation_result(resolution, request);
     case request_type::OBJECT:
        return get_object_data_result(*resolution, request);
     case request_type::IMMUTABLE:
        return get_immutable_data_result(*resolution, request);
     case request_type::ARRAY:
        return get_array_request_result(resolution, request);
     case request_type::STRUCTURE:
        return get_structure_request_result(resolution, request);
     case request_type::PROPERTY:
      {
        auto const& property = as_property(request);
        return property.extractor->extract(
            get_result(resolution, property.record));
      }
     case request_type::UNION:
      {
        return as_union(request).constructor->construct(
            get_result(resolution, as_union(request).member_request));
      }
     case request_type::SOME:
      {
        auto const& some = as_some(request);
        return some.wrapper->wrap(
            get_result(resolution, some.value));
      }
     case request_type::REQUIRED:
      {
        auto const& required = as_required(request);
        return required.unwrapper->unwrap(
            get_result(resolution, required.optional_value));
      }
     case request_type::ISOLATED:
        // These should already have been eliminated by now.
        assert(0);
        throw exception("internal error: unresolved isolated request");
     default:
        assert(0);
        throw exception("internal error: invalid request type");
    }
}

bool static
is_preresolved(
    background_request_resolution_data* resolution,
    untyped_request const& original_request)
{
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    if (!preresolved_request)
        return false;
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::REMOTE_CALCULATION:
     case request_type::META:
      {
        auto& calc_resolution =
            cast_resolution_data<remote_calc_resolution_data>(*resolution);
        // All we need is the ID of the calculation.
        return calc_resolution.id.is_ready();
      }
     case request_type::OBJECT:
        // All we need is the ID of the data, which is the actual request.
        return true;
     case request_type::IMMUTABLE:
         // All we need is the immutable ID of the data, which is the actual request.
         return true;
     default:
        // Otherwise, we need the value, so we use result_is_resolved.
        return result_is_resolved(resolution, original_request);
    }
}

untyped_request static
get_preresolved_result(
    background_request_resolution_data* resolution,
    untyped_request const& request)
{
    switch (request.type)
    {
     case request_type::REMOTE_CALCULATION:
     case request_type::META:
      {
        auto& calc_resolution =
            cast_resolution_data<remote_calc_resolution_data>(*resolution);
        // Just reference the calculation's result as an object.
        return
            make_untyped_request(request_type::OBJECT,
                string(get(cast_immutable<string>(calc_resolution.id.data()))),
                request.result_interface);
      }
     case request_type::OBJECT:
      {
        // This is already considered preresolved.
        return request;
      }
     case request_type::IMMUTABLE:
      {
        // This is already considered preresolved.
        return request;
      }
     default:
      {
        // Anything else is recorded as a value.
        return
            make_untyped_request(request_type::IMMEDIATE,
                get_result(resolution, request), request.result_interface);
      }
    }
}

bool static
objectification_complete(
    background_request_resolution_data* resolution,
    untyped_request const& original_request)
{
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    if (!preresolved_request)
        return false;
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::OBJECT:
        return true;
     case request_type::IMMUTABLE:
        return true;
     case request_type::REMOTE_CALCULATION:
     case request_type::META:
      {
        auto& calc_resolution =
            cast_resolution_data<remote_calc_resolution_data>(*resolution);
        return calc_resolution.id.is_ready();
      }
     case request_type::IMMEDIATE:
        return true;
     case request_type::FUNCTION:
        return local_calc_objectification_complete(resolution, request);
     case request_type::ARRAY:
        return array_objectification_complete(resolution, request);
     case request_type::STRUCTURE:
        return structure_objectification_complete(resolution, request);
     case request_type::PROPERTY:
        return
            objectification_complete(resolution,
                as_property(request).record);
     case request_type::UNION:
        return
            objectification_complete(resolution,
                as_union(request).member_request);
     case request_type::SOME:
        return
            objectification_complete(resolution,
                as_some(request).value);
     case request_type::REQUIRED:
        return
            objectification_complete(resolution,
                as_required(request).optional_value);
     case request_type::ISOLATED:
        // These should already have been eliminated by now.
        assert(0);
        throw exception("internal error: unresolved isolated request");
     default:
        assert(0);
        throw exception("internal error: invalid request type");
    }
}

untyped_request static
get_objectified_form(
    background_request_resolution_data* resolution,
    untyped_request const& original_request)
{
    auto const* preresolved_request =
        get_preresolved_request(&resolution->preresolution, original_request);
    assert(preresolved_request);
    auto const& request = *preresolved_request;

    switch (request.type)
    {
     case request_type::OBJECT:
         return request;
     case request_type::IMMUTABLE:
        return request;
     case request_type::REMOTE_CALCULATION:
     case request_type::META:
      {
        auto& calc_resolution =
            cast_resolution_data<remote_calc_resolution_data>(*resolution);
        auto& result_id = calc_resolution.id.data();
        assert(is_initialized(result_id));
        return
            make_untyped_request(
                request_type::OBJECT,
                string(get(cast_immutable<string>(result_id))),
                request.result_interface);
      }
     case request_type::IMMEDIATE:
        return request;
     case request_type::FUNCTION:
        return get_local_calc_objectified_form(resolution, request);
     case request_type::ARRAY:
        return get_array_objectified_form(resolution, request);
     case request_type::STRUCTURE:
        return get_structure_objectified_form(resolution, request);
     case request_type::PROPERTY:
      {
        auto const& info = as_property(request);
        property_request_info new_info;
        new_info.extractor = info.extractor;
        new_info.field = info.field;
        new_info.record =
            get_objectified_form(resolution, info.record);
        return replace_request_contents(request, new_info);
      }
     case request_type::UNION:
      {
        auto const& info = as_union(request);
        union_request_info new_info;
        new_info.constructor = info.constructor;
        new_info.member_name = info.member_name;
        new_info.member_request =
            get_objectified_form(resolution, info.member_request);
        return replace_request_contents(request, new_info);
      }
     case request_type::SOME:
      {
        auto const& info = as_some(request);
        some_request_info new_info;
        new_info.value =
            get_objectified_form(resolution, info.value);
        new_info.wrapper = info.wrapper;
        return replace_request_contents(request, new_info);
      }
     case request_type::REQUIRED:
      {
        auto const& info = as_required(request);
        required_request_info new_info;
        new_info.optional_value =
            get_objectified_form(resolution, info.optional_value);
        new_info.unwrapper = info.unwrapper;
        return replace_request_contents(request, new_info);
      }
     case request_type::ISOLATED:
        // These should already have been eliminated by now.
        assert(0);
        throw exception("internal error: unresolved isolated request");
     default:
        assert(0);
        throw exception("internal error: invalid request type");
    }
}

// Try resolving a request immediately, in the foreground.
// If it's too expensive to do so, this returns false.
bool static
try_immediate_resolution(
    alia__shared_ptr<background_execution_system> const& bg,
    framework_context const& context,
    untyped_immutable* result,
    optional<untyped_request>* objectified_form,
    untyped_request const& request,
    background_request_interest_type interest)
{
    // I've disabled this check for now because it's preventing image slices
    // from loading immediately, and I'm not sure it's even necessary.
    //if (compute_request_complexity(request) > 100)
    //    return false;
    background_request_resolution_data resolution;
    update_resolution(bg, context, &resolution, request, true, interest);
    switch (interest)
    {
     case background_request_interest_type::RESULT:
        if (result_is_resolved(&resolution, request))
        {
            *result = get_result(&resolution, request);
            return true;
        }
        return false;
     case background_request_interest_type::OBJECTIFIED_FORM:
        if (objectification_complete(&resolution, request))
        {
            *objectified_form = get_objectified_form(&resolution, request);
            return true;
        }
        return false;
     default:
        assert(0);
        return false;
    }
}

// BACKGROUND REQUEST SYSTEM

struct background_request_item
{
    owned_id requester_id;
    framework_context context;
    untyped_request request;
    background_request_interest_type interest;
    background_job_controller* controller;
};

enum class background_request_update_type
{
    OBJECTIFIED_FORM,
    RESULT
};

struct background_request_update_item
{
    owned_id requester_id;
    background_request_update_type type;
    // valid for OBJECTIFIED_FORM type
    optional<untyped_request> objectified_form;
    // valid for RESULT type
    untyped_immutable result;
};

typedef synchronized_queue<background_request_update_item>
    background_request_update_queue;

struct background_request_system_data
{
    alia__shared_ptr<background_execution_system> execution_system;

    // updates are posted here by background jobs
    alia__shared_ptr<background_request_update_queue> shared_update_queue;

    // background_request_ptrs (which are local to the same thread as the
    // background_request_system) use these to post requests and check for
    // updates.
    std::queue<background_request_item> local_request_queue;

    // local_update_queue can't be a std::queue because background_request_ptrs
    // need to iterate over it.
    std::vector<background_request_update_item> local_update_queue;
};

background_request_system::~background_request_system()
{
    delete data_;
}

void initialize_background_request_system(
    background_request_system& request_system,
    alia__shared_ptr<background_execution_system> const& execution_system)
{
    auto* data = new background_request_system_data;
    data->execution_system = execution_system;
    data->shared_update_queue.reset(new background_request_update_queue);
    request_system.data_ = data;
}

struct request_resolution_job : background_job_interface
{
    request_resolution_job(
        alia__shared_ptr<background_execution_system> const& bg,
        alia__shared_ptr<background_request_update_queue> const& update_queue,
        background_request_item const& request)
      : bg_(bg)
      , update_queue_(update_queue)
      , request_(request)
      , sent_objectified_form_(false)
    {}

    void gather_inputs()
    {
        update_resolution(bg_, request_.context, &resolution_,
            request_.request, false, request_.interest);
    }

    bool inputs_ready()
    {
        switch (request_.interest)
        {
         case background_request_interest_type::RESULT:
            return result_is_resolved(&resolution_, request_.request);
         case background_request_interest_type::OBJECTIFIED_FORM:
            return objectification_complete(&resolution_, request_.request);
         default:
            assert(0);
            return false;
        }
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        switch (request_.interest)
        {
         case background_request_interest_type::RESULT:
          {
            background_request_update_item update_item;
            update_item.type = background_request_update_type::RESULT;
            update_item.requester_id = request_.requester_id;
            update_item.result = get_result(&resolution_, request_.request);
            assert(is_initialized(update_item.result));
            push(*update_queue_, update_item);
            break;
          }
         case background_request_interest_type::OBJECTIFIED_FORM:
          {
            background_request_update_item update_item;
            update_item.type =
                background_request_update_type::OBJECTIFIED_FORM;
            update_item.requester_id = request_.requester_id;
            update_item.objectified_form =
                get_objectified_form(&resolution_, request_.request);
            assert(update_item.objectified_form);
            push(*update_queue_, update_item);
            break;
          }
         default:
          assert(0);
        }
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "request"; // TODO
        return info;
    }

 private:
    alia__shared_ptr<background_execution_system> bg_;
    alia__shared_ptr<background_request_update_queue> update_queue_;
    background_request_resolution_data resolution_;
    background_request_item request_;
    bool sent_objectified_form_;
};

std::vector<untyped_request>
issue_new_requests(background_request_system& request_system)
{
    std::vector<untyped_request> req_list;
    auto& data = *request_system.data_;

    // Create jobs to service any new requests.
    {
        auto& queue = data.local_request_queue;
        while (!queue.empty())
        {
            auto const& request = queue.front();
            add_background_job(*data.execution_system,
                background_job_queue_type::CALCULATION,
                request.controller,
                new request_resolution_job(
                    data.execution_system,
                    data.shared_update_queue,
                    request),
                BACKGROUND_JOB_HIDDEN);

            req_list.push_back(request.request);

            queue.pop();
        }
    }
    return req_list;
}

void gather_updates(background_request_system& request_system)
{
    auto& data = *request_system.data_;

    // Transfer new updates from the shared queue to the local one.
    process_queue_items(*data.shared_update_queue,
        [&](background_request_update_item const& update)
        {
            data.local_update_queue.push_back(update);
        });
}

void clear_updates(background_request_system& request_system)
{
    auto& data = *request_system.data_;

    // Clear out the updates from the last update.
    // (We assume that all background_request_ptrs associated with this request
    // system will check for updates between updates. This may have to be
    // revisited.)
    data.local_update_queue.clear();
}

// BACKGROUND REQUEST POINTERS

void background_request_ptr::reset(
    background_request_system& system,
    id_interface const& requester_id,
    framework_context const& context,
    untyped_request const& request,
    background_request_interest_type interest)
{
    // Reset members to new request.
    this->reset();
    system_ = &system;
    requester_id_.store(requester_id);
    context_ = context;
    interest_ = interest;

    // Try doing an immediate resolution of the request, and if that fails,
    // add it to the system's local request queue.
    if (try_immediate_resolution(system.data_->execution_system, context,
            &result_, &objectified_form_, request, interest))
    {
        is_resolved_ = true;
    }
    else
    {
        background_request_item request_item;
        request_item.requester_id = requester_id_;
        request_item.context = context;
        request_item.request = request;
        request_item.interest = interest;
        request_item.controller = &controller_;
        system_->data_->local_request_queue.push(request_item);
    }
}

void background_request_ptr::reset()
{
    if (system_)
    {
        requester_id_.clear();
        is_resolved_ = false;
        cradle::reset(result_);
        controller_.cancel();
        controller_.reset();
        objectified_form_ = none;
        system_ = 0;
    }
}

void background_request_ptr::update()
{
    // If we don't already have a result, check the system for updates matching
    // this request.
    if (this->is_initialized() && !this->is_resolved())
    {
        auto& queue = system_->data_->local_update_queue;
        for (auto i = queue.begin(); i != queue.end(); ++i)
        {
            if (i->requester_id.matches(requester_id_.get()))
            {
                switch (i->type)
                {
                 case background_request_update_type::OBJECTIFIED_FORM:
                    assert(this->interest() ==
                        background_request_interest_type::OBJECTIFIED_FORM);
                    this->objectified_form_ = i->objectified_form;
                    this->is_resolved_ = true;
                    break;
                 case background_request_update_type::RESULT:
                    assert(this->interest() ==
                        background_request_interest_type::RESULT);
                    swap(this->result_, i->result);
                    this->is_resolved_ = true;
                    break;
                }
            }
        }
    }
}

void background_request_ptr::swap_with(background_request_ptr& other)
{
    using std::swap;
    swap(requester_id_, other.requester_id_);
    swap(interest_, other.interest_);
    swap(context_, other.context_);
    swap(is_resolved_, other.is_resolved_);
    swap(result_, other.result_);
    swap(controller_, other.controller_);
    swap(objectified_form_, other.objectified_form_);
}

// META-LIKE REQUESTS

optional<string>
perform_meta_request_by_id(
    alia__shared_ptr<background_execution_system> const& bg,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    string const& generator_id,
    bool dry_run)
{
    // Check the disk cache.
    auto disk_cache_key = context.context_id + "/meta/" + generator_id;
    {
        auto& disk_cache = *get_disk_cache(*bg);
        int64_t entry;
        uint32_t entry_crc;
        if (entry_exists(disk_cache, disk_cache_key, &entry, &entry_crc))
        {
            try
            {
                record_usage(disk_cache, entry);
                value cached_value;
                uint32_t file_crc;
                read_value_file(&cached_value,
                    get_path_for_id(disk_cache, entry),
                    &file_crc);
                if (file_crc != entry_crc)
                    throw crc_error();
                return some(from_value<string>(cached_value));
            }
            catch (...)
            {
                // If the disk cache read fails, just do the actual request...
            }
        }
    }

    // Wait for the generator calculation to finish.
    {
        null_check_in check_in;
        null_progress_reporter reporter;
        wait_for_remote_calculation(check_in, reporter, connection, context,
            session, generator_id);
    }

    // Issue the generated calculation via ID.
    optional<string> calculation_id;
    {
        auto calc_request =
            web_request(web_request_method::POST,
                context.framework.api_url + "/calc/" + generator_id +
                    "?context=" + context.context_id +
                    "&dry_run=" + (dry_run ? "true" : "false"),
                blob(),
                no_headers);
        null_check_in check_in;
        null_progress_reporter reporter;
        auto response =
            parse_json_response(
                perform_web_request(check_in, reporter, connection, session,
                    calc_request));
        calculation_id =
            dry_run
          ? from_value<optional<string> >(response)
          : some(from_value<calculation_request_response>(response).id);
    }

    // If the result contains an ID, cache it to disk.
    if (calculation_id)
    {
        auto& disk_cache = *get_disk_cache(*bg);
        int64_t entry = initiate_insert(disk_cache, disk_cache_key);
        uint32_t crc;
        write_value_file(get_path_for_id(disk_cache, entry),
            to_value(string(get(calculation_id))), &crc);
        finish_insert(disk_cache, entry, crc);
    }

    return calculation_id;
}

// Get the calculation ID associated with a Thinknode calculation, using the
// memory cache and disk cache.
optional<string> static
get_thinknode_calculation_id(
    alia__shared_ptr<background_execution_system> const& bg,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& request,
    bool dry_run)
{
    // If the calculation is simply a reference, just return the ID directly.
    if (request.type == calculation_request_type::REFERENCE)
        return as_reference(request);

    // Try the memory cache.
    //
    // Note that a background_data_ptr's scope should really be the entire
    // time that the application is interested in its result, so ideally this
    // should live beyond this function call, but that would considerably
    // complicate the interface of this function and those that call it, and
    // since the memory cache keeps around recently used entries anyway, it's
    // probably not worth it.
    //
    auto memory_cache_id = make_id_by_reference(request);
    {
        background_data_ptr<string> calc_id(*bg, memory_cache_id);
        if (calc_id.is_ready())
            return some(*calc_id);
    }

    // Otherwise, try loading it from the disk cache.
    auto disk_cache_key = context.context_id + "/calc/" +
        value_to_base64_string(to_value(request));
    auto& disk_cache = *get_disk_cache(*bg);
    {
        int64_t entry;
        uint32_t entry_crc;
        if (entry_exists(disk_cache, disk_cache_key, &entry, &entry_crc))
        {
            try
            {
                record_usage(disk_cache, entry);
                value cached_value;
                uint32_t file_crc;
                read_value_file(&cached_value,
                    get_path_for_id(disk_cache, entry),
                    &file_crc);
                if (file_crc != entry_crc)
                    throw crc_error();
                auto calculation_id = from_value<string>(cached_value);
                // Write it to the memory cache.
                set_cached_data(*bg, memory_cache_id,
                    erase_type(make_immutable(calculation_id)));
                return some(calculation_id);
            }
            catch (...)
            {
                // If the disk cache read fails, just do the actual request...
            }
        }
    }

    // If we haven't found it yet, we have to go to Thinknode.
    null_check_in check_in;
    auto calculation_id =
        dry_run
      ? request_dry_run_calculation(
            check_in, connection, context, session, request)
      : some(
            request_remote_calculation(
                check_in, connection, context, session, request));

    // If the result contains an ID, cache it.
    if (calculation_id)
    {
        // Write it to the memory cache.
        set_cached_data(*bg, memory_cache_id,
            erase_type(make_immutable(string(get(calculation_id)))));
        // Write it to the disk cache.
        int64_t entry = initiate_insert(disk_cache, disk_cache_key);
        uint32_t crc;
        write_value_file(get_path_for_id(disk_cache, entry),
            to_value(string(get(calculation_id))), &crc);
        finish_insert(disk_cache, entry, crc);
    }

    return calculation_id;
}

// Substitute the variables in a Thinknode request for new requests.
calculation_request static
substitute_variables(
    std::map<string,calculation_request> const& substitutions,
    calculation_request const& request)
{
    auto recursive_call =
        [&substitutions](calculation_request const& request)
        {
            return substitute_variables(substitutions, request);
        };
    switch (request.type)
    {
     case calculation_request_type::ARRAY:
        return
            make_calculation_request_with_array(
                calculation_array_request(
                    map(recursive_call, as_array(request).items),
                    as_array(request).item_schema));
     case calculation_request_type::FUNCTION:
        return
            make_calculation_request_with_function(
                function_application(
                    as_function(request).account,
                    as_function(request).app,
                    as_function(request).name,
                    map(recursive_call, as_function(request).args),
                    as_function(request).level));
     case calculation_request_type::LET:
        throw exception("internal error: encountered let request during variable substitution");
     case calculation_request_type::META:
        return
            make_calculation_request_with_meta(
                meta_calculation_request(
                    recursive_call(as_meta(request).generator),
                    as_meta(request).schema));
     case calculation_request_type::OBJECT:
        return
            make_calculation_request_with_object(
                calculation_object_request(
                    map(recursive_call, as_object(request).properties),
                    as_object(request).schema));
     case calculation_request_type::PROPERTY:
        return
            make_calculation_request_with_property(
                calculation_property_request(
                    recursive_call(as_property(request).object),
                    as_property(request).schema,
                    as_property(request).field));
     case calculation_request_type::REFERENCE:
     case calculation_request_type::VALUE:
        return request;
     case calculation_request_type::VARIABLE:
      {
        auto substitution = substitutions.find(as_variable(request));
        if (substitution == substitutions.end())
            throw exception("internal error: missing variable substitution");
        return substitution->second;
      }
     default:
        throw exception("internal error: invalid Thinknode calculation request");
    }
}

optional<let_calculation_submission_info>
submit_let_calculation_request(
    alia__shared_ptr<background_execution_system> const& bg,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    augmented_calculation_request const& augmented_request,
    bool dry_run)
{
    let_calculation_submission_info result;

    // We expect this request to be series of nested let requests (since this
    // is what as_compact_thinknode_request constructs), so we'll deconstruct
    // that one-by-one, submitting the requests and recording the
    // substitutions...

    std::map<string,calculation_request> substitutions;

    // current_request stores a pointer into the full request that indicates
    // how far we've unwrapped it.
    calculation_request const* current_request = &augmented_request.request;

    while (current_request->type == calculation_request_type::LET)
    {
        auto const& let = as_let(*current_request);

        // Loop through all the variables (there should only be one, given
        // how as_compact_thinknode_request constructs things).
        for (auto const& var : let.variables)
        {
            // Apply the existing substitutions and submit the request.
            auto calculation_id =
                get_thinknode_calculation_id(bg, connection, context, session,
                    substitute_variables(substitutions, var.second), dry_run);

            // If there's no calculation ID, then this must be a dry run that
            // hasn't been done yet, so the whole result is none.
            if (!calculation_id)
                return none;

            // We got a calculation ID, so record the new substitution.
            substitutions[var.first] =
                make_calculation_request_with_reference(get(calculation_id));

            // If this is a reported variable, record it.
            auto const& reported = augmented_request.reported_variables;
            if (std::find(reported.begin(), reported.end(), var.first) !=
                reported.end())
            {
                result.reported_subcalcs.push_back(
                    reported_calculation_info(
                        get(calculation_id),
                        // We assume that all reported calculations are
                        // function calls.
                        var.second.type == calculation_request_type::FUNCTION ?
                        as_function(var.second).name :
                        "internal error: unrecognized reported calc"));
            }
            // Otherwise, just record its ID.
            else
            {
                result.other_subcalc_ids.push_back(get(calculation_id));
            }
        }

        // Proceed to the next level of nesting.
        current_request = &let.in;
    }

    // Now we've made it to the actual request, so again apply the
    // substitutions and submit it.
    auto calc_request = substitute_variables(substitutions, *current_request);
    auto main_calc_id =
        get_thinknode_calculation_id(bg, connection, context, session,
            calc_request, dry_run);
    if (!main_calc_id)
        return none;

    // Check the status of the calculation. If it was canceled, then resubmit
    // it manually
    null_check_in check_in;
    null_progress_reporter reporter;
    auto status =
        cradle::from_value<calculation_status>(
            parse_json_response(
                perform_web_request(
                    check_in,
                    reporter,
                    connection,
                    session,
                    make_get_request(
                        context.framework.api_url + "/calc/" +
                            get(main_calc_id) + "/status?context=" +
                        context.context_id,
                        no_headers))));

    if (status.type == calculation_status_type::CANCELED)
    {
        if (dry_run)
        {
            return none;
        }

        auto request =
            cradle::from_value<calculation_request>(
                parse_json_response(
                    perform_web_request(
                        check_in,
                        reporter,
                        connection,
                        session,
                        make_get_request(
                            context.framework.api_url + "/calc/" +
                                get(main_calc_id) + "?context=" +
                            context.context_id,
                            no_headers))));
        main_calc_id =
            request_remote_calculation(
                check_in, connection, context, session,
                request);

    }

    result.main_calc_id = get(main_calc_id);

    return result;
}

}
