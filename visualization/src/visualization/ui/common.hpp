#ifndef VISUALIZATION_UI_COMMON_HPP
#define VISUALIZATION_UI_COMMON_HPP

#include <visualization/common.hpp>
#include <cradle/gui/common.hpp>

namespace visualization {

// The current display library wants to have a display context associated with
// everything, but that actually seems like it might be more trouble than it's
// worth. It complicates mixing views from different sources. So for now, I've
// defined a null_display_context that can be used where we don't actually want
// a display context. Maybe at some point, display contexts should be removed
// entirely...
struct null_display_context {};

// Refresh a cloned accessor value to bring it back in sync with the original.
template<class T>
void refresh_accessor_clone(keyed_data<T>& clone, accessor<T> const& original)
{
    refresh_keyed_data(clone, original.id());
    if (!is_valid(clone) && is_gettable(original))
        set(clone, get(original));
}

// Make a readonly accesssor to const keyed_data.
template<class T>
auto
make_accessor(keyed_data<T> const* keyed)
{
    return make_readonly(make_accessor(const_cast<keyed_data<T>*>(keyed)));
}

// This is the equivalent of gui_map, but for scene graphs.
// Since scene graphs are built up as a linked list of pointers to interfaces,
// it can be difficult to get data related to a scene graph into a form that
// can be used with normal UI functions (which expect everything to be
// accessors). This function attempts to address that.
//
// gui_map_scene_graph(ctx, fn, objects) will call fn(ctx, object) for
// each object in the linked list :objects.
//
// fn(ctx, object) is expected to return an accessor, and once this accessor
// is gettable for all objects, the values of those accessors are collected
// together into a vector and returned (in accessor form) to the caller.
//
template<class MappedItem, class Fn, class Object>
indirect_accessor<std::vector<MappedItem> >
gui_map_scene_graph(gui_context& ctx, Fn const& fn, Object const* objects)
{
    // First determine the size of the linked list.
    size_t n_items = 0;
    for (auto const* i = objects; i; i = i->next_)
    {
        ++n_items;
    }

    // Get our cached data and check if it needs to be reset.
    struct cached_data
    {
        std::vector<owned_id> mapped_ids;
        std::vector<MappedItem> mapped_items;
        local_identity abbreviated_identity;
    };
    cached_data* data;
    get_cached_data(ctx, &data);
    if (data->mapped_ids.size() != n_items)
    {
        data->mapped_ids.resize(n_items);
        data->mapped_items.resize(n_items);
        inc_version(data->abbreviated_identity);
    }

    // Now loop through all the objects and invoke :fn on each.
    size_t n_valid_items = 0;
    // The index goes backwards because the objects are added in reverse
    // order, so this gets the resulting map back in order.
    size_t index = n_items - 1;
    naming_context nc(ctx);
    for (auto const* i = objects; i; i = i->next_)
    {
        named_block nb(nc, get_id(i->id_));
        auto mapped_item = fn(ctx, *i);
        if (is_gettable(mapped_item))
        {
            if (!data->mapped_ids[index].matches(mapped_item.id()))
            {
                data->mapped_items[index] = get(mapped_item);
                data->mapped_ids[index].store(mapped_item.id());
                inc_version(data->abbreviated_identity);
            }
            ++n_valid_items;
        }
        --index;
    }

    // Initialize an empty result as a fallback.
    accessor<std::vector<MappedItem> > const* result =
        erase_type(ctx, empty_accessor<std::vector<MappedItem> >());
    // If all the mapped items are valid, use the full result.
    alia_if (n_valid_items == n_items)
    {
        result =
            erase_type(ctx,
                make_custom_getter(&data->mapped_items,
                    optimize_id_equality(
                        id_array(&data->mapped_ids, false),
                        get_id(data->abbreviated_identity))));
    }
    alia_end
    return ref(result);
}

}

#endif
