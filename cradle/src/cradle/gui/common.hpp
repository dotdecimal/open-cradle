#ifndef CRADLE_GUI_COMMON_HPP
#define CRADLE_GUI_COMMON_HPP

#include <alia/ui/api.hpp>
#include <cradle/common.hpp>
#include <alia/ui/utilities/timing.hpp>
#include <cradle/gui/types.hpp>

namespace cradle {

using alia::accessor;

struct gui_system;
struct app_context;
struct framework_context;

struct gui_context : alia::ui_context
{
    cradle::gui_system* gui_system;
};

struct scoped_gui_context : noncopyable
{
    scoped_gui_context() {}
    scoped_gui_context(alia::ui_context& alia_ctx, gui_system& system)
    { begin(alia_ctx, system); }
    ~scoped_gui_context() { end(); }

    void begin(alia::ui_context& alia_ctx, gui_system& system);
    void end();

    gui_context& context() { return ctx_; }

 private:
    gui_context ctx_;
};

// add_fallback_value(primary, fallback) creates an accessor that will yield
// the primary value when/if it's ready and the fallback value otherwise.
// It can be used to substitute in quick approximations for results that take
// a long time to compute.
template<class Primary, class Fallback>
struct fallback_accessor
  : accessor<typename accessor_value_type<Primary>::type>
{
    fallback_accessor() {}
    fallback_accessor(Primary primary, Fallback fallback)
      : primary_(primary), fallback_(fallback)
    {}
    bool is_gettable() const
    { return primary_.is_gettable() || fallback_.is_gettable(); }
    typename accessor_value_type<Primary>::type const& get() const
    { return primary_.is_gettable() ? primary_.get() : fallback_.get(); }
    id_interface const& id() const
    { return primary_.is_gettable() ? primary_.id() : fallback_.id(); }
    bool is_settable() const
    { return primary_.is_settable(); }
    void set(typename accessor_value_type<Primary>::type const& value) const
    { primary_.set(value); }
 private:
    Primary primary_;
    Fallback fallback_;
};
template<class PrimaryAccessor, class FallbackAccessor>
fallback_accessor<
    typename copyable_accessor_helper<PrimaryAccessor const&>::result_type,
    typename copyable_accessor_helper<FallbackAccessor const&>::result_type>
add_fallback_value(
    PrimaryAccessor const& primary, FallbackAccessor const& fallback)
{
    return
        fallback_accessor<
            typename copyable_accessor_helper<
                PrimaryAccessor const&>::result_type,
            typename copyable_accessor_helper<
                FallbackAccessor const&>::result_type>(
            make_accessor_copyable(primary),
            make_accessor_copyable(fallback));
}

// collection_index_id is an ID representing the result of selecting a
// particular element from a collecton.
template<class Collection, class Index>
struct collection_index_id : id_interface
{
    collection_index_id() {}

    collection_index_id(Collection const& collection, Index const& index)
      : collection_(collection), index_(index) {}

    id_interface* clone() const
    {
        collection_index_id* copy = new collection_index_id;
        this->deep_copy(copy);
        return copy;
    }

    bool equals(id_interface const& other) const
    {
        collection_index_id const& other_id =
            static_cast<collection_index_id const&>(other);
        return collection_.equals(other_id.collection_) &&
            index_.equals(other_id.index_);
    }

    bool less_than(id_interface const& other) const
    {
        collection_index_id const& other_id =
            static_cast<collection_index_id const&>(other);
        return collection_.less_than(other_id.collection_) ||
            collection_.equals(other_id.collection_) &&
            index_.less_than(other_id.index_);
    }

    void stream(std::ostream& o) const
    { o << "index(" << collection_ << "," << index_ << ")"; }

    void deep_copy(id_interface* copy) const
    {
        collection_index_id* typed_copy =
            static_cast<collection_index_id*>(copy);
        collection_.deep_copy(&typed_copy->collection_);
        index_.deep_copy(&typed_copy->index_);
    }

    size_t hash() const
    {
        return collection_.hash() ^ index_.hash();
    }

