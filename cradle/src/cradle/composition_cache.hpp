#ifndef CRADLE_COMPOSITION_CACHE_HPP
#define CRADLE_COMPOSITION_CACHE_HPP

#include <boost/unordered_map.hpp>

#include <cradle/common.hpp>

namespace cradle {

// When composition calculation requests, the same exact subrequests tend to
// get reused repeatedly in different parts of the three. However, it's
// difficult to avoid rebuilding those requests over and over without
// diverting substantially from a purely functional, bottom-up style of
// composition.
//
// A composition_cache is designed to solve that problem. It caches the results
// of the function calls used to compose subrequests. When a composition
// function needs a subrequest, instead of directly invoking the function to
// compose it, it asks the cache for a result. If the cache has seem the exact
// same function applied to the exact same arguments before, it will simply
// return the recorded result.
//
// Since request composition is always done within a single "data context", and
// since that data context tends to supply the vast majority of the data passed
// into each function call, the data context is included as a "free" parameter.
// A composition_cache is always associated with a single data context, and the
// data context is NOT considered one of the arguments passed into the composer
// functions.

struct composition_cache_entry
{
    untyped_request result;

    // If this is true, the entry is currently being composed. (i.e., We are
    // inside the function call to compose this entry.)
    bool composing;

    // call_id identifies the call that generated this result.
    // It's a combination of the ID of the function (ID'd by pointer) and the
    // arguments (ID'd by their actual values).
    owned_id call_id;
};

// composition_cache_entry_map maps composition calls to the resulting requests
typedef boost::unordered_map<id_interface const*,composition_cache_entry,
    id_interface_pointer_hash,id_interface_pointer_equality_test>
    composition_cache_entry_map;

template<class DataContext>
struct composition_cache
{
    // the data context that this cache is associated with
    DataContext const* context;

    // cache entries
    composition_cache_entry_map entries;

    // the order that the cache entry results were added - If entries are
    // traversed in this order, all requests are guaranteed to be visited
    // before the higher-level requests that use them.
    std::vector<id_interface const*> order_added;

