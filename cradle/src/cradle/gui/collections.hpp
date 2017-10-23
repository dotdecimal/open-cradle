#ifndef CRADLE_GUI_COLLECTIONS_HPP
#define CRADLE_GUI_COLLECTIONS_HPP

#include <cradle/gui/common.hpp>

namespace cradle {

template<class MappedItem>
struct gui_map_to_vector_data
{
    owned_id input_id;
    std::vector<MappedItem> mapped_items;
    std::vector<owned_id> mapped_ids;
    unsigned n_valid_items;
    local_identity abbreviated_identity;
};

struct id_array : id_interface
{
    id_array() : ids_(0), owner_(false) {}

    id_array(id_array const& other)
    {
        owner_ = other.owner_;
        if (owner_)
        {
            ids_ = new std::vector<owned_id>;
            *ids_ = *other.ids_;
        }
        else
            ids_ = other.ids_;
    }

    id_array& operator=(id_array const& other)
    {
        if (owner_)
            delete ids_;
        owner_ = other.owner_;
        if (owner_)
        {
            ids_ = new std::vector<owned_id>;
            *ids_ = *other.ids_;
        }
        else
            ids_ = other.ids_;
        return *this;
    }

    id_array(std::vector<owned_id>* ids, bool owner)
      : ids_(ids), owner_(owner)
    {}

    ~id_array() { if (owner_) delete ids_; }

    id_interface* clone() const
    {
        id_array* copy = new id_array;
        this->deep_copy(copy);
        return copy;
    }

    bool equals(id_interface const& other) const
    {
        id_array const& other_id = static_cast<id_array const&>(other);
        size_t size = ids_->size(), other_size = other_id.ids_->size();
        if (size != other_size)
            return false;
        for (size_t i = 0; i != size; ++i)
        {
            if (!(*ids_)[i].get().equals((*other_id.ids_)[i].get()))
                return false;
        }
        return true;
    }

    bool less_than(id_interface const& other) const
    {
        id_array const& other_id = static_cast<id_array const&>(other);
        size_t size = ids_->size(), other_size = other_id.ids_->size();
        if (size != other_size)
            return size < other_size;
        for (size_t i = 0; i != size; ++i)
        {
            if ((*ids_)[i].get().less_than((*other_id.ids_)[i].get()))
                return true;
            if ((*other_id.ids_)[i].get().less_than((*ids_)[i].get()))
                return false;
        }
        return false;
    }

    void stream(std::ostream& o) const
    {
        o << "[";
        size_t size = ids_->size();
        for (size_t i = 0; i != size; ++i)
        {
            if (i != 0)
                o << ",";
            o << (*ids_)[i].get();
        }
        o << "]";
    }

    void deep_copy(id_interface* copy) const
    {
        id_array* typed_copy = static_cast<id_array*>(copy);
        typed_copy->owner_ = true;
        typed_copy->ids_ = new std::vector<owned_id>;
        *typed_copy->ids_ = *ids_;
    }

    size_t hash() const
    {
        size_t h = 0;
        size_t size = ids_->size();
        for (size_t i = 0; i != size; ++i)
            h ^= (*ids_)[i].get().hash();
        return h;
    }