 private:
    Collection collection_;
    Index index_;
};
// make_index_id(collection, index) combines the collection and index IDs into
// a single collection_index_id.
template<class Collection, class Index>
collection_index_id<Collection,Index>
make_index_id(Collection const& collection, Index const& index)
{ return collection_index_id<Collection,Index>(collection, index); }

// Given an accessor to an array, select_index(accessor, n) returns an accessor
// to the nth item within the array.
template<class ArrayAccessor, class IndexAccessor>
struct indexed_accessor
  : accessor<typename ArrayAccessor::value_type::value_type>
{
    typedef typename alia::accessor_value_type<ArrayAccessor>::type array_type;
    typedef typename ArrayAccessor::value_type::value_type item_type;
    indexed_accessor() {}
    indexed_accessor(ArrayAccessor array, IndexAccessor index)
      : array_(array), index_(index)
    {}
    bool is_gettable() const
    { return array_.is_gettable() && index_.is_gettable(); }
    item_type const& get() const
    {
        size_t index = index_.get();
        array_type const& array = array_.get();
        if (index >= array.size())
            throw index_out_of_bounds("indexed_accessor", index, array.size());
        return array[index];
    }
    alia::id_interface const& id() const
    {
        id_ = make_index_id(ref(&array_.id()), ref(&index_.id()));
        return id_;
    }
    bool is_settable() const
    {
        return array_.is_gettable() && index_.is_gettable() &&
            array_.is_settable();
    }
    void set(item_type const& x) const
    {
        size_t index = index_.get();
        array_type array = array_.get();
        if (index >= array.size())
            throw index_out_of_bounds("indexed_accessor", index, array.size());
        array[index] = x;
        array_.set(array);
    }
 private:
    ArrayAccessor array_;
    IndexAccessor index_;
    mutable collection_index_id<alia::id_ref,alia::id_ref> id_;
};
template<class ArrayAccessor, class Index>
indexed_accessor<
    typename copyable_accessor_helper<ArrayAccessor const&>::result_type,
    input_accessor<Index> >
select_index(ArrayAccessor const& array, Index index)
{
    return
        indexed_accessor<
            typename copyable_accessor_helper<
                ArrayAccessor const&>::result_type,
            input_accessor<Index> >(
            make_accessor_copyable(array), in(index));
}
template<class ArrayAccessor, class IndexAccessor>
indexed_accessor<
    typename copyable_accessor_helper<ArrayAccessor const&>::result_type,
    typename copyable_accessor_helper<IndexAccessor const&>::result_type>
select_index_via_accessor(ArrayAccessor const& array,
    IndexAccessor const& index)
{
    return
        indexed_accessor<
            typename copyable_accessor_helper<
                ArrayAccessor const&>::result_type,
            typename copyable_accessor_helper<
                IndexAccessor const&>::result_type>(
            make_accessor_copyable(array),
            make_accessor_copyable(index));
}

// select_index_by_value is identical to select_index, but it yields a copy of
// the nth item, and thus doesn't require the array to return a reference to
// its elements.
template<class ArrayAccessor, class Index>
struct index_by_value_accessor
  : accessor<typename ArrayAccessor::value_type::value_type>
{
    typedef typename alia::accessor_value_type<ArrayAccessor>::type array_type;
    typedef typename ArrayAccessor::value_type::value_type item_type;
    index_by_value_accessor() {}
    index_by_value_accessor(ArrayAccessor array, Index index)
      : array_(array), index_(index)
    {}
    bool is_gettable() const { return array_.is_gettable(); }
    item_type const& get() const { return lazy_getter_.get(*this); }
    alia::id_interface const& id() const
    {
        id_ = make_index_id(ref(&array_.id()), make_id(index_));
        return id_;
    }
    bool is_settable() const
    { return array_.is_gettable() && array_.is_settable(); }
    void set(item_type const& x) const
    {
        array_type a = array_.get();
        a[index_] = x;
        array_.set(a);
    }
 private:
    ArrayAccessor array_;
    Index index_;
    mutable collection_index_id<alia::id_ref,alia::value_id<Index> > id_;
    friend struct alia::lazy_getter<item_type>;
    item_type generate() const { return array_.get()[index_]; }
    alia::lazy_getter<item_type> lazy_getter_;
};
template<class ArrayAccessor, class Index>
index_by_value_accessor<
    typename copyable_accessor_helper<ArrayAccessor const&>::result_type,
    Index>
select_index_by_value(ArrayAccessor const& array, Index index)
{
    return
        index_by_value_accessor<
            typename copyable_accessor_helper<
                ArrayAccessor const&>::result_type,
            Index>(
            make_accessor_copyable(array), index);
}

// make_persistent_copy(ctx, x), where x is an accessor, makes a persistent
// copy of x's value as keyed_data and returns that keyed_data.
template<class Value>
alia::keyed_data<Value>*
make_persistent_copy(alia::ui_context& ctx, accessor<Value> const& x)
{
    alia::keyed_data<Value>* data;
    get_cached_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        refresh_keyed_data(*data, x.id());
        if (!is_valid(*data) && is_gettable(x))
            set(*data, get(x));
    }
    return data;
}

