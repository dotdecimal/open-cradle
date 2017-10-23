#ifndef CRADLE_IO_CALC_REQUESTS_HPP
#define CRADLE_IO_CALC_REQUESTS_HPP

#include <cradle/common.hpp>
#include <cradle/api.hpp>
#include <cradle/composition_cache.hpp>

// This file defines the Thinknode representation of calculation requests.
// It's kept outside of cradle/io/services because it's useful in contexts
// where that code isn't included.

namespace cradle {

struct calculation_request;
struct calculation_array_request;
struct calculation_object_request;
struct calculation_property_request;
struct meta_calculation_request;
struct let_calculation_request;

api(struct)
struct function_application
{
    string account, app, name;
    std::vector<calculation_request> args;
    omissible<int> level;
};

api(union)
union calculation_request
{
    function_application function;
    cradle::value value;
    string reference;
    calculation_array_request array;
    calculation_object_request object;
    calculation_property_request property;
    meta_calculation_request meta;
    let_calculation_request let;
    string variable;
};

api(struct)
struct calculation_array_request
{
    std::vector<calculation_request> items;
    api_type_info item_schema;
};

api(struct)
struct calculation_object_request
{
    std::map<string,calculation_request> properties;
    api_type_info schema;
};

api(struct)
struct calculation_property_request
{
    calculation_request object;
    api_type_info schema;
    calculation_request field;
};

api(struct)
struct meta_calculation_request
{
    calculation_request generator;
    api_type_info schema;
};

api(struct)
struct let_calculation_request
{
    std::map<string,calculation_request> variables;
    calculation_request in;
};

// as_thinknode_request(r) returns the Thinknode representation of r.
calculation_request
as_thinknode_request(untyped_request const& request);

// same, but for typed requests
template<class Value>
calculation_request
as_thinknode_request(request<Value> const& request)
{
    return as_thinknode_request(request.untyped);
}

// untemplated helper for as_compact_thinknode_request, below - Note that if
// :reported_variables is supplied, it will be filled in with a list of the
// variable names corresponding to calculations marked as 'reported'.
calculation_request
generate_compact_thinknode_request(
    composition_cache_entry_map const& cache_entries,
    std::vector<id_interface const*> const& order_added,
    untyped_request const& request,
    std::vector<string>* reported_variables = nullptr);

// as_compact_thinknode_request(cache, r) generates a compact version of the
// Thinknode request for r by using the cache as an indication of which
// subrequests are reused frequently within r and inserting 'let' requests
// with variables to represent those subrequests.
template<class DataContext>
calculation_request
as_compact_thinknode_request(
    composition_cache<DataContext>& cache,
    untyped_request const& request)
{
    return
        generate_compact_thinknode_request(
            cache.entries, cache.order_added, request);
}

// same, but for typed requests
template<class DataContext, class Value>
calculation_request
as_compact_thinknode_request(
    composition_cache<DataContext>& cache,
    request<Value> const& request)
{
    return as_compact_thinknode_request(cache, request.untyped);
}

// This augments a normal Thinknode calculation request with extra information
// that's useful for status reporting.
api(struct internal)
struct augmented_calculation_request
{
    calculation_request request;
    std::vector<string> reported_variables;
};

// This is identical to as_compact_thinknode_request, but it returns an
// augmented_calculation_request.
template<class DataContext>
augmented_calculation_request
as_augmented_thinknode_request(
    composition_cache<DataContext>& cache,
    untyped_request const& request)
{
    augmented_calculation_request result;
    result.request =
        generate_compact_thinknode_request(
            cache.entries,
            cache.order_added,
            request,
            &result.reported_variables);
    return result;
}

// same, but for typed requests
template<class DataContext, class Value>
augmented_calculation_request
as_augmented_thinknode_request(
    composition_cache<DataContext>& cache,
    request<Value> const& request)
{
    return as_augmented_thinknode_request(cache, request.untyped);
}

}

#endif