 private:
    std::vector<owned_id>* ids_;
    // If this is true, this object provides ownership of the id vector.
    bool owner_;
};

// This is the GUI version of a functional map.
// It takes a vector of items and a function mapping a single item to another
// type. This function is a GUI function, so its first argument is a GUI
// context (and the second is the item to map). It can use the normal GUI
// caching and background calculation mechanisms.
// The result of this function is an accessor to a vector of mapped items.
// This will be gettable once all items have been successfully mapped (i.e.,
// once the mapping function is returning a gettable accessor for every item).
template<class MappedItem, class Fn, class Item>
indirect_accessor<std::vector<MappedItem> >
gui_map(gui_context& ctx, Fn const& fn,
    accessor<std::vector<Item> > const& items)
{
    accessor<std::vector<MappedItem> > const* result =
        erase_type(ctx, empty_accessor<std::vector<MappedItem> >());
    alia_if (is_gettable(items))
    {
        size_t n_items = get(items).size();
        gui_map_to_vector_data<MappedItem>* data;
        get_cached_data(ctx, &data);
        if (!data->input_id.matches(items.id()))
        {
            data->mapped_items.resize(n_items);
            data->mapped_ids.resize(n_items);
            data->n_valid_items = 0;
            data->input_id.store(items.id());
            inc_version(data->abbreviated_identity);
        }
        // Map the items.
        // Note that we continue doing this even after all the items are
        // valid. This is because the mapped item is allowed to change.
        alia_for (size_t i = 0; i != n_items; ++i)
        {
            auto mapped_item = fn(ctx, select_index(ref(&items), i));
            if (is_gettable(mapped_item))
            {
                if (!data->mapped_ids[i].matches(mapped_item.id()))
                {
                    data->mapped_items[i] = get(mapped_item);
                    data->mapped_ids[i].store(mapped_item.id());
                    inc_version(data->abbreviated_identity);
                }
                if (data->n_valid_items == i)
                    ++data->n_valid_items;
            }
        }
        alia_end
        // If all the mapped items are valid, return the full result.
        if (data->n_valid_items == n_items)
        {
            result = erase_type(ctx,
                make_custom_getter(&data->mapped_items,
                    optimize_id_equality(
                        id_array(&data->mapped_ids, false),
                        get_id(data->abbreviated_identity))));
        }
    }
    alia_end
    return ref(result);
}

template<class Item>
value_id<size_t>
get_item_id(size_t index, Item const& item)
{
    return make_id(index);
}

template<class Fn, class Item>
void
for_each(gui_context& ctx,
    Fn const& fn,
    accessor<std::vector<Item> > const& items)
{
    alia_if (is_gettable(items))
    {
        naming_context nc(ctx);
        size_t n_items = get(items).size();
        for (size_t i = 0; i != n_items; ++i)
        {
            named_block nb(nc, get_item_id(i, get(items)[i]));
            fn(ctx, i, select_index(ref(&items), i));
        }
    }
    alia_end
}

// select_map_index(map, index), where map is an accessor to a std::map and
// index is an accessor to an index into that map, returns an accessor to the
// value in that map associated with that index.
// Note that this assumes you want that element to always be in the map, so if
// it's not already there, it will supply a default_initialized element.
template<class MapAccessor, class IndexAccessor>
struct map_index_accessor
  : accessor<typename MapAccessor::value_type::mapped_type>
{
    typedef typename alia::accessor_value_type<MapAccessor>::type map_type;
    typedef typename map_type::mapped_type value_type;
    map_index_accessor() {}
    map_index_accessor(MapAccessor const& map, IndexAccessor const& index)
      : map_(map), index_(index)
    {}
    bool is_gettable() const
    {
        return map_.is_gettable() && index_.is_gettable();
    }
    value_type const& get() const
    {
        map_type const& m = map_.get();
        auto iterator = m.find(index_.get());
        static value_type missing_item = default_initialized<value_type>();
        if (iterator == m.end())
            return missing_item;
        else
            return iterator->second;
    }
    alia::id_interface const& id() const
    {
        id_ = make_index_id(ref(&map_.id()), ref(&index_.id()));
        return id_;
    }
    bool is_settable() const
    {
        return map_.is_settable();
    }
    void set(value_type const& x) const
    {
        map_type m = map_.is_gettable() ? map_.get() : map_type();
        m[index_.get()] = x;
        map_.set(m);
    }
 private:
    MapAccessor map_;
    IndexAccessor index_;
    mutable collection_index_id<alia::id_ref,alia::id_ref> id_;
};
template<class MapAccessor, class IndexAccessor>
map_index_accessor<
    typename copyable_accessor_helper<MapAccessor const&>::result_type,
    typename copyable_accessor_helper<IndexAccessor const&>::result_type>
select_map_index(MapAccessor const& map, IndexAccessor const& index)
{
    return
        map_index_accessor<
            typename copyable_accessor_helper<
                MapAccessor const&>::result_type,
            typename copyable_accessor_helper<
                IndexAccessor const&>::result_type>(
            make_accessor_copyable(map),
            make_accessor_copyable(index));
}

// is_inside_map(map, index), where map is an accessor to a std::map and
// index is an accessor to an index into that map, returns an accessor to a
// boolean indicating if that index is contained within that map.
template<class MapAccessor, class IndexAccessor>
struct is_member_of_map_accessor : accessor<bool>
{
    typedef typename alia::accessor_value_type<MapAccessor>::type map_type;
    is_member_of_map_accessor() {}
    is_member_of_map_accessor(
        MapAccessor const& map, IndexAccessor const& index)
      : map_(map), index_(index)
    {}
    bool is_gettable() const
    {
        return map_.is_gettable() && index_.is_gettable();
    }
    bool const& get() const
    {
        map_type const& m = map_.get();
        // Need to return a reference.
        static bool const true_ = true, false_ = false;
        if (m.find(index_.get()) != m.end())
            return true_;
        else
            return false_;
    }
    alia::id_interface const& id() const
    {
        if (this->is_gettable())
        {
            id_ = make_id(this->get());
            return id_;
        }
        else
            return no_id;
    }
    bool is_settable() const
    {
        return false;
    }
    void set(bool const& x) const
    {
    }
 private:
    MapAccessor map_;
    IndexAccessor index_;
    mutable value_id<bool> id_;
};
template<class MapAccessor, class IndexAccessor>
is_member_of_map_accessor<
    typename copyable_accessor_helper<MapAccessor const&>::result_type,
    typename copyable_accessor_helper<IndexAccessor const&>::result_type>
is_member_of_map(MapAccessor const& map, IndexAccessor const& index)
{
    return
        is_member_of_map_accessor<
            typename copyable_accessor_helper<
                MapAccessor const&>::result_type,
            typename copyable_accessor_helper<
                IndexAccessor const&>::result_type>(
            make_accessor_copyable(map),
            make_accessor_copyable(index));
}

// select_map_index_readonly(map, index), where map is an accessor to a
// std::map and index is an accessor to an index into that map, returns a
// read-only accessor to the value in that map associated with that index.
// The accessor is only gettable if the index actually has an associated value.
template<class MapAccessor, class IndexAccessor>
struct readonly_map_index_accessor
  : accessor<typename MapAccessor::value_type::mapped_type>
{
    typedef typename alia::accessor_value_type<MapAccessor>::type map_type;
    typedef typename map_type::mapped_type value_type;
    readonly_map_index_accessor() {}
    readonly_map_index_accessor(
        MapAccessor const& map, IndexAccessor const& index)
      : map_(map), index_(index)
    {}
    bool is_gettable() const
    {
        if (map_.is_gettable() && index_.is_gettable())
        {
            map_type const& m = map_.get();
            return m.find(index_.get()) != m.end();
        }
        else
            return false;
    }
    value_type const& get() const
    {
        map_type const& m = map_.get();
        return m.find(index_.get())->second;
    }
    alia::id_interface const& id() const
    {
        id_ = make_index_id(ref(&map_.id()), ref(&index_.id()));
        return id_;
    }
    bool is_settable() const
    {
        return false;
    }
    void set(value_type const& x) const
    {
    }
 private:
    MapAccessor map_;
    IndexAccessor index_;
    mutable collection_index_id<alia::id_ref,alia::id_ref> id_;
};
template<class MapAccessor, class IndexAccessor>
readonly_map_index_accessor<MapAccessor,IndexAccessor>
select_map_index_readonly(MapAccessor const& map, IndexAccessor const& index)
{
    return readonly_map_index_accessor<MapAccessor,IndexAccessor>(map, index);
}

// This is the GUI equivalent of a functional map for std::maps.
// It's analogous to the vector version above, but the mapping function takes
// three arguments instead of two: the GUI context, the item's key, and the
// item's value.
template<class MappedItem, class Fn, class Key, class Value>
indirect_accessor<std::vector<MappedItem> >
gui_map(
    gui_context& ctx, Fn const& fn,
    accessor<std::map<Key,Value> > const& items)
{
    accessor<std::vector<MappedItem> > const* result =
        erase_type(ctx, empty_accessor<std::vector<MappedItem> >());
    alia_if (is_gettable(items))
    {
        size_t n_items = get(items).size();
        gui_map_to_vector_data<MappedItem>* data;
        get_cached_data(ctx, &data);
        if (!data->input_id.matches(items.id()))
        {
            data->mapped_items.resize(n_items);
            data->mapped_ids.resize(n_items);
            data->n_valid_items = 0;
            data->input_id.store(items.id());
            inc_version(data->abbreviated_identity);
        }
        // Map the items.
        // Note that we continue doing this even after all the items are
        // valid. This is because the mapped item is allowed to change.
        {
            size_t index = 0;
            alia_for (auto const& item : get(items))
            {
                auto key = in_ptr(&item.first);
                auto value = select_map_index(ref(&items), key);
                auto mapped_item = fn(ctx, key, value);
                if (is_gettable(mapped_item))
                {
                    if (!data->mapped_ids[index].matches(mapped_item.id()))
                    {
                        data->mapped_items[index] = get(mapped_item);
                        data->mapped_ids[index].store(mapped_item.id());
                        inc_version(data->abbreviated_identity);
                    }
                    if (data->n_valid_items == index)
                        ++data->n_valid_items;
                }
                ++index;
            }
            alia_end
        }
        // If all the mapped items are valid, return the full result.
        if (data->n_valid_items == n_items)
        {
            result = erase_type(ctx,
                make_custom_getter(&data->mapped_items,
                    optimize_id_equality(
                        id_array(&data->mapped_ids, false),
                        get_id(data->abbreviated_identity))));
        }
    }
    alia_end
    return ref(result);
}

template<class Key, class MappedItem>
struct gui_map_to_map_data
{
    owned_id input_id;
    std::map<Key,MappedItem> mapped_items;
    std::vector<owned_id> mapped_ids;
    unsigned n_valid_items;
    local_identity abbreviated_identity;
};

// gui_map_to_map is like the above, but it produces a std::map rather than a
// std::vector. The keys in the result are the same as the input.
template<class MappedItem, class Fn, class Key, class Value>
indirect_accessor<std::map<Key,MappedItem> >
gui_map_to_map(
    gui_context& ctx, Fn const& fn,
    accessor<std::map<Key,Value> > const& items)
{
    accessor<std::map<Key,MappedItem> > const* result =
        erase_type(ctx, empty_accessor<std::map<Key,MappedItem> >());
    alia_if (is_gettable(items))
    {
        size_t n_items = get(items).size();
        gui_map_to_map_data<Key,MappedItem>* data;
        get_cached_data(ctx, &data);
        // If the ID of the items changes, invalidate everything.
        if (!data->input_id.matches(items.id()))
        {
            data->mapped_items.clear();
            data->mapped_ids.clear();
            data->mapped_ids.resize(n_items);
            data->n_valid_items = 0;
            inc_version(data->abbreviated_identity);
            data->input_id.store(items.id());
        }
        // Map the items.
        // Note that we continue doing this even after all the items are
        // valid. This is because the mapped item is allowed to change.
        {
            size_t index = 0;
            alia_for (auto const& item : get(items))
            {
                auto key = in_ptr(&item.first);
                auto value = select_map_index(ref(&items), key);
                auto mapped_item = fn(ctx, key, value);
                if (is_gettable(mapped_item))
                {
                    if (!data->mapped_ids[index].matches(mapped_item.id()))
                    {
                        data->mapped_items[item.first] = get(mapped_item);
                        data->mapped_ids[index].store(mapped_item.id());
                        inc_version(data->abbreviated_identity);
                    }
                    if (data->n_valid_items == index)
                        ++data->n_valid_items;
                }
                ++index;
            }
            alia_end
        }
        // If all the mapped items are valid, return the full result.
        if (data->n_valid_items == n_items)
        {
            result = erase_type(ctx,
                make_custom_getter(&data->mapped_items,
                    optimize_id_equality(
                        id_array(&data->mapped_ids, false),
                        get_id(data->abbreviated_identity))));
        }
    }
    alia_end
    return ref(result);
}

template<class Fn, class Key, class Value>
void
for_each(gui_context& ctx,
    Fn const& fn,
    accessor<std::map<Key,Value> > const& items)
{
    alia_if (is_gettable(items))
    {
        naming_context nc(ctx);
        for (auto const& item : get(items))
        {
            auto key = in(item.first); // TODO
            named_block nb(nc, key.id());
            auto value = select_map_index(ref(&items), key);
            fn(ctx, key, value);
        }
    }
    alia_end
}

// Add an item to the end of a container via an accessor.
template<class Container, class Value>
void push_back_to_accessor(
    accessor<Container> const& container,
    Value const& value)
{
    Container x = is_gettable(container) ? get(container) : Container();
    x.push_back(value);
    set(container, x);
}

// Add an item to a container via an accessor.
template<class Container, class Value>
void insert_to_accessor(
    accessor<Container> const& container,
    Value const& value,
    size_t index)
{
    Container x = is_gettable(container) ? get(container) : Container();
    x.insert(x.begin() + index, value);
    set(container, x);
}

// Add an item to a map via an accessor.
template<class Key, class Value>
void add_to_map_accessor(
    accessor<std::map<Key,Value> > const& container,
    Key const& key, Value const& value)
{
    std::map<Key,Value> x =
        is_gettable(container) ? get(container) : std::map<Key,Value>();
    x[key] = value;
    set(container, x);
}

// Remove an item from a random access container via an accessor.
template<class Value>
void remove_item_from_accessor(
    accessor<std::vector<Value> > const& container,
    size_t index)
{
    auto x = get(container);
    x.erase(x.begin() + index);
    set(container, x);
}

// Remove an item (identified by value) from a random access container via an
// accessor.
template<class Value>
void remove_value_from_accessor(
    accessor<std::vector<Value> > const& container,
    Value const& value)
{
    auto x = get(container);
    x.erase(std::remove(x.begin(), x.end(), value), x.end());
    set(container, x);
}

// Remove an item from a map via an accessor.
template<class Key, class Value>
void remove_item_from_map_accessor(
    accessor<std::map<Key,Value> > const& container,
    Key const& key)
{
    auto x = get(container);
    x.erase(key);
    set(container, x);
}

// get_collection_size(ctx, x), where x is a STL-compatible collection, yields
// a read-only accessor to the size of the collection.
template<class Collection>
size_t get_collection_size_helper(Collection const& x)
{ return x.size(); }
template<class Collection>
gui_apply_accessor<size_t>
get_collection_size(gui_context& ctx, accessor<Collection> const& x)
{
    return gui_apply(ctx, get_collection_size_helper<Collection>, x);
}

// is_empty(x), where x is an STL-compatible collection, yields a read-only
// accessor to a boolean flag indicating whether or not the collection is
// empty.
template<class Collection>
auto
is_empty(Collection const& x)
{
    return
        lazy_apply(
            [ ](auto const& collection)
            {
                return collection.empty();
            },
            x);
}

}

#endif