// Detect a transition into this part of the UI.
bool detect_transition_into_here(ui_context& ctx);

// _field(x, f), where x is an accessor to a structure, yields an accessor to
// the member f within x.
#define _field(x, f) \
    alia::select_field( \
        alia::make_accessor_copyable(x), \
        &remove_const_reference<decltype(get(x))>::type::f)

// Various utilities for implementing union member accessors.

template<class UnionAccessor, class Getter>
struct union_accessor_member_type
  : remove_const_reference<decltype(Getter()(UnionAccessor().get()))>
{};

template<class UnionAccessor, class Getter, class Setter>
struct union_member_accessor
  : accessor<typename union_accessor_member_type<UnionAccessor,Getter>::type>
{
    typedef typename accessor_value_type<UnionAccessor>::type union_type;
    typedef typename union_accessor_member_type<UnionAccessor,Getter>::type
        member_type;

    union_member_accessor() {}
    union_member_accessor(UnionAccessor union_accessor,
        Getter getter, Setter setter)
      : union_accessor_(union_accessor),
        getter_(getter), setter_(setter)
    {}
    bool is_gettable() const { return union_accessor_.is_gettable(); }
    member_type const& get() const
    {
        return getter_(union_accessor_.get());
    }
    id_interface const& id() const
    {
        id_ = make_index_id(ref(&union_accessor_.id()), make_id(getter_));
        return id_;
    }
    bool is_settable() const { return union_accessor_.is_settable(); }
    void set(member_type const& x) const
    {
        union_type u;
        setter_(u, x);
        union_accessor_.set(u);
    }
 private:
    UnionAccessor union_accessor_;
    Getter getter_;
    Setter setter_;
    mutable collection_index_id<id_ref,value_id<Getter> > id_;
};
template<class UnionAccessor, class Getter, class Setter>
union_member_accessor<
    typename copyable_accessor_helper<UnionAccessor const&>::result_type,
    Getter,Setter>
select_union_member(UnionAccessor const& union_accessor,
    Getter const& getter, Setter const& setter)
{
    return
        union_member_accessor<
            typename copyable_accessor_helper<UnionAccessor const&>::result_type,
            Getter,Setter>(
        union_accessor, getter, setter);
}

template<class T>
struct remove_const
{
};
template<class T>
struct remove_const<T const&>
{
    typedef T& type;
};