    // no default constructor because the data context is required
    explicit composition_cache(DataContext const& context)
      : context(&context)
    {}
};

// Request composers call this to get the data context associated with the
// cache.
template<class DataContext>
DataContext const&
get_data_context(composition_cache<DataContext> const& cache)
{
    return *cache.context;
}

// Cast a composition_cache's data context to a different type.
// FromDataContext must be a subtype of ToDataContext.
template<class ToDataContext, class FromDataContext>
composition_cache<ToDataContext>&
composition_cache_cast(composition_cache<FromDataContext>& from)
{
    // Do a static cast of the data context pointer just to check that the
    // types are compatible.
    static_cast<ToDataContext const*>(from.context);
    // Once we know the types are compatible, we can simply reinterpret one
    // type as the other.
    return reinterpret_cast<composition_cache<ToDataContext>&>(from);
}

// Common code for invoking a composer function using the cache.
// (This is independent of the number of arguments.)
#define CRADLE_INVOKE_COMPOSER(generate_id, call_function) \
    /* Generate the ID for this call and see if it's in the cache. */ \
    auto call_id = generate_id; \
    auto entry = cache.entries.find(&call_id); \
    if (entry == cache.entries.end()) \
    { \
        /* If it's not there, we need to call the function and cache its
           result. Note that we insert the entry before calling the function
           that is meant to compose it. This is so we can detect attempts to
           look up this entry from within the call (which would otherwise
           result in infinite recursion). */ \
        composition_cache_entry new_entry; \
        new_entry.composing = true; \
        /* The key we're storing is a pointer to the ID, so we have to store
           the actual ID in the entry itself. */ \
        new_entry.call_id.store(call_id); \
        entry = \
            cache.entries.insert( \
                composition_cache_entry_map::value_type( \
                    &new_entry.call_id.get(), \
                    new_entry)).first; \
        /* Now call the function to compose this result. */ \
        entry->second.result = call_function.untyped; \
        entry->second.composing = false; \
        /* :order_added should reflect the order that the RESULTS are added,
           so we need to do it here because other results might be added
           within the call to compose this result. */ \
        cache.order_added.push_back(&new_entry.call_id.get()); \
    } \
    /* If this entry is still flagged as being composed, then this is an
       attempt to use this entry's result to construct the result itself,
       which is infinite recursion, so throw an exception. */ \
    if (entry->second.composing) \
    { \
        throw exception("infinitely recursive request composition detected"); \
    } \
    decltype(call_function) result; \
    result.untyped = entry->second.result; \
    return result;

// Invoke a function f with the given argument(s) using the given cache.
// 0 args
template<class DataContext, class Function>
auto
invoke_composer(
    Function const& f,
    composition_cache<DataContext>& cache)
{
    CRADLE_INVOKE_COMPOSER(
        make_id(f),
        f(cache))
}
// 1 arg
template<class DataContext, class Function, class Arg0>
auto
invoke_composer(
    Function const& f,
    composition_cache<DataContext>& cache,
    Arg0 const& arg0)
{
    CRADLE_INVOKE_COMPOSER(
        combine_ids(make_id(f), make_id_by_reference(arg0)),
        f(cache, arg0))
}
// 2 args
template<class DataContext, class Function, class Arg0, class Arg1>
auto
invoke_composer(
    Function const& f,
    composition_cache<DataContext>& cache,
    Arg0 const& arg0,
    Arg1 const& arg1)
{
    CRADLE_INVOKE_COMPOSER(
        combine_ids(
            combine_ids(make_id(f), make_id_by_reference(arg0)),
            make_id_by_reference(arg1)),
        f(cache, arg0, arg1))
}
// 6 args
template<class DataContext, class Function, class Arg0, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
auto
invoke_composer(
    Function const& f,
    composition_cache<DataContext>& cache,
    Arg0 const& arg0,
    Arg1 const& arg1,
    Arg2 const& arg2,
    Arg3 const& arg3,
    Arg4 const& arg4,
    Arg5 const& arg5)
{
    CRADLE_INVOKE_COMPOSER(
        combine_ids(
            combine_ids(
                combine_ids(
                    combine_ids(
                        combine_ids(
                            combine_ids(
                                make_id(f),
                                make_id_by_reference(arg0)),
                            make_id_by_reference(arg1)),
                        make_id_by_reference(arg2)),
                    make_id_by_reference(arg3)),
                make_id_by_reference(arg4)),
            make_id_by_reference(arg5)),
        f(cache, arg0, arg1, arg2, arg3, arg4, arg5))
}

// invoke_leaf_composer is the same, but it just passes the cache's data
// context. (The composer that's invoked won't be able to do any caching, so
// it's hopefully not calling other composition functions, which is why it's
// considered a leaf.)
// 0 args
template<class DataContext, class Function>
auto
invoke_leaf_composer(
    Function const& f,
    composition_cache<DataContext>& cache)
{
    CRADLE_INVOKE_COMPOSER(
        make_id(f),
        f(get_data_context(cache)))
}
// 1 arg
template<class DataContext, class Function, class Arg0>
auto
invoke_leaf_composer(
    Function const& f,
    composition_cache<DataContext>& cache,
    Arg0 const& arg0)
{
    CRADLE_INVOKE_COMPOSER(
        combine_ids(make_id(f), make_id_by_reference(arg0)),
        f(get_data_context(cache), arg0))
}
// 2 args
template<class DataContext, class Function, class Arg0, class Arg1>
auto
invoke_leaf_composer(
    Function const& f,
    composition_cache<DataContext>& cache,
    Arg0 const& arg0,
    Arg1 const& arg1)
{
    CRADLE_INVOKE_COMPOSER(
        combine_ids(
            combine_ids(make_id(f), make_id_by_reference(arg0)),
            make_id_by_reference(arg1)),
        f(get_data_context(cache), arg0, arg1))
}

// The #define is an implementation detail, so clean it up.
#undef CRADLE_INVOKE_COMPOSER

// wrap_with_cacher(f) takes a composition function that expects a
// composition_cacher as its first argument and converts it to one that takes
// the corresponding data context as its first argument. The wrapper will
// create a local composition_cache for the function to use. This is designed
// to alter the interface of a composition function to what an external caller
// might expect, and since it creates a local cache specifically for this
// function, it should really only be used when the composition function is
// called in isolation.
template<class Function>
struct composition_cache_wrapper
{
    composition_cache_wrapper(Function function)
      : function_(function)
    {}

    template<class DataContext, class... Args>
    auto
    operator()(DataContext const& data_ctx, Args... args) const
    {
        composition_cache<DataContext> cache(data_ctx);
        return function_(cache, args...);
    }

    Function function_;
};
template<class Function>
composition_cache_wrapper<Function>
wrap_with_cacher(Function function)
{
    return composition_cache_wrapper<Function>(function);
}

}

#endif