// _union_member(x, m), where x is an accessor to a union, yields an accessor
// to the member m of x.
// (You should be sure that x is actually an m before using it.)
#define _union_member(x, m) \
    select_union_member(make_accessor_copyable(x), \
        static_cast<decltype(as_##m(get(x)))(*)(decltype(get(x)))>(&as_##m), \
        static_cast<void (*)(remove_const<decltype(get(x))>::type, \
            decltype(as_##m(get(x))))>(&set_to_##m))

// Various utilities for implementing optional union member accessors.

template<class UnionAccessor, class Getter>
struct optional_union_accessor_member_type
  : remove_const_reference<decltype(Getter()(UnionAccessor().get().get()))>
{};

template<class UnionAccessor, class Matcher, class Getter, class Setter>
struct optional_union_member_accessor
  : accessor<optional<
        typename optional_union_accessor_member_type<UnionAccessor,Getter>::type> >
{
    typedef typename accessor_value_type<UnionAccessor>::type::value_type
        union_type;
    typedef typename optional_union_accessor_member_type<UnionAccessor,Getter>::type
        member_type;

    optional_union_member_accessor() {}
    optional_union_member_accessor(UnionAccessor union_accessor,
        Matcher matcher, Getter getter, Setter setter)
      : union_accessor_(union_accessor),
        matcher_(matcher), getter_(getter), setter_(setter)
    {}
    bool is_gettable() const { return union_accessor_.is_gettable(); }
    optional<member_type> const& get() const
    { return lazy_getter_.get(*this); }
    id_interface const& id() const
    {
        id_ = make_index_id(ref(&union_accessor_.id()), make_id(getter_));
        return id_;
    }
    bool is_settable() const { return union_accessor_.is_settable(); }
    void set(optional<member_type> const& x) const
    {
        if (x)
        {
            union_type u;
            setter_(u, x.get());
            union_accessor_.set(some(u));
        }
        else
            union_accessor_.set(none);
    }
 private:
    friend struct lazy_getter<optional<member_type> >;
    lazy_getter<optional<member_type> > lazy_getter_;
    optional<member_type> generate() const
    {
        optional<union_type> const& u = union_accessor_.get();
        if (u && matcher_(u.get()))
            return some(getter_(u.get()));
        else
            return none;
    }
    UnionAccessor union_accessor_;
    Matcher matcher_;
    Getter getter_;
    Setter setter_;
    mutable collection_index_id<id_ref,value_id<Getter> > id_;
};
template<class UnionAccessor, class Matcher, class Getter, class Setter>
optional_union_member_accessor<
    typename copyable_accessor_helper<UnionAccessor const&>::result_type,
    Matcher,Getter,Setter>
select_optional_union_member(UnionAccessor const& union_accessor,
    Matcher const& matcher, Getter const& getter, Setter const& setter)
{
    return
        optional_union_member_accessor<
            typename copyable_accessor_helper<UnionAccessor const&>::result_type,
            Matcher,Getter,Setter>(
        union_accessor, matcher, getter, setter);
}

// _optional_union_member(x, m), where x is an accessor to a optional union,
// yields an accessor to the member m of x. The value of the accessor is
// optional and only has a value if x is actually an m.
#define _optional_union_member(x, m) \
    select_optional_union_member(make_accessor_copyable(x), \
        static_cast<bool(*)(decltype(get(x.get())))>(&is_##m), \
        static_cast<decltype(as_##m(get(x.get())))(*)(decltype(get(x.get())))>(&as_##m), \
        static_cast<void (*)(remove_const<decltype(get(x.get()))>::type, \
            decltype(as_##m(get(x.get()))))>(&set_to_##m))

// _switch_accessor(x), where x is an accessor, acts like an alia_switch
// statement over the value of x. It takes care of checking first that x is
// accessible.

#define _switch_accessor(x) \
    alia_if (is_gettable(x)) \
    { \
        alia_switch (get(x))

#define _end_switch \
        alia_end \
    } \
    alia_end

// apply_value_type<T>(ctx, x), where x is an accessor to a cradle::value,
// yields an accessor to a value of type T which is a view of x's generic value
// using to_value and from_value.
template<class T>
struct value_accessor_type_applier_data
{
    keyed_data<T> result;
};
template<class T, class ValueAccessor>
struct value_accessor_type_applier : accessor<T>
{
    value_accessor_type_applier() {}
    value_accessor_type_applier(ValueAccessor const& wrapped,
        value_accessor_type_applier_data<T>* data)
      : wrapped_(wrapped)
      , data_(data)
    {}
    id_interface const& id() const { return data_->result.key.get(); }
    bool is_gettable() const { return is_valid(data_->result); }
    T const& get() const { return alia::get(data_->result); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(T const& x) const
    {
        wrapped_.set(to_value(x));
        // Also update the cached value.
        // This is necessary because we have cases where two different pieces
        // of code try to set accessors that ultimately refer back to the
        // same value_accessor_type_applier.
        data_->result.value = x;
    }
 private:
    ValueAccessor wrapped_;
    value_accessor_type_applier_data<T>* data_;
};
template<class T, class ValueAccessor>
value_accessor_type_applier<T,
    typename copyable_accessor_helper<ValueAccessor const&>::result_type>
apply_value_type(gui_context& ctx, value_accessor_type_applier_data<T>* data,
    ValueAccessor const& x)
{
    if (is_refresh_pass(ctx))
        refresh_keyed_data(data->result, x.id());
    if (!is_valid(data->result) && is_gettable(x))
    {
        try
        {
            set(data->result, from_value<T>(get(x)));
        }
        catch (...)
        {
            T default_constructed;
            ensure_default_initialization(default_constructed);
            set(data->result, default_constructed);
        }
    }
    return
        value_accessor_type_applier<T,
            typename copyable_accessor_helper<
                ValueAccessor const&>::result_type>(
            make_accessor_copyable(x), data);
}
template<class T, class ValueAccessor>
value_accessor_type_applier<T,
    typename copyable_accessor_helper<ValueAccessor const&>::result_type>
apply_value_type(gui_context& ctx, ValueAccessor const& x)
{
    value_accessor_type_applier_data<T>* data;
    get_data(ctx, &data);
    return apply_value_type(ctx, data, x);
};

// This form takes an additional argument that acts as a generate for the
// initial value.
template<class T, class ValueAccessor, class InitialValueGenerator>
value_accessor_type_applier<T,
    typename copyable_accessor_helper<ValueAccessor const&>::result_type>
apply_value_type(
    gui_context& ctx, value_accessor_type_applier_data<T>* data,
    ValueAccessor const& x,
    InitialValueGenerator const& initial_value_generator)
{
    if (is_refresh_pass(ctx))
        refresh_keyed_data(data->result, x.id());
    if (!is_valid(data->result) && is_gettable(x))
    {
        try
        {
            set(data->result, from_value<T>(get(x)));
        }
        catch (...)
        {
            set(data->result, initial_value_generator());
        }
    }
    return
        value_accessor_type_applier<T,
            typename copyable_accessor_helper<
                ValueAccessor const&>::result_type>(
            make_accessor_copyable(x), data);
}
template<class T, class ValueAccessor, class InitialValueGenerator>
value_accessor_type_applier<T,
    typename copyable_accessor_helper<ValueAccessor const&>::result_type>
apply_value_type(gui_context& ctx, ValueAccessor const& x,
    InitialValueGenerator const& initial_value_generator)
{
    value_accessor_type_applier_data<T>* data;
    get_data(ctx, &data);
    return apply_value_type(ctx, data, x, initial_value_generator);
};

// accessor_base_cast<T>(a), where T is a base class of a's value type,
// yields an accessor to just the portion of a's value that is type T.
template<class Wrapped, class To>
struct accessor_base_caster : accessor<To>
{
    accessor_base_caster() {}
    accessor_base_caster(Wrapped wrapped) : wrapped_(wrapped) {}
    id_interface const& id() const { return wrapped_.id(); }
    bool is_gettable() const { return wrapped_.is_gettable(); }
    To const& get() const { return static_cast<To const&>(wrapped_.get()); }
    bool is_settable() const
    { return wrapped_.is_gettable() && wrapped_.is_settable(); }
    void set(To const& value) const
    {
        auto wrapped_value = wrapped_.get();
        static_cast<To&>(wrapped_value) = value;
        wrapped_.set(wrapped_value);
    }
 private:
    Wrapped wrapped_;
};
template<class To, class Wrapped>
accessor_base_caster<
    typename copyable_accessor_helper<Wrapped const&>::result_type,To>
accessor_base_cast(Wrapped const& accessor)
{
    return
        accessor_base_caster<
            typename copyable_accessor_helper<Wrapped const&>::result_type,
            To>(make_accessor_copyable(accessor));
}

// optimize_id_equality(full_id, quick_id) will merge the two IDs so that
// quick_id will be used to make testing equality faster and full_id will be
// used for other purposes (like persistent recording).
template<class FullId, class QuickId>
struct equality_optimized_id : id_interface
{
    equality_optimized_id() {}

    equality_optimized_id(FullId const& full_id, QuickId const& quick_id)
      : full_id_(full_id), quick_id_(quick_id) {}

    id_interface* clone() const
    {
        equality_optimized_id* copy = new equality_optimized_id;
        this->deep_copy(copy);
        return copy;
    }

    bool equals(id_interface const& other) const
    {
        equality_optimized_id const& other_id =
            static_cast<equality_optimized_id const&>(other);
        if (quick_id_.equals(other_id.quick_id_))
        {
            assert(full_id_.equals(other_id.full_id_));
            return true;
        }
        return full_id_.equals(other_id.full_id_);
    }

    bool less_than(id_interface const& other) const
    {
        equality_optimized_id const& other_id =
            static_cast<equality_optimized_id const&>(other);
        return full_id_.less_than(other_id.full_id_);
    }

    void stream(std::ostream& o) const
    { full_id_.stream(o); }

    void deep_copy(id_interface* copy) const
    {
        equality_optimized_id* typed_copy =
            static_cast<equality_optimized_id*>(copy);
        full_id_.deep_copy(&typed_copy->full_id_);
        quick_id_.deep_copy(&typed_copy->quick_id_);
    }

    size_t hash() const
    {
        return quick_id_.hash();
    }

 private:
    FullId full_id_;
    QuickId quick_id_;
};
template<class FullId, class QuickId>
equality_optimized_id<FullId,QuickId>
optimize_id_equality(FullId const& full_id, QuickId const& quick_id)
{
    return equality_optimized_id<FullId,QuickId>(full_id, quick_id);
}

// minimize_id_changes(ctx, x) yields a new accessor to x's value with a local
// ID that only changes when x's value actually changes.
template<class Value>
struct id_change_minimization_data
{
    owned_id input_id;
    local_identity output_id;
    Value value;
    bool is_valid;
    id_change_minimization_data() : is_valid(false) {}
};
template<class WrappedAccessor>
struct id_change_minimization_accessor
  : accessor<typename WrappedAccessor::value_type>
{
    typedef typename WrappedAccessor::value_type wrapped_value_type;
    id_change_minimization_accessor() {}
    id_change_minimization_accessor(
        WrappedAccessor const& wrapped,
        id_change_minimization_data<wrapped_value_type>* data)
      : wrapped_(wrapped), data_(data)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    wrapped_value_type const& get() const { return wrapped_.get(); }
    id_interface const& id() const
    {
        id_ = get_id(data_->output_id);
        return id_;
    }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(wrapped_value_type const& value) const
    {
        wrapped_.set(value);
    }
 private:
    WrappedAccessor wrapped_;
    id_change_minimization_data<wrapped_value_type>* data_;
    value_id_by_reference<local_id> mutable id_;
};
template<class Value>
void
update_id_change_minimization_data(
    id_change_minimization_data<Value>* data,
    accessor<Value> const& x)
{
    if (!data->input_id.matches(x.id()))
    {
        // Only change the output ID if the value has actually changed.
        if (!(data->is_valid && is_gettable(x) && data->value == get(x)))
        {
            inc_version(data->output_id);
            data->is_valid = false;
        }
        data->input_id.store(x.id());
    }
    if (!data->is_valid && is_gettable(x))
    {
        data->value = get(x);
        data->is_valid = true;
    }
}
template<class Accessor, class Value>
id_change_minimization_accessor<
    typename copyable_accessor_helper<Accessor const&>::result_type>
minimize_id_changes(dataless_ui_context& ctx,
    id_change_minimization_data<Value>* data,
    Accessor const& x)
{
    if (is_refresh_pass(ctx))
        update_id_change_minimization_data(data, x);
    return
        id_change_minimization_accessor<
            typename copyable_accessor_helper<Accessor const&>::result_type>(
            make_accessor_copyable(x), data);
}
template<class Accessor>
id_change_minimization_accessor<
    typename copyable_accessor_helper<Accessor const&>::result_type>
minimize_id_changes(gui_context& ctx, Accessor const& x)
{
    id_change_minimization_data<typename Accessor::value_type>* data;
    get_data(ctx, &data);
    return minimize_id_changes(ctx, data, x);
}

// rq_in(x) creates a read-only accessor for a request with the value of x.
template<class T>
struct input_request_accessor : accessor<request<T> >
{
    input_request_accessor() {}
    input_request_accessor(T const& v)
      : value_(v)
    {}
    bool is_gettable() const { return true; }
    request<T> const& get() const
    {
        request_ = rq_value(value_);
        return request_;
    }
    bool is_settable() const { return false; }
    void set(request<T> const& value) const {}
    id_interface const& id() const
    {
        id_ = make_id_by_reference(value_);
        return id_;
    }
 private:
    T value_;
    mutable request<T> request_;
    mutable value_id_by_reference<T> id_;
};
template<class T>
input_request_accessor<T>
rq_in(T const& value)
{
    return input_request_accessor<T>(value);
}

// as_value_request(x) wraps an accessor to a value as an accessor to a value
// request. (It also makes it read-only since setting doesn't make much sense.)
template<class Wrapped>
struct request_accessor_wrapper :
    accessor<request<typename Wrapped::value_type> >
{
    request_accessor_wrapper() {}
    request_accessor_wrapper(Wrapped const& wrapped)
      : wrapped_(wrapped)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    request<typename Wrapped::value_type> const& get() const
    {
        request_ = rq_value(wrapped_.get());
        return request_;
    }
    bool is_settable() const { return false; }
    void set(request<typename Wrapped::value_type> const& value) const {}
    // I think this is reasonable (and safe since they couldn't actually be
    // used in the same place).
    id_interface const& id() const { return wrapped_.id(); }
 private:
    Wrapped wrapped_;
    mutable request<typename Wrapped::value_type> request_;
};
template<class Accessor>
request_accessor_wrapper<Accessor>
as_value_request(Accessor const& accessor)
{
    return request_accessor_wrapper<Accessor>(accessor);
}

// Use this to fill empty display space.
void do_empty_display_panel(gui_context& ctx,
    layout const& layout_spec = GROW);

// Do styled text, without a flow_layout.
// (This is the styled_text analogue to do_text for strings.)
void do_text(
    gui_context& ctx, accessor<styled_text> const& paragraph);

// Do a styled text fragment.
void do_text(
    gui_context& ctx, accessor<styled_text_fragment> const& text,
    layout const& layout_spec = default_layout);

// Do styled text, within a flow_layout.
// (This is the styled_text analogue to do_flow_text for strings.)
void do_flow_text(
    gui_context& ctx, accessor<styled_text> const& paragraph,
    layout const& layout_spec = default_layout);

// Do a markdown document.
void do_markup_document(
    gui_context& ctx, accessor<markup_document> const& markup,
    layout const& layout_spec = default_layout);

// gui_apply(ctx, fn, arg1, ...) applies the function fn to the given
// arguments passed in accessor form. It ensures that the function is only
// applied when the arguments are all gettable and is only reapplied when one
// or more arguments change. The result is also in the form of an accessor.
// The function is called in the foreground (UI) thread.

enum class gui_apply_status
{
    UNCOMPUTED,
    READY,
    FAILED
};

template<class Value>
struct gui_apply_result_data
{
    local_identity output_id;
    Value result;
    gui_apply_status status = gui_apply_status::UNCOMPUTED;
};

template<class Value>
void reset(gui_apply_result_data<Value>& data)
{
    if (data.status != gui_apply_status::UNCOMPUTED)
    {
        inc_version(data.output_id);
        data.status = gui_apply_status::UNCOMPUTED;
    }
}

template<class Value>
struct gui_apply_accessor : accessor<Value>
{
    gui_apply_accessor() {}
    gui_apply_accessor(gui_apply_result_data<Value>& data) : data_(&data) {}
    id_interface const& id() const
    {
        id_ = get_id(data_->output_id);
        return id_;
    }
    Value const& get() const { return data_->result; }
    bool is_gettable() const
    { return data_->status == gui_apply_status::READY; }
    bool is_settable() const { return false; }
    void set(Value const& value) const {}
 private:
    gui_apply_result_data<Value>* data_;
    mutable value_id_by_reference<local_id> id_;
};

template<class Value>
gui_apply_accessor<Value>
make_accessor(gui_apply_result_data<Value>& data)
{ return gui_apply_accessor<Value>(data); }

void record_failure(gui_context& ctx, string const& message);

template<class Result>
void
process_gui_apply_args(
    gui_context& ctx,
    gui_apply_result_data<Result>& data,
    bool& args_ready)
{
}
template<class Result, class Arg, class ...Rest>
void
process_gui_apply_args(
    gui_context& ctx,
    gui_apply_result_data<Result>& data,
    bool& args_ready,
    Arg const& arg,
    Rest const& ...rest)
{
    owned_id* cached_id;
    get_cached_data(ctx, &cached_id);
    if (!is_gettable(arg))
    {
        reset(data);
        args_ready = false;
    }
    else if (!cached_id->matches(arg.id()))
    {
        reset(data);
        cached_id->store(arg.id());
    }
    process_gui_apply_args(ctx, data, args_ready, rest...);
}

template<class Fn, class ...Args>
auto
gui_apply(gui_context& ctx, Fn const& fn, Args const& ...args)
{
    gui_apply_result_data<decltype(fn(get(args)...))>* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;
    if (!is_refresh_pass(ctx))
        make_accessor(data);
    bool args_ready = true;
    process_gui_apply_args(ctx, data, args_ready, args...);
    if (data.status == gui_apply_status::UNCOMPUTED && args_ready)
    {
        try
        {
            data.result = fn(get(args)...);
            data.status = gui_apply_status::READY;
        }
        catch (std::bad_alloc&)
        {
            data.status = gui_apply_status::FAILED;
            record_failure(ctx, "debug details:\n(ga) insufficient memory ");
        }
        catch (std::exception& e)
        {
            data.status = gui_apply_status::FAILED;
            record_failure(ctx, string(e.what() + string("\n\ndebug details:\n(ga) std::exception")));
        }
        catch (...)
        {
            data.status = gui_apply_status::FAILED;
            record_failure(ctx, "debug details:\n(ga) unknown error ");
        }
    }
    return make_accessor(data);
}

// is_equal(x, v) returns true iff x is gettable and its value equals v
template<class Value>
[[deprecated("Use the == operator.")]]
bool is_equal(accessor<Value> const& x, Value const& v)
{ return is_gettable(x) && get(x) == v; }

// has_value(x) returns true iff x is gettable and it has a value
template<class Value>
bool has_value(accessor<optional<Value> > const& x)
{ return is_gettable(x) && get(x); }

// Do a collapsible UI block.
// This doesn't provide any UI controls for collapsing and expanding the UI,
// but it takes care of doing transitioning effects between the collapsed and
// expanded states.
// :is_expanded is a boolean flag indicating if block is expanded.
// :do_ui is a callback to do the actual UI.
// Note that it's possible for :do_ui to be invoked when is_expanded is false
// due to the transitioning effect.
template<class UiCallback>
void
do_collapsible_ui(
    gui_context& ctx,
    accessor<bool> const& is_expanded,
    UiCallback const& do_ui,
    layout const& layout_spec = default_layout)
{
    collapsible_content cc(ctx, is_true(is_expanded));
    alia_if (cc.do_content())
    {
        do_ui();
    }
    alia_end
}

// Get information about the Thinknode context that we're using.
// This was moved into the GUI context because the GUI system was already tied
// to a particular Thinknode context and gui_request needed to be able to get
// the context without an app_context. This should probably be revisited at
// some point, so I left an overload that takes an app_context as well.
indirect_accessor<framework_context>
get_framework_context(gui_context& ctx);
indirect_accessor<framework_context> static inline
get_framework_context(gui_context& ctx, app_context& app_ctx)
{
    return get_framework_context(ctx);
}

// Limits the frequency of updates to an accessor based on a given delay
template<class Accessor>
indirect_accessor<typename Accessor::value_type>
limit_calcs(gui_context& ctx, Accessor const& acssr, int const& delay = 500)
{
    indirect_accessor<typename Accessor::value_type> result;
    alia_if(is_gettable(acssr))
    {
        auto calc_state = get_state(ctx, get(acssr));

        alia_if(!is_equal(calc_state, get(acssr)))
        {
            //Temporary value to compare to copy
            auto tmp = get_state(ctx, get(acssr));
            timer t(ctx);

            /*
            If timer is triggered, compare the values in copy and tmp,
            if they are equal, allow the value for the calculation to update.
            This is triggered every 1 seconds after the initial change, if copy
            and tmp are equal when it triggers, the value has not changed and
            we should update the value for the calculation.
            */
            alia_if(t.triggered() && is_equal(tmp, get(acssr)))
            {
                set(calc_state, get(acssr));
            }
            alia_end

                /*
                If the timer is not already ticking and the value has changed, capture
                the value with tmp and start the timer.
                This will be repeated as many times as necessary in the event that the
                value continues changing longer than the delay.
                */
                alia_if(!t.is_active())
            {
                set(tmp, get(acssr));
                t.start(delay);
            }
            alia_end
        }
        alia_end

            result = make_indirect(ctx, calc_state);
    }
    alia_else
    {
        result =
        make_indirect(ctx,
            empty_accessor<typename Accessor::value_type>());
    }
        alia_end
        return result;
}

// Periodically refreshes patient data on a timer. Will also refresh data on a
// transition into this UI.
void
periodically_refresh_data(gui_context& ctx, int milliseconds_timer = 30000);

}

#endif
