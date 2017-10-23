#ifndef CRADLE_COMMON_HPP
#define CRADLE_COMMON_HPP

#include <alia/common.hpp>
#include <alia/id.hpp>
#include <vector>
#include <map>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

#include <boost/date_time/posix_time/posix_time_types.hpp>

#ifndef _WIN32
    // ignore warnings for GCC
    #pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

// General-purpose utility functions and types...

namespace cradle {

using namespace alia;

using std::uint8_t;
using std::int8_t;
using std::uint16_t;
using std::int16_t;
using std::uint32_t;
using std::int32_t;
using std::uint64_t;
using std::int64_t;

// Although the api() statements are removed by the preprocessor, IDE code
// analysis on the original headers doesn't like them. This macro causes the
// IDE to ignore them.
#define api(x)

struct exception : alia::exception
{
    exception(string const& msg)
      : alia::exception(msg)
    {}
    ~exception() throw() {}

    // If this returns true, it indicates that the condition that caused the
    // exception is transient, and thus retrying the same operation at a later
    // time may solve the problem.
    virtual bool is_transient() const { return false; }
};

// A lot of CRADLE algorithms use callbacks to report progress or check in with
// their callers (which can, for example, pause or terminate the algorithm).
// The following are the interface definitions for these callbacks and some
// utilities for constructing them.

// A progress reporter object gets called periodically with the progress of the
// algorithm (0 is just started, 1 is done).
struct progress_reporter_interface
{
    virtual void operator()(float) = 0;
};
// If you don't want to know about the progress, pass one of these.
struct null_progress_reporter : progress_reporter_interface
{
    void operator()(float) {}
};

// When an algorithm is divided into subtasks, this can be used to translate
// the progress of the subtasks into the overall progress of the main task.
// When creating the progress reporter for each subtask, you specify the
// portion of the overall job that the subtask represents. The state variable
// must be shared across all subtasks and tracks the overall progress of the
// main task. It's up to the caller to ensure that the sum of the portions of
// all subtasks is 1.
struct task_subdivider_state
{
    float offset;
    task_subdivider_state() : offset(0) {}
};
struct subtask_progress_reporter : progress_reporter_interface
{
    subtask_progress_reporter(
        progress_reporter_interface& parent_reporter,
        task_subdivider_state& state,
        float portion)
      : parent_reporter_(&parent_reporter)
      , state_(state)
      , portion_(portion)
    {
        offset_ = state_.offset;
        state_.offset += portion;
    }

    void operator()(float progress)
    {
        (*parent_reporter_)(offset_ + progress * portion_);
    }

 private:
    progress_reporter_interface* parent_reporter_;
    task_subdivider_state& state_;
    float offset_;
    float portion_;
};

// When an algorithm is divided into subtasks, and called from within a loop
// this sub_progress_reporter can be used.
// Takes in the parent reporter which it will call to report progress
// Takes in an offset which is the current progress when then reporter is made
// Takes in a scale which is used when this reporter is created in a loop
struct sub_progress_reporter : progress_reporter_interface
{
    sub_progress_reporter(
        progress_reporter_interface& parent_reporter,
        float offset,
        float scale)
      : parent_reporter_(&parent_reporter)
      , offset_(offset)
      , scale_ (scale)
    {
    }

    void operator()(float progress)
    {
        (*parent_reporter_)(offset_ + progress * scale_);
    }

 private:
    progress_reporter_interface* parent_reporter_;
    float offset_;
    float scale_;
};

// Algorithms call this to check in with the caller every few milliseconds.
// This can be used to abort the algorithm by throwing an exception.
struct check_in_interface
{
    virtual void operator()() = 0;
};
// If you don't need the algorithm to check in, pass one of these.
struct null_check_in : check_in_interface
{
    void operator()() {}
};

// If you need an algorithm to check in with two different controllers, you
// can use this to merge the check_in objects supplied by the two controllers.
struct merged_check_in : check_in_interface
{
    merged_check_in(check_in_interface* a, check_in_interface* b)
      : a(a), b(b)
    {}

    void operator()() { (*a)(); (*b)(); }

 private:
    check_in_interface* a;
    check_in_interface* b;
};

using std::swap;

template<class T>
struct remove_const_reference
{
    typedef T type;
};
template<class T>
struct remove_const_reference<T const>
{
    typedef T type;
};
template<class T>
struct remove_const_reference<T const&>
{
    typedef T type;
};

// Given a vector of optional values, return a vector containing all values
// inside those optionals.
template<class Value>
std::vector<Value>
filter_optionals(std::vector<optional<Value> > const& values)
{
    std::vector<Value> filtered;
    for (auto const& v : values)
    {
        if (v)
            filtered.push_back(get(v));
    }
    return filtered;
}

// functional map over a vector
template<class Item, class Fn>
auto map(Fn const& fn, std::vector<Item> const& items) ->
    std::vector<typename remove_const_reference<decltype(fn(Item()))>::type>
{
    typedef typename remove_const_reference<decltype(fn(Item()))>::type
        mapped_item_type;
    size_t n_items = items.size();
    std::vector<mapped_item_type> result(n_items);
    for (size_t i = 0; i != n_items; ++i)
        result[i] = fn(items[i]);
    return result;
}

// functional map over a map
template<class Key, class Value, class Fn>
auto map(Fn const& fn, std::map<Key,Value> const& items) ->
    std::map<Key,decltype(fn(Value()))>
{
    typedef decltype(fn(Value())) mapped_item_type;
    std::map<Key,mapped_item_type> result;
    for (auto const& item : items)
        result[item.first] = fn(item.second);
    return result;
}

// functional map from a std::map to a vector
template<class Key, class Value, class Fn>
auto map_to_vector(Fn const& fn, std::map<Key,Value> const& items) ->
    std::vector<decltype(fn(Key(),Value()))>
{
    typedef decltype(fn(Key(),Value())) mapped_item_type;
    std::vector<mapped_item_type> result;
    result.reserve(items.size());
    for (auto const& item : items)
        result.push_back(fn(item.first, item.second));
    return result;
}

// Get a pointer to the elements of a vector.
template<class T>
T const* get_elements_pointer(std::vector<T> const& vector)
{ return vector.empty() ? 0 : &vector[0]; }

// These seem to be missing from Visual C++ 11...

template <bool B, class T = void>
struct enable_if_c {
    typedef T type;
};
template <class T>
struct enable_if_c<false, T> {};

template <class Cond, class T = void>
struct enable_if : public enable_if_c<Cond::value, T> {};

template <bool B, class T = void>
struct disable_if_c {
    typedef T type;
};
template <class T>
struct disable_if_c<true, T> {};

template <class Cond, class T = void>
struct disable_if : public disable_if_c<Cond::value, T> {};

// any is capable of storing any value.
// The value can then be accessed via a cast.
struct untyped_cloneable_value_holder
{
    virtual ~untyped_cloneable_value_holder() {}
    virtual untyped_cloneable_value_holder* clone() const = 0;
};
template<class T>
struct typed_cloneable_value_holder : untyped_cloneable_value_holder
{
    explicit typed_cloneable_value_holder(T const& value) : value(value) {}
    explicit typed_cloneable_value_holder(T&& value)
      : value(static_cast<T&&>(value))
    {}
    untyped_cloneable_value_holder* clone() const
    { return new typed_cloneable_value_holder(value); }
    T value;
};
struct any
{
    // default constructor
    any() : holder_(0) {}
    // destructor
    ~any() { delete holder_; }
    // copy constructor
    any(any const& other)
      : holder_(other.holder_ ? other.holder_->clone() : 0)
    {}
    // move constructor
    any(any&& other)
      : holder_(other.holder_)
    {
        other.holder_ = 0;
    }
    // constructor for concrete values
    template<class T>
    explicit any(T const& value)
      : holder_(new typed_cloneable_value_holder<T>(value))
    {}
    // constructor for concrete values (by rvalue)
    template<typename T>
    any(T&& value
      , typename disable_if<std::is_same<any&,T> >::type* = 0
      , typename disable_if<std::is_const<T> >::type* = 0)
      : holder_(
            new typed_cloneable_value_holder<typename std::decay<T>::type >(
                static_cast<T&&>(value)))
    {
    }
    // copy assignment operator
    any& operator=(any const& other)
    {
        delete holder_;
        holder_ = other.holder_ ? other.holder_->clone() : 0;
        return *this;
    }
    // move assignment operator
    any& operator=(any&& other)
    {
        delete holder_;
        holder_ = other.holder_;
        other.holder_ = 0;
        return *this;
    }
    // swap
    void swap(any& other)
    { std::swap(holder_, other.holder_); }
    // assignment operator for concrete values
    template<class T>
    any& operator=(T const& value)
    {
        delete holder_;
        holder_ = new typed_cloneable_value_holder<T>(value);
        return *this;
    }
    // assignment operator for concrete values (by rvalue)
    template <class T>
    any& operator=(T&& value)
    {
        any(static_cast<T&&>(value)).swap(*this);
        return *this;
    }
    // value holder
    untyped_cloneable_value_holder* holder_;
};
static inline void swap(any& a, any& b)
{ a.swap(b); }
static inline untyped_cloneable_value_holder const*
get_value_pointer(any const& a)
{ return a.holder_; }
static inline untyped_cloneable_value_holder*
get_value_pointer(any& a)
{ return a.holder_; }
template<class T>
T const* any_cast(any const& a)
{
    typed_cloneable_value_holder<T> const* ptr =
        dynamic_cast<typed_cloneable_value_holder<T> const*>(get_value_pointer(a));
    return ptr ? &ptr->value : 0;
}
template<class T>
T* any_cast(any& a)
{
    typed_cloneable_value_holder<T>* ptr =
        dynamic_cast<typed_cloneable_value_holder<T>*>(get_value_pointer(a));
    return ptr ? &ptr->value : 0;
}
template<class T>
T const& unsafe_any_cast(any const& a)
{
    assert(dynamic_cast<typed_cloneable_value_holder<T> const*>(get_value_pointer(a)));
    return static_cast<typed_cloneable_value_holder<T> const*>(get_value_pointer(a))->value;
}
template<class T>
T& unsafe_any_cast(any& a)
{
    assert(dynamic_cast<typed_cloneable_value_holder<T>*>(get_value_pointer(a)));
    return static_cast<typed_cloneable_value_holder<T>*>(get_value_pointer(a))->value;
}

// any_by_ref is just like any, but it stores its value by reference (using a
// shared_ptr). It is thus must cheaper to copy for large values.
struct untyped_value_holder
{
    virtual ~untyped_value_holder() {}
};
template<class T>
struct typed_value_holder : untyped_value_holder
{
    typed_value_holder(T const& value) : value(value) {}
    T value;
};
struct any_by_ref
{
    any_by_ref() {}
    template<class T>
    explicit any_by_ref(T const& value)
      : holder_(new typed_value_holder<T>(value))
    {}
    any_by_ref& operator=(any_by_ref const& other)
    {
        holder_ = other.holder_;
        return *this;
    }
    template<class T>
    any_by_ref& operator=(T const& value)
    {
        holder_.reset(new typed_value_holder<T>(value));
        return *this;
    }
    alia__shared_ptr<untyped_value_holder> holder_;
};
static inline void swap(any_by_ref& a, any_by_ref& b)
{ swap(a.holder_, b.holder_); }
static inline untyped_value_holder const*
get_value_pointer(any_by_ref const& a)
{ return a.holder_.get(); }
template<class T>
T const* any_cast(any_by_ref const& a)
{
    typed_value_holder<T>* ptr =
        dynamic_cast<typed_value_holder<T> const*>(get_value_pointer(a));
    return ptr ? &ptr->value : 0;
}
template<class T>
T const& unsafe_any_cast(any_by_ref const& a)
{
    assert(dynamic_cast<typed_value_holder<T> const*>(get_value_pointer(a)));
    return static_cast<typed_value_holder<T> const*>(get_value_pointer(a))->value;
}

// Sometimes we want to express polymorphic ownership of a resource.
// The idea is that the resource may be owned in many different ways, and we
// don't care what way. We only want an object that will provide ownership of
// the resource until it's destructed.
// We can achieve this by using an any object to hold the ownership object.
typedef any ownership_holder;

// TYPE INFO

enum class raw_simple_type
{
    NIL,
    BOOLEAN,
    INTEGER,
    FLOAT,
    STRING,
    DATETIME,
    BLOB,
    DYNAMIC
};

#ifdef OPTIONAL
#undef OPTIONAL
#endif

enum class raw_kind
{
    STRUCTURE,
    UNION,
    ENUM,
    MAP,
    ARRAY,
    OPTIONAL,
    SIMPLE,
    // The type describes an actual reference to other data.
    DATA_REFERENCE,
    // Type info must be able to include references to other type info or else
    // it would be impossible to describe recursive types.
    NAMED_TYPE_REFERENCE,
    // This should only be used in structure fields.
    // It results in the thinknode 'omissible' flag being set.
    OMISSIBLE
};

struct raw_type_info
{
    raw_kind kind;
    any info;

    raw_type_info() {}
    raw_type_info(raw_kind kind, any const& info)
      : kind(kind), info(info)
    {}
};

struct raw_structure_field_info
{
    string name;
    string description;
    raw_type_info type;

    raw_structure_field_info() {}
    raw_structure_field_info(
        string const& name,
        string const& description,
        raw_type_info type)
      : name(name), description(description), type(type)
    {}
};

struct raw_structure_info
{
    string name;
    string description;
    std::vector<raw_structure_field_info> fields;

    raw_structure_info() {}
    raw_structure_info(
        string const& name,
        string const& description,
        std::vector<raw_structure_field_info> const& fields)
      : name(name), description(description), fields(fields)
    {}
};

struct raw_union_member_info
{
    string name;
    string description;
    raw_type_info type;

    raw_union_member_info() {}
    raw_union_member_info(
        string const& name,
        string const& description,
        raw_type_info type)
      : name(name), description(description), type(type)
    {}
};

struct raw_union_info
{
    string name;
    string description;
    std::vector<raw_union_member_info> members;

    raw_union_info() {}
    raw_union_info(
        string const& name,
        string const& description,
        std::vector<raw_union_member_info> const& members)
      : name(name), description(description), members(members)
    {}
};

struct raw_enum_value_info
{
    string name;
    string description;

    raw_enum_value_info() {}
    raw_enum_value_info(
        string const& name,
        string const& description)
      : name(name), description(description)
    {}
};

struct raw_enum_info
{
    string name;
    string description;
    std::vector<raw_enum_value_info> values;

    raw_enum_info() {}
    raw_enum_info(
        string const& name,
        string const& description,
        std::vector<raw_enum_value_info> const& values)
      : name(name), description(description), values(values)
    {}
};

struct raw_array_info
{
    optional<unsigned> size;
    raw_type_info element_type;

    raw_array_info() {}
    raw_array_info(
        optional<unsigned> const& size,
        raw_type_info const& element_type)
      : size(size), element_type(element_type)
    {}
};

struct raw_map_info
{
    raw_type_info key, value;

    raw_map_info() {}
    raw_map_info(
        raw_type_info const& key,
        raw_type_info const& value)
      : key(key), value(value)
    {}
};

struct raw_named_type_reference
{
    string app;
    string type;

    raw_named_type_reference() {}
    raw_named_type_reference(
        string const& app,
        string const& type)
      : app(app), type(type)
    {}
};

// DYNAMIC VALUES - Dynamic values are values whose structure is determined at
// run-time rather than compile time.

// All dynamic values have corresponding concrete C++ types.
// The following is a list of value types along with their significance and the
// corresponding C++ type.

enum class value_type
{
    NIL,            // no value, cradle::nil_type
    BOOLEAN,        // bool
    INTEGER,        // cradle::integer
    FLOAT,          // double
    STRING,         // cradle::string
    BLOB,           // binary blob, cradle::blob
    DATETIME,       // boost::posix_time::ptime
    LIST,           // ordered list of values, cradle::value_list
    MAP,            // collection of named values, cradle::value_map
};

std::ostream& operator<<(std::ostream& s, value_type t);
std::istream& operator>>(std::istream& s, value_type& t);

typedef int64_t integer;

// errors
struct type_mismatch : exception
{
 public:
    type_mismatch(value_type expected, value_type got);
    value_type expected() const { return expected_; }
    value_type got() const { return got_; }
 private:
    value_type expected_, got_;
};

// NIL

struct nil_type {};
static inline bool operator==(nil_type a, nil_type b)
{ return true; }
static inline bool operator!=(nil_type a, nil_type b)
{ return false; }
static inline bool operator<(nil_type a, nil_type b)
{ return false; }
static inline std::ostream& operator<<(std::ostream& s, nil_type x)
{ return s << "nil"; }
static inline raw_type_info get_type_info(nil_type)
{ return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::NIL)); }
static inline size_t deep_sizeof(nil_type)
{ return 0; }
} namespace std {
    template<>
    struct hash<cradle::nil_type>
    {
        size_t operator()(cradle::nil_type x) const
        {
            return 0;
        }
    };
} namespace cradle {
static nil_type nil;

// BLOBS

struct blob
{
    ownership_holder ownership;
    void const* data;
    std::size_t size;

    blob() : data(0), size(0) {}
};
bool operator==(blob const& a, blob const& b);
inline bool operator!=(blob const& a, blob const& b)
{ return !(a == b); }
bool operator<(blob const& a, blob const& b);

static inline raw_type_info get_type_info(blob const&)
{ return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::BLOB)); }
static inline size_t deep_sizeof(blob const& b)
{
    // This ignores the size of the ownership holder, but that's not a big deal.
    return sizeof(blob) + b.size;
}

} namespace std {
    template<>
    struct hash<cradle::blob>
    {
        size_t operator()(cradle::blob const& x) const
        {
            // TODO: hash for real?
            return 0;
        }
    };
} namespace cradle {

struct value;

// Lists are represented as std::vectors and can be manipulated as such.
typedef std::vector<value> value_list;

// MAPS

// Maps are represented as std::maps and can be manipulated as such.
typedef std::map<value,value> value_map;

// Note that many of these functions talk about record. A record is just a map
// where the keys are all strings.

// This queries a record for a field with a key matching the given string.
// If the field is not present in the map, an exception is thrown.
value get_field(value_map const& r, string const& field);

// This is the same as above, but its return value indicates whether or not
// the field is in the record.
bool get_field(value* v, value_map const& r, string const& field);


// Given a value_map that's meant to represent a union value, this checks that
// the map contains only one value and returns its key.
// This is used primarily by the preprocessor.
value const& get_union_value_type(value_map const& map);

// VALUES

struct value
{
    // STRUCTORS

    // Default construction creates a nil value.
    value() { set(nil); }

    // v must be one of the supported types or this will yield a compile-time
    // error.
    template<class T>
    explicit value(T const& v) { set(v); }

    // GETTERS

    value_type type() const { return type_; }

    // Get the value.
    // Requesting the wrong type will result in a type_mismatch exception.
    // If the type matches, *v will point to the value.
    void get(bool const** v) const;
    void get(integer const** v) const;
    void get(double const** v) const;
    void get(string const** v) const;
    void get(blob const** v) const;
    void get(boost::posix_time::ptime const** v) const;
    void get(value_list const** v) const;
    void get(value_map const** v) const;

    // SETTERS

    void set(nil_type _);
    void set(bool v);
    void set(integer v);
    void set(double v);
    void set(string const& v);
    void set(char const* v) { set(string(v)); }
    void set(blob const& v);
    void set(boost::posix_time::ptime const& v);
    void set(value_list const& v);
    void set(value_map const& v);

    // potentially more efficient setters which consume their arguments
    void swap_in(value_list& v);
    void swap_in(value_map& v);

    void swap_with(value& other);

 private:
    value_type type_;
    any value_;
};

// This is a generic function for reading a field from a record.
// It exists primarily so that omissible types can override it.
template<class Field>
void read_field_from_record(Field* field_value,
    value_map const& record, string const& field_name)
{
    auto dynamic_field_value = get_field(record, field_name);
    try
    {
        from_value(field_value, dynamic_field_value);
    }
    catch (cradle::exception& e)
    {
        e.add_context("in field " + field_name);
        throw;
    }
}

// This is a generic function for writing a field to a record.
// It exists primarily so that omissible types can override it.
template<class Field>
void write_field_to_record(value_map& record, string const& field_name,
    Field const& field_value)
{
    to_value(&record[value(field_name)], field_value);
}

static inline raw_type_info get_type_info(value const&)
{ return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::DYNAMIC)); }
size_t deep_sizeof(value const& v);
static inline void swap(value& a, value& b)
{ a.swap_with(b); }

template<class T>
T const& cast(value const& v)
{
    T const* x;
    v.get(&x);
    return *x;
}

template<class T>
void set(value& v, T const& x) { v.set(x); }

// comparison operators

bool operator==(value const& a, value const& b);
bool operator!=(value const& a, value const& b);
bool operator<(value const& a, value const& b);
bool operator<=(value const& a, value const& b);
bool operator>(value const& a, value const& b);
bool operator>=(value const& a, value const& b);

void value_to_string(string* s, value const& v);

// All regular CRADLE types provide to_value(&v, x) and from_value(&x, v).
// The following are alternate, often more convenient forms.
template<class T>
value to_value(T const& x)
{
    value v;
    to_value(&v, x);
    return v;
}
template<class T>
T from_value(value const& v)
{
    T x;
    from_value(&x, v);
    return x;
}

static inline void to_value(value* v, value const& x)
{ *v = x; }
static inline void from_value(value* x, value const& v)
{ *x = v; }

// Convert the given value to a string.
template<class T>
std::ostream& generic_ostream_operator(std::ostream& s, T const& x)
{
    value v;
    to_value(&v, x);
    string t;
    value_to_string(&t, v);
    s.write(t.c_str(), t.length());
    return s;
}

// Convert the given value to a string.
template<class T>
string to_string(T const& x)
{
    std::ostringstream s;
    s << x;
    return s.str();
}

// Convert the given value to a formatted string.
template<class T>
string to_string(T const& x, int precision)
{
    std::ostringstream s;
    s.precision(precision);
    s << x;
    return s.str();
}

struct invalid_enum_string : exception
{
    invalid_enum_string(
        raw_enum_info const& enum_type, string const& value)
      : exception("invalid " + enum_type.name + " value: " + value)
    {}
};
struct invalid_enum_value : exception
{
    invalid_enum_value(
        raw_enum_info const& enum_type, unsigned value)
      : exception("invalid " + enum_type.name + " value: " + to_string(value))
    {}
};
struct index_out_of_bounds : exception
{
    index_out_of_bounds(char const* label, size_t value, size_t upper_bound)
      : exception(string("index out of bounds: ") + label +
            "; value: " + to_string(value) +
            "; upper bound: " + to_string(upper_bound))
    {}
};

// Check that an index is in bounds.
// index must be nonnegative and strictly less than upper_bound to pass.
void check_index_bounds(char const* label, size_t index, size_t upper_bound);

// Apply the functor fn to the value v.
// fn must have the function call operator overloaded for all supported
// types (including nil). If it doesn't, you'll get a compile-time error.
// fn is passed as a non-const reference so that it can accumulate results.
template<class Fn>
void apply_fn_to_value(Fn& fn, value const& v)
{
    switch (v.type())
    {
     case value_type::NIL:
        fn(nil);
        break;
     case value_type::BOOLEAN:
        fn(cast<bool>(v));
        break;
     case value_type::INTEGER:
        fn(cast<integer>(v));
        break;
     case value_type::FLOAT:
        fn(cast<double>(v));
        break;
     case value_type::STRING:
        fn(cast<string>(v));
        break;
     case value_type::BLOB:
        fn(cast<blob>(v));
        break;
     case value_type::DATETIME:
        fn(cast<boost::posix_time::ptime>(v));
        break;
     case value_type::LIST:
        fn(cast<value_list>(v));
        break;
     case value_type::MAP:
        fn(cast<value_map>(v));
        break;
    }
}

// Apply the functor fn to two values of the same type.
// If a and b are not the same type, this throws a type_mismatch exception.
template<class Fn>
void apply_fn_to_value_pair(Fn& fn, value const& a, value const& b)
{
    if (a.type() != b.type())
        throw type_mismatch(a.type(), b.type());
    switch (a.type())
    {
     case value_type::NIL:
        fn(nil, nil);
        break;
     case value_type::BOOLEAN:
        fn(cast<bool>(a), cast<bool>(b));
        break;
     case value_type::INTEGER:
        fn(cast<integer>(a), cast<integer>(b));
        break;
     case value_type::FLOAT:
        fn(cast<double>(a), cast<double>(b));
        break;
     case value_type::STRING:
        fn(cast<string>(a), cast<string>(b));
        break;
     case value_type::BLOB:
        fn(cast<blob>(a), cast<blob>(b));
        break;
     case value_type::DATETIME:
        fn(cast<boost::posix_time::ptime>(a), cast<boost::posix_time::ptime>(b));
        break;
     case value_type::LIST:
        fn(cast<value_list>(a), cast<value_list>(b));
        break;
     case value_type::MAP:
        fn(cast<value_map>(a), cast<value_map>(b));
        break;
    }
}

} namespace std {
    template<>
    struct hash<cradle::value>
    {
        size_t operator()(cradle::value const& x) const;
    };
} namespace cradle {

// text I/O

std::ostream& operator<<(std::ostream& s, value const& v);

void parse_value_string(value* v, string const& s);
void value_to_string(string* s, value const& v);

// CRADLE type interface for various built-in types

static inline raw_type_info get_type_info(bool)
{ return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::BOOLEAN)); }
static inline size_t deep_sizeof(bool)
{ return sizeof(bool); }
void to_value(value* v, bool x);
void from_value(bool* x, value const& v);

static inline raw_type_info get_type_info(string const&)
{ return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::STRING)); }
static inline size_t deep_sizeof(string const& x)
{ return sizeof(string) + sizeof(char) * x.length(); }
void to_value(value* v, string const& x);
void from_value(string* x, value const& v);

static inline void to_value(value* v, nil_type n) { v->set(n); }
static inline void from_value(nil_type* n, value const& v) {}

void to_value(value* v, blob const& x);
void from_value(blob* x, value const& v);

#define CRADLE_DECLARE_NUMBER_INTERFACE(T) \
    void to_value(value* v, T x); \
    void from_value(T* x, value const& v); \
    static inline size_t deep_sizeof(T) { return sizeof(T); }

#define CRADLE_DECLARE_INTEGER_INTERFACE(T) \
    integer to_integer(T x); \
    void from_integer(T* x, integer n); \
    raw_type_info get_type_info(T); \
    CRADLE_DECLARE_NUMBER_INTERFACE(T)

CRADLE_DECLARE_INTEGER_INTERFACE(signed char)
CRADLE_DECLARE_INTEGER_INTERFACE(unsigned char)
CRADLE_DECLARE_INTEGER_INTERFACE(signed short)
CRADLE_DECLARE_INTEGER_INTERFACE(unsigned short)
CRADLE_DECLARE_INTEGER_INTERFACE(signed int)
CRADLE_DECLARE_INTEGER_INTERFACE(unsigned int)
CRADLE_DECLARE_INTEGER_INTERFACE(signed long)
CRADLE_DECLARE_INTEGER_INTERFACE(unsigned long)
CRADLE_DECLARE_INTEGER_INTERFACE(signed long long)
CRADLE_DECLARE_INTEGER_INTERFACE(unsigned long long)

#define CRADLE_DECLARE_FLOAT_INTERFACE(T) \
    raw_type_info get_type_info(T); \
    CRADLE_DECLARE_NUMBER_INTERFACE(T)

CRADLE_DECLARE_FLOAT_INTERFACE(float)
CRADLE_DECLARE_FLOAT_INTERFACE(double)

// CRADLE type interface for std::vectors

template<class T>
void to_value(value* v, std::vector<T> const& x)
{
    value_list l;
    size_t n_elements = x.size();
    l.resize(n_elements);
    for (size_t i = 0; i != n_elements; ++i)
        to_value(&l[i], x[i]);
    v->swap_in(l);
}
template<class T>
void from_value(std::vector<T>* x, value const& v)
{
    value_list const& l = cast<value_list>(v);
    size_t n_elements = l.size();
    x->resize(n_elements);
    for (size_t i = 0; i != n_elements; ++i)
        from_value(&(*x)[i], l[i]);
}

} namespace std {
    template<class Item>
    struct hash<vector<Item> >
    {
        size_t operator()(vector<Item> const& x) const
        {
            size_t h = 0;
            for (auto const& i : x)
                h = alia::combine_hashes(h, alia::invoke_hash(i));
            return h;
        }
    };
} namespace alia {

template<class T>
std::ostream& operator<<(std::ostream& s, std::vector<T> const& x)
{
    s << "{";
    size_t n_elements = x.size();
    for (unsigned i = 0; i != n_elements; ++i)
    {
        if (i != 0)
            s << ",";
        s << x[i];
    }
    s << "}";
    return s;
}

} namespace cradle {

// forward declared for use by another forward declaration below
} namespace ClipperLib {
    struct IntPoint;
    typedef std::vector<std::vector<IntPoint> > Paths;
    typedef Paths Polygons;
} namespace cradle {

// forward declaring here so that compiler won't have the vector version
// trump the polyset version way over in geometry/clipper.hpp/.cpp
typedef ClipperLib::Polygons clipper_polyset;
raw_type_info get_type_info(clipper_polyset);

template<class T>
raw_type_info get_type_info(std::vector<T> const&)
{
    return raw_type_info(raw_kind::ARRAY,
        any(raw_array_info(none, get_type_info(T()))));
}

template<class T>
size_t deep_sizeof(std::vector<T> const& x)
{
    size_t size = sizeof(std::vector<T>);
    for (auto const& i : x)
        size += deep_sizeof(i);
    return size;
}

// CRADLE type interface for std::maps

template<class Key, class Value>
void to_value(value* v, std::map<Key,Value> const& x)
{
    value_map record;
    for (auto const& i : x)
        to_value(&record[to_value(i.first)], i.second);
    v->swap_in(record);
}
template<class Key, class Value>
void from_value(std::map<Key,Value>* x, value const& v)
{
    x->clear();
    value_map const& record = cast<value_map>(v);
    for (auto const& i : record)
        from_value(&(*x)[from_value<Key>(i.first)], i.second);
}

} namespace std {
    template<class Key, class Value>
    struct hash<map<Key,Value> >
    {
        size_t operator()(map<Key,Value> const& x) const
        {
            size_t h = 0;
            for (auto const& i : x)
            {
                h = alia::combine_hashes(h,
                        alia::combine_hashes(
                            alia::invoke_hash(i.first),
                            alia::invoke_hash(i.second)));
            }
            return h;
        }
    };
} namespace alia {
template<class Key, class Value>
std::ostream& operator<<(std::ostream& s, std::map<Key,Value> const& x)
{
    s << "{";
    for (typename std::map<Key,Value>::const_iterator
        i = x.begin(); i != x.end(); ++i)
    {
        if (i != x.begin())
            s << ";";
        s << "(" << i->first << ":" << i->second << ")";
    }
    s << "}";
    return s;
}
} namespace cradle {

using alia::operator<<;

template<class Key, class Value>
raw_type_info get_type_info(std::map<Key,Value> const&)
{
    return raw_type_info(raw_kind::MAP,
        any(raw_map_info(get_type_info(Key()), get_type_info(Value()))));
}

template<class Key, class Value>
size_t deep_sizeof(std::map<Key,Value> const& x)
{
    size_t size = sizeof(std::map<Key,Value>);
    for (auto const& i : x)
        size += deep_sizeof(i.first) + deep_sizeof(i.second);
    return size;
}

// array size checking utility

struct array_size_error : exception
{
    array_size_error(size_t expected_size, size_t actual_size);
    ~array_size_error() throw() {}
    size_t expected_size, actual_size;
};

void check_array_size(size_t expected_size, size_t actual_size);

#define CRADLE_REGULAR_TYPE_WRAPPER(templating, Wrapper, field) \
    templating \
    void to_value(cradle::value* v, Wrapper const& x) \
    { to_value(v, x.field); \
    templating \
    void from_value(Wrapper* x, cradle::value const& v) \
    { from_value(&x->field, v); \
    templating \
    bool operator==(Wrapper const& x, Wrapper const& y) \
    { return x.field == y.field; } \
    templating \
    bool operator!=(Wrapper const& x, Wrapper const& y) \
    { return x.field != y.field; } \
    templating \
    bool operator<(Wrapper const& x, Wrapper const& y) \
    { return x.field < y.field; }

}

namespace alia {

template<class T>
cradle::raw_type_info get_type_info(optional<T> const&)
{
    using cradle::get_type_info;
    return cradle::raw_type_info(cradle::raw_kind::OPTIONAL,
        cradle::any(get_type_info(T())));
}
template<class T>
size_t deep_sizeof(optional<T> const& x)
{
    using cradle::deep_sizeof;
    return sizeof(optional<T>) + (x ? deep_sizeof(get(x)) : 0);
}

template<class T>
void to_value(cradle::value* v, optional<T> const& x)
{
    cradle::value_map record;
    if (x)
    {
        to_value(&record[cradle::value("some")], get(x));
    }
    else
    {
        to_value(&record[cradle::value("none")], cradle::nil);
    }
    set(*v, record);
}
template<class T>
void from_value(optional<T>* x, cradle::value const& v)
{
    cradle::value_map const& record = cradle::cast<cradle::value_map>(v);
    string type;
    from_value(&type, cradle::get_union_value_type(record));
    if (type == "some")
    {
        T t;
        from_value(&t, cradle::get_field(record, "some"));
        *x = t;
    }
    else if (type == "none")
        *x = none;
    else
        throw cradle::exception("invalid optional type");
}
template<class T>
std::ostream& operator<<(std::ostream& s, optional<T> const& x)
{
    if (x)
        s << get(x);
    else
        s << "none";
    return s;
}
} namespace std {
    template<class T>
    struct hash<alia::optional<T> >
    {
        size_t operator()(alia::optional<T> const& x) const
        {
            return x ? alia::invoke_hash(get(x)) : 0;
        }
    };
} namespace alia {

template<unsigned N, class T>
cradle::raw_type_info get_type_info(vector<N,T>)
{
    using cradle::get_type_info;
    return cradle::raw_type_info(cradle::raw_kind::ARRAY,
        cradle::any(cradle::raw_array_info(N, get_type_info(T()))));
}
template<unsigned N, class T>
size_t deep_sizeof(vector<N,T> const& x)
{
    // We assume that vectors are only used with types that have a static
    // deep_sizeof.
    using cradle::deep_sizeof;
    return deep_sizeof(T()) * N;
}
template<unsigned N, class T>
void from_value(vector<N,T>* x, cradle::value const& v)
{
    using cradle::from_value;
    cradle::value_list const& l = cradle::cast<cradle::value_list>(v);
    cradle::check_array_size(N, l.size());
    for (unsigned i = 0; i != N; ++i)
        from_value(&(*x)[i], l[i]);
}
template<unsigned N, class T>
void to_value(cradle::value* v, vector<N,T> const& x)
{
    using cradle::to_value;
    cradle::value_list l(N);
    for (unsigned i = 0; i != N; ++i)
        to_value(&l[i], x[i]);
    v->swap_in(l);
}

}

namespace cradle {

// OTHER UTILITIES

// omissible<T> is the same as optional<T>, but it obeys thinknode's behavior
// foe omissible fields. (It should only be used as a field in a structure.)
// optional<T> stores an optional value of type T (or no value).
template<class T>
struct omissible
{
    typedef T value_type;
    omissible() : valid_(false) {}
    omissible(T const& value) : value_(value), valid_(true) {}
    omissible(none_type) : valid_(false) {}
    omissible(optional<T> const& opt)
      : valid_(opt ? true : false), value_(opt ? opt.get() : T())
    {}
    omissible& operator=(T const& value)
    { value_ = value; valid_ = true; return *this; }
    omissible& operator=(none_type)
    { valid_ = false; return *this; }
    omissible& operator=(optional<T> const& opt)
    {
        valid_ = opt ? true : false;
        value_ = opt ? opt.get() : T();
        return *this;
    }
    // allows use within if statements without other unintended conversions
    typedef bool omissible::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return valid_ ? &omissible::valid_ : 0; }
    T const& get() const
    { assert(valid_); return value_; }
    T& get()
    { assert(valid_); return value_; }
 private:
    T value_;
    bool valid_;
};
template<class T>
T const& get(omissible<T> const& omis)
{ return omis.get(); }
template<class T>
T& get(omissible<T>& omis)
{ return omis.get(); }
template<class T>
optional<T> as_optional(omissible<T> const& omis)
{ return omis ? some(get(omis)) : optional<T>(); }

template<class T>
bool operator==(omissible<T> const& a, omissible<T> const& b)
{
    return a ? (b && get(a) == get(b)) : !b;
}
template<class T>
bool operator!=(omissible<T> const& a, omissible<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(omissible<T> const& a, omissible<T> const& b)
{
    return b && (a ? get(a) < get(b) : true);
}
template<class T>
cradle::raw_type_info get_type_info(omissible<T> const&)
{
    using cradle::get_type_info;
    return cradle::raw_type_info(cradle::raw_kind::OMISSIBLE,
        cradle::any(get_type_info(T())));
}
template<class T>
size_t deep_sizeof(omissible<T> const& x)
{
    return sizeof(omissible<T>) + (x ? deep_sizeof(get(x)) : 0);
}
template<class T>
void read_field_from_record(omissible<T>* field_value,
    value_map const& record, string const& field_name)
{
    // If the field doesn't appear in the record, just set it to none.
    value dynamic_field_value;
    if (get_field(&dynamic_field_value, record, field_name))
    {
        try
        {
            T value;
            from_value(&value, dynamic_field_value);
            *field_value = value;
        }
        catch (cradle::exception& e)
        {
            e.add_context("in field " + field_name);
            throw;
        }
    }
    else
        *field_value = none;
}
template<class T>
void write_field_to_record(value_map& record, string const& field_name,
    omissible<T> const& field_value)
{
    // Only write the field to the record if it has a value.
    if (field_value)
        write_field_to_record(record, field_name, get(field_value));
}

template<class T>
void to_value(cradle::value* v, omissible<T> const& x)
{
    cradle::value_map record;
    if (x)
    {
        to_value(&record[value("some")], get(x));
    }
    else
    {
        to_value(&record[value("none")], cradle::nil);
    }
    set(*v, record);
}
template<class T>
void from_value(omissible<T>* x, cradle::value const& v)
{
    cradle::value_map const& record = cradle::cast<cradle::value_map>(v);
    string type;
    from_value(&type, cradle::get_union_value_type(record));
    if (type == "some")
    {
        T t;
        from_value(&t, cradle::get_field(record, "some"));
        *x = t;
    }
    else if (type == "none")
        *x = none;
    else
        throw cradle::exception("invalid omissible type");
}
template<class T>
std::ostream& operator<<(std::ostream& s, omissible<T> const& x)
{
    if (x)
        s << get(x);
    else
        s << "none";
    return s;
}
} namespace std {
    template<class T>
    struct hash<cradle::omissible<T> >
    {
        size_t operator()(cradle::omissible<T> const& x) const
        {
            return x ? alia::invoke_hash(get(x)) : 0;
        }
    };
} namespace cradle {

// C arrays that act as regular CRADLE types
template<unsigned N, class T>
struct c_array
{
    T const& operator[](size_t i) const { return elements[i]; }
    T& operator[](size_t i) { return elements[i]; }

    typedef T value_type;

    static unsigned const length = N;

    T elements[N];

    // STL-like interface
    T const* begin() const { return &elements[0]; }
    T const* end() const { return begin() + N; }
    T* begin() { return &elements[0]; }
    T* end() { return begin() + N; }
    size_t size() const { return size_t(N); }
};
template<unsigned N, class T>
bool operator==(c_array<N,T> const& a, c_array<N,T> const& b)
{
    for (unsigned i = 0; i != N; ++i)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}
template<unsigned N, class T>
bool operator!=(c_array<N,T> const& a, c_array<N,T> const& b)
{
    return !(a == b);
}
template<unsigned N, class T>
bool operator<(c_array<N,T> const& a, c_array<N,T> const& b)
{
    for (unsigned i = 0; i != N; ++i)
    {
        if (a[i] < b[i])
            return true;
        if (b[i] < a[i])
            return false;
    }
    return false;
}
template<unsigned N, class T>
raw_type_info get_type_info(c_array<N,T> const&)
{
    return raw_type_info(raw_kind::ARRAY,
        any(raw_array_info(N, get_type_info(T()))));
}
template<unsigned N, class T>
size_t deep_sizeof(c_array<N,T> const& x)
{
    size_t size = 0;
    for (auto const& element : x)
    {
        size += deep_sizeof(element);
    }
    return size;
}
template<unsigned N, class T>
void swap(c_array<N,T>& a, c_array<N,T>& b)
{
    for (unsigned i = 0; i != N; ++i)
        swap(a[i], b[i]);
}
template<unsigned N, class T>
void from_value(c_array<N,T>* x, value const& v)
{
    value_list const& l = cast<value_list>(v);
    check_array_size(N, l.size());
    for (unsigned i = 0; i != N; ++i)
        from_value(&(*x)[i], l[i]);
}
template<unsigned N, class T>
void to_value(value* v, c_array<N,T> const& x)
{
    value_list l(N);
    for (unsigned i = 0; i != N; ++i)
        to_value(&l[i], x[i]);
    v->swap_in(l);
}
template<unsigned N, class T>
std::ostream& operator<<(std::ostream& s, c_array<N,T> const& x)
{
    s << "{";
    for (unsigned i = 0; i != N; ++i)
    {
        if (i != 0)
            s << ",";
        s << x[i];
    }
    s << "}";
    return s;
}
} namespace std {
    template<unsigned N, class T>
    struct hash<cradle::c_array<N,T> >
    {
        size_t operator()(cradle::c_array<N,T> const& x) const
        {
            size_t h = 0;
            for (size_t i = 0; i != N; ++i)
                h = alia::combine_hashes(h, alia::invoke_hash(x[i]));
            return h;
        }
    };
} namespace cradle {

// array stores an immutable array of values with unspecified ownership.
// Note that arrays are externalized as blobs, so only POD types are allowed.
// (Well, nearly POD, since we want to allow the constructors that the
// preprocessor generates.)
template<class T>
struct array
{
    // This doesn't seem to work on GCC, at least not 4.6.
  #ifdef _MSC_VER
    static_assert(std::is_trivially_destructible<T>::value &&
        std::is_standard_layout<T>::value,
        "array elements must be POD types.");
  #endif

    array() : elements(0), n_elements(0) {}

    T const* elements;
    size_t n_elements;
    ownership_holder ownership;

    // STL-like interface
    typedef T value_type;
    typedef T const* const_iterator;
    typedef const_iterator iterator;
    const_iterator begin() const { return elements; }
    const_iterator end() const { return elements + n_elements; }
    typedef size_t size_type;
    size_t size() const { return n_elements; }
    T const& operator[](size_t i) const { return elements[i]; }
};

template<typename T>
struct array_deleter
{
   void operator()(T* p)
   {
      delete [] p;
   }
};
template<class T>
T* allocate(array<T>* array, size_t n_elements)
{
    alia__shared_ptr<T> ptr(new T[n_elements], array_deleter<T>());
    array->ownership = ptr;
    array->elements = ptr.get();
    array->n_elements = n_elements;
    return ptr.get();
}

template<class T>
void clear(array<T>* array)
{
    array->elements = 0;
    array->n_elements = 0;
    array->ownership = ownership_holder();
}

template<class T>
void initialize(array<T>* array, std::vector<T> const& vector)
{
    size_t n_elements = vector.size();
    alia__shared_ptr<T> ptr(new T[n_elements], array_deleter<T>());
    array->ownership = ptr;
    T* elements = ptr.get();
    array->elements = elements;
    array->n_elements = n_elements;
    for (size_t i = 0; i != n_elements; ++i)
        elements[i] = vector[i];
}

template<class T>
raw_type_info get_type_info(array<T>)
{
    return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::BLOB));
}

template<class T>
size_t deep_sizeof(array<T> const& x)
{
    // We assume that c_array is only used with types that have a static
    // deep_sizeof.
    return deep_sizeof(T()) * x.size();
}

template<class T>
void swap(array<T>& a, array<T>& b)
{
    swap(a.elements, b.elements);
    swap(a.n_elements, b.n_elements);
    swap(a.ownership, b.ownership);
}

template<class T>
bool operator==(array<T> const& a, array<T> const& b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i != a.size(); ++i)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}
template<class T>
bool operator!=(array<T> const& a, array<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(array<T> const& a, array<T> const& b)
{
    if (a.size() < b.size())
        return true;
    if (b.size() < a.size())
        return false;
    for (size_t i = 0; i != a.size(); ++i)
    {
        if (a[i] < b[i])
            return true;
        if (b[i] < a[i])
            return false;
    }
    return false;
}

template<class T>
void from_value(array<T>* x, value const& v)
{
    blob const& b = cast<blob>(v);
    x->n_elements = b.size / sizeof(T);
    x->elements = reinterpret_cast<T const*>(b.data);
    x->ownership = b.ownership;
}
template<class T>
void to_value(value* v, array<T> const& x)
{
    blob b;
    b.size = x.n_elements * sizeof(T);
    b.data = reinterpret_cast<void const*>(x.elements);
    b.ownership = x.ownership;
    set(*v, b);
}

template<class T>
std::ostream& operator<<(std::ostream& s, array<T> const& x)
{
    s << "{";
    for (unsigned i = 0; i != x.n_elements; ++i)
    {
        if (i != 0)
            s << ",";
        s << x.elements[i];
    }
    s << "}";
    return s;
}

} namespace std {
    template<class T>
    struct hash<cradle::array<T> >
    {
        size_t operator()(cradle::array<T> const& x) const
        {
            size_t h = 0;
            for (auto const& i : x)
                h = alia::combine_hashes(h, alia::invoke_hash(i));
            return h;
        }
    };
} namespace cradle {

// immutable<T> holds (by shared_ptr) an immutable value of type T.
// immutable also provides the ability to efficiently store values in a
// dynamically-typed way. Any immutable<T> can be cast to and from
// untyped_immutable.

struct untyped_immutable_value : noncopyable
{
    virtual ~untyped_immutable_value() {}
    virtual raw_type_info type_info() const = 0;
    virtual size_t deep_size() const = 0;
    virtual size_t hash() const = 0;
    virtual value as_value() const = 0;
    virtual bool equals(untyped_immutable_value const* other) const = 0;
};

struct untyped_immutable
{
    alia__shared_ptr<untyped_immutable_value> ptr;
};

bool static inline
is_initialized(untyped_immutable const& x)
{ return x.ptr ? true : false; }

static inline untyped_immutable_value const*
get_value_pointer(untyped_immutable const& x)
{ return x.ptr.get(); }

void static inline
reset(untyped_immutable& x)
{ x.ptr.reset(); }

template<class T>
struct immutable_value : untyped_immutable_value
{
    T value;
    raw_type_info type_info() const { return get_type_info(T()); }
    size_t deep_size() const { return deep_sizeof(this->value); }
    size_t hash() const { return invoke_hash(this->value); }
    cradle::value as_value() const { return to_value(this->value); }
    bool equals(untyped_immutable_value const* other) const
    {
        auto const* typed_other =
            dynamic_cast<immutable_value<T> const*>(other);
        return typed_other && this->value == typed_other->value;
    }
};

template<class T>
struct immutable
{
    alia__shared_ptr<immutable_value<T> > ptr;
};

template<class T>
bool is_initialized(immutable<T> const& x)
{ return x.ptr ? true : false; }

template<class T>
void reset(immutable<T>& x)
{ x.ptr.reset(); }

template<class T>
void initialize(immutable<T>& x, T const& value)
{
    x.ptr.reset(new immutable_value<T>);
    x.ptr->value = value;
}

template<class T>
immutable<T> make_immutable(T const& value)
{
    immutable<T> x;
    initialize(x, value);
    return x;
}

} namespace std {
    template<class T>
    struct hash<cradle::immutable<T> >
    {
        size_t operator()(cradle::immutable<T> const& x) const
        {
            return is_initialized(x) ? alia::invoke_hash(get(x)) : 0;
        }
    };
} namespace cradle {

template<class T>
void swap_in(immutable<T>& x, T& value)
{
    x.ptr.reset(new immutable_value<T>);
    swap(x.ptr->value, value);
}

template<class T>
T const& get(immutable<T> const& x)
{ return x.ptr->value; }

template<class T>
raw_type_info get_type_info(immutable<T>)
{ return get_type_info(T()); }

template<class T>
size_t deep_sizeof(immutable<T> const& x)
{ return x.ptr->deep_size(); }

template<class T>
void swap(immutable<T>& a, immutable<T>& b)
{ swap(a.ptr, b.ptr); }

template<class T>
bool operator==(immutable<T> const& a, immutable<T> const& b)
{
    // First test if the two immutables are actually pointing to the same
    // thing, which avoids having to do a deep comparison for this case.
    return a.ptr == b.ptr ||
        (a.ptr ? (b.ptr && get(a) == get(b)) : !b.ptr);
}
template<class T>
bool operator!=(immutable<T> const& a, immutable<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(immutable<T> const& a, immutable<T> const& b)
{
    return b.ptr && (a.ptr ? get(a) < get(b) : true);
}

template<class T>
void from_value(immutable<T>* x, cradle::value const& v)
{
    using cradle::from_value;
    x->ptr.reset(new immutable_value<T>);
    from_value(&x->ptr->value, v);
}
template<class T>
void to_value(cradle::value* v, immutable<T> const& x)
{
    using cradle::to_value;
    if (x.ptr)
        to_value(v, get(x));
    else
        to_value(v, T());
}

template<class T>
std::ostream& operator<<(std::ostream& s, immutable<T> const& x)
{
    if (x.ptr)
        s << get(x);
    else
        s << T();
    return s;
}

struct immutable_data_type_mismatch : exception
{
 public:
    immutable_data_type_mismatch(
        raw_type_info const& expected, raw_type_info const& got);
    ~immutable_data_type_mismatch() throw() {}
    raw_type_info const& expected() const { return *expected_; }
    raw_type_info const& got() const { return *got_; }
 private:
    alia__shared_ptr<raw_type_info> expected_, got_;
};

// Cast an untyped_immutable to a typed one.
template<class T>
immutable<T>
cast_immutable(untyped_immutable const& untyped)
{
    immutable<T> typed;
    if (untyped.ptr)
    {
        typed.ptr =
            std::dynamic_pointer_cast<immutable_value<T> >(untyped.ptr);
        if (!typed.ptr)
        {
            throw immutable_data_type_mismatch(
                get_type_info(T()), untyped.ptr->type_info());
        }
    }
    return typed;
}

// Cast an untyped_immutable to a typed one.
template<class T>
void from_immutable(T* value, untyped_immutable const& untyped)
{
    *value = get(cast_immutable<T>(untyped));
}

// This is a lower level form of cast that works directly on the value
// pointers.
template<class T>
void cast_immutable_value(T const** typed_value,
    untyped_immutable_value const* untyped)
{
    immutable_value<T> const* typed =
        dynamic_cast<immutable_value<T> const*>(untyped);
    if (!typed)
    {
        throw immutable_data_type_mismatch(
            get_type_info(T()), untyped->type_info());
    }
    *typed_value = &typed->value;
}

// Erase the compile-time type information associated with the given immutable
// to produce an untyped_immutable.
template<class T>
untyped_immutable
erase_type(immutable<T> const& typed)
{
    untyped_immutable untyped;
    untyped.ptr = std::static_pointer_cast<untyped_immutable_value>(typed.ptr);
    return untyped;
}

template<class T>
untyped_immutable
swap_in_and_erase_type(T& value)
{
    immutable<T> tmp;
    swap_in(tmp, value);
    return erase_type(tmp);
}

untyped_immutable
get_field(std::map<string,untyped_immutable> const& fields,
    string const& field_name);

// object_reference<T> holds a reference to object data.

template<class T>
struct object_reference
{
    object_reference() {}
    object_reference(string const& uid) : uid(uid) {}

    string uid;
};

template<class T>
object_reference<T>
make_object_reference(string const& uid)
{
    object_reference<T> ref;
    ref.uid = uid;
    return ref;
}

template<class T>
raw_type_info get_type_info(object_reference<T>)
{
    return raw_type_info(raw_kind::DATA_REFERENCE, any(get_type_info(T())));
}

template<class T>
size_t deep_sizeof(object_reference<T> const& x)
{
    return deep_sizeof(x.uid);
}

template<class T>
void swap(object_reference<T>& a, object_reference<T>& b)
{ swap(a.uid, b.uid); }

template<class T>
bool operator==(object_reference<T> const& a, object_reference<T> const& b)
{
    return a.uid == b.uid;
}
template<class T>
bool operator!=(object_reference<T> const& a, object_reference<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(object_reference<T> const& a, object_reference<T> const& b)
{
    return a.uid < b.uid;
}

template<class T>
void from_value(object_reference<T>* x, cradle::value const& v)
{
    using cradle::from_value;
    from_value(&x->uid, v);
}
template<class T>
void to_value(cradle::value* v, object_reference<T> const& x)
{
    using cradle::to_value;
    to_value(v, x.uid);
}

template<class T>
std::ostream& operator<<(std::ostream& s, object_reference<T> const& x)
{
    s << x.uid;
    return s;
}
} namespace std {
    template<class T>
    struct hash<cradle::object_reference<T> >
    {
        size_t operator()(cradle::object_reference<T> const& x) const
        {
            return alia::invoke_hash(x.uid);
        }
    };
} namespace cradle {


 //immutable_reference<T> holds a reference to immutable data.

template<class T>
struct immutable_reference
{
    immutable_reference() {}
    immutable_reference(string const& uid) : uid(uid) {}

    string uid;
};

template<class T>
immutable_reference<T>
make_immutable_reference(string const& uid)
{
    immutable_reference<T> ref;
    ref.uid = uid;
    return ref;
}

template<class T>
raw_type_info get_type_info(immutable_reference<T>)
{
    return raw_type_info(raw_kind::DATA_REFERENCE, any(get_type_info(T())));
}

template<class T>
size_t deep_sizeof(immutable_reference<T> const& x)
{
    return deep_sizeof(x.uid);
}

template<class T>
void swap(immutable_reference<T>& a, immutable_reference<T>& b)
{ swap(a.uid, b.uid); }

template<class T>
bool operator==(immutable_reference<T> const& a, immutable_reference<T> const& b)
{
    return a.uid == b.uid;
}
template<class T>
bool operator!=(immutable_reference<T> const& a, immutable_reference<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(immutable_reference<T> const& a, immutable_reference<T> const& b)
{
    return a.uid < b.uid;
}

template<class T>
void from_value(immutable_reference<T>* x, cradle::value const& v)
{
    using cradle::from_value;
    from_value(&x->uid, v);
}
template<class T>
void to_value(cradle::value* v, immutable_reference<T> const& x)
{
    using cradle::to_value;
    to_value(v, x.uid);
}

template<class T>
std::ostream& operator<<(std::ostream& s, immutable_reference<T> const& x)
{
    s << x.uid;
    return s;
}

}
namespace std {
    template<class T>
    struct hash<cradle::immutable_reference<T> >
    {
        size_t operator()(cradle::immutable_reference<T> const& x) const
        {
            return alia::invoke_hash(x.uid);
        }
    };
}
namespace cradle {

// This is used to represent lists of values (possibly with heterogenous
// types) at compile time.

struct empty_compile_time_list {};

template<class Head, class Tail>
struct compile_time_list
{
    Head head;
    Tail tail;
};

// ensure_default_initialization(x), where x is a non-const reference to an
// object that has just been default-constructed, ensures that x is properly
// initialized to a sane default value.
template<class T>
void ensure_default_initialization(T& x)
{}
static inline
void ensure_default_initialization(bool& x)
{ x = false; }
#define CRADLE_DECLARE_NUMBER_INITIALIZER(T) \
    static inline \
    void ensure_default_initialization(T& x) \
    { x = 0; }
CRADLE_DECLARE_NUMBER_INITIALIZER(signed char)
CRADLE_DECLARE_NUMBER_INITIALIZER(unsigned char)
CRADLE_DECLARE_NUMBER_INITIALIZER(signed short)
CRADLE_DECLARE_NUMBER_INITIALIZER(unsigned short)
CRADLE_DECLARE_NUMBER_INITIALIZER(signed int)
CRADLE_DECLARE_NUMBER_INITIALIZER(unsigned int)
CRADLE_DECLARE_NUMBER_INITIALIZER(signed long)
CRADLE_DECLARE_NUMBER_INITIALIZER(unsigned long)
CRADLE_DECLARE_NUMBER_INITIALIZER(signed long long)
CRADLE_DECLARE_NUMBER_INITIALIZER(unsigned long long)
CRADLE_DECLARE_NUMBER_INITIALIZER(float)
CRADLE_DECLARE_NUMBER_INITIALIZER(double)

// default_initialized<T>() returns a default initialized object of type T.
template<class T>
T default_initialized()
{
    T x;
    ensure_default_initialization(x);
    return x;
}

// This is used to uniquely identify functions within CRADLE.
struct resolved_function_uid
{
    resolved_function_uid() {}
    resolved_function_uid(string const& app, string const& uid)
      : app(app), uid(uid)
    {}

    string app;
    string uid;
};

bool static inline
operator==(resolved_function_uid const& a, resolved_function_uid const& b)
{ return a.app == b.app && a.uid == b.uid; }
bool static inline
operator!=(resolved_function_uid const& a, resolved_function_uid const& b)
{ return !(a == b); }
bool static inline
operator<(resolved_function_uid const& a, resolved_function_uid const& b)
{ return a.app < b.app || (a.app == b.app && a.uid < b.uid); }

std::ostream& operator<<(std::ostream& s, resolved_function_uid const& id);

} namespace std {
    template<>
    struct hash<cradle::resolved_function_uid>
    {
        size_t operator()(cradle::resolved_function_uid const& x) const
        {
            return alia::combine_hashes(
                alia::invoke_hash(x.app), alia::invoke_hash(x.uid));
        }
    };
} namespace cradle {

// DYNAMIC TYPE INTERFACE - This provides a dynamic interface to retrieving
// information about a type and converting values to and from it.

struct dynamic_type_interface
{
    virtual std::type_info const& cpp_typeid() const = 0;

    virtual raw_type_info type_info() const = 0;

    virtual untyped_immutable
    value_to_immutable(value const& dynamic_value) const = 0;

    virtual value
    immutable_to_value(untyped_immutable const& immutable) const = 0;
};

template<class Value>
struct dynamic_type_implementation : dynamic_type_interface
{
    virtual std::type_info const& cpp_typeid() const
    {
        return typeid(Value);
    }

    raw_type_info type_info() const
    {
        return get_type_info(Value());
    }

    untyped_immutable
    value_to_immutable(value const& dynamic_value) const
    {
        Value typed_value;
        from_value(&typed_value, dynamic_value);
        return swap_in_and_erase_type(typed_value);
    }

    value
    immutable_to_value(untyped_immutable const& immutable) const
    {
        Value const* typed_value;
        cast_immutable_value(&typed_value, get_value_pointer(immutable));
        return to_value(*typed_value);
    }
};

template<class Value>
struct dynamic_type_interface_maker
{
    static dynamic_type_interface const* invoke()
    {
        static dynamic_type_implementation<Value> the_interface;
        return &the_interface;
    }
};

// DATA UPGRADES


//// an empty mutation upgrade (equivalent to a cast)
////    TODO: Implement EMPTY

// Denotes the different kind of upgrades that are available.
api(enum)
enum class upgrade_type
{
    // no upgrade
    NONE,
    // an upgrade via a custom function
    FUNCTION
};

// Returns greater upgrade value as determined by the enum values from upgrade_type
// This is used to parse down through the types so structs that have a field that
// has been modified will be upgraded.
upgrade_type static
merged_upgrade_type(upgrade_type a, upgrade_type b)
{
    return a > b ? a : b;
}

// Gets the explicit update type for values. This is the default implementation
// of this and it's expected to be overridden by the user for any type they are defining
// an upgrade for.
template<class T>
upgrade_type
get_explicit_upgrade_type(T const&)
{
    return upgrade_type::NONE;
}

#define upgrade_struct(T) \
    upgrade_type static \
    get_explicit_upgrade_type(T const&) \
    { \
        return upgrade_type::FUNCTION; \
    }

// Gets the update type for values. This is the default implementation
// of this and it's expected to be overridden.
template<class T>
upgrade_type
get_upgrade_type(T const&, std::vector<std::type_index> parsed_types)
{
    return upgrade_type::NONE;
}

// Gets the update type for optional values.
template<class T>
upgrade_type
get_upgrade_type(optional<T> const&, std::vector<std::type_index> parsed_types)
{
    return get_upgrade_type(T(), parsed_types);
}

// Gets the update type for optional values.
template<class T>
upgrade_type
get_upgrade_type(object_reference<T> const&, std::vector<std::type_index> parsed_types)
{
    // object_references are not upgraded. They will be upgraded when you attempt to get
    // the data associated with the object_reference id
    return upgrade_type::NONE;
}

// Gets the update type for values stored in a vector.
template<class T>
upgrade_type
get_upgrade_type(std::vector<T> const&, std::vector<std::type_index> parsed_types)
{
    return get_upgrade_type(T(), parsed_types);
}

// Gets the update type for values stored in a map
template<class Key, class Value>
upgrade_type
get_upgrade_type(std::map<Key, Value> const&, std::vector<std::type_index> parsed_types)
{
    return merged_upgrade_type(
        get_upgrade_type(Key(), parsed_types),
        get_upgrade_type(Value(), parsed_types));
}

// Gets the update type for values stored in an array.
template<class T>
upgrade_type
get_upgrade_type(array<T> const&, std::vector<std::type_index> parsed_types)
{
    return get_upgrade_type(T(), parsed_types);
}

// Checks if value_map has field and calls upgrade value if it does.
template<class T>
void
upgrade_field(T *x, value_map const& r, string const& field)
{
    auto i = r.find(value(field));
    if (i != r.end())
    {
        upgrade_value(x, i->second);
    }
}

// Creates a value_map and attempt and attempts to upgrade fields.
template<class T>
void
upgrade_field(T *x, value const& v, string const& field)
{
    auto const &r = cradle::cast<cradle::value_map>(v);
    upgrade_field(x, r, field);
}

// Handles upgrading values that need to be put in a container.
template<class T>
T auto_upgrade_value_for_container(value const& v)
{
    T x;
    upgrade_value(&x, v);
    return x;
}

// Handles updating values stored in a vector.
template<class T>
void
auto_upgrade_value(std::vector<T> *x, value const& v)
{
    cradle::value_list const& l = cradle::cast<cradle::value_list>(v);

    for (auto const& i : l)
    {
        x->push_back(auto_upgrade_value_for_container<T>(i));
    }
}

// Handles updating values stored in an array.
template<class T>
void
auto_upgrade_value(array<T> *x, value const& v)
{
    // TODO: figure out how to upgrade array values, you don't know size that previous
    // structure was because it could have changed (props added or removed)

    // only allow regular types in arrays
    from_value(x, v);
}

// Handles updating values stored in a map.
template<class Key, class Value>
void
auto_upgrade_value(std::map<Key,Value> *x, value const& v)
{
    cradle::value_map const& l = cradle::cast<cradle::value_map>(v);
    for (auto const& i : l)
    {
        x->insert(std::pair<Key, Value>(
            auto_upgrade_value_for_container<Key>(i.first),
            auto_upgrade_value_for_container<Value>(i.second)));
    }
}

// Handles updating values for object_reference types.
// Object_reference types do not have upgrades so its just calling from_value.
template<class T>
void
auto_upgrade_value(object_reference<T>* x, value const& v)
{
    from_value(x, v);
}

// Handles updating values for basic types.
// Basic types do not have upgrades so its just calling from_value.
template<class T>
void
auto_upgrade_value(T* x, value const& v)
{
    from_value(x, v);
}

// Generic upgrade_value function.
template<class T>
void
upgrade_value(T *x, value const& v)
{
    auto_upgrade_value(x, v);
}

// REQUESTS

api(enum internal)
enum class request_type
{
    // General requests that can be done locally or remotely...

    // a plain value wrapped in a request
    IMMEDIATE,
    // request for a function to be applied to some inputs
    FUNCTION,
    // an array of requests that should be evaluated
    // The result is an array containing the results of the requests in the
    // array.
    ARRAY,
    // a request to construct a structure from a map of requests representing
    // its fields
    STRUCTURE,
    // a request for a field within the result of another request
    PROPERTY,
    // a request to construct a union type from one of its member values
    UNION,
    // a request to wrap another request's result as an optional result
    SOME,
    // This wraps a request that produces an optional value, checks that it
    // has a value, and yields that value
    REQUIRED,
    // This wraps a request so that the request is resolved on its own.
    // When resolved as part of a larger request, this will be preresolved and
    // then reinserted as either an IMMEDIATE (for local requests) or an
    // IMMUTABLE (for remote requests).
    ISOLATED,

    // Requests that must be done remotely...

    // request for a calculation to be done remotely in Thinknode
    // (This can be wrapped around the above requests to specify that they
    // should be done remotely.)
    REMOTE_CALCULATION,
    // a Thinknode meta request
    META,
    // request for object (ISS) data from Thinknode
    // (the associated content is the data's ID)
    OBJECT,
    // request for immutable data from Thinknode
    // (the associated content is the data's ID)
    IMMUTABLE
};

struct untyped_request
{
    request_type type;

    // If type is IMMEDIATE, this is an untyped_immutable.
    // If type is FUNCTION, this is a function_request_info.
    // If type is ARRAY, this is a std::vector<untyped_request>.
    // If type is STRUCTURE, this is a structure_request_info.
    // If type is FIELD, this is a field_request_info.
    // If type is UNION, this is a union_request_info.
    // If type is SOME, this is a some_request_info.
    // If type is REQUIRED, this is a required_request_info.
    // If type is ISOLATED, this is an untyped_request.
    // If type is REMOTE_CALCULATION, this is an untyped_request.
    // If type is META, this is an untyped_request (the generator).
    // If type is OBJECT, this is the object ID of the data (as a string).
    // If type is IMMUTABLE, this is the immutable ID of the data (as a string).
    any_by_ref contents;

    // dynamic interface to working with result values
    dynamic_type_interface const* result_interface;

    // a precomputed hash of the request
    size_t hash;
};

// semi-standard interface for requests
// (Requests aren't considered regular CRADLE types, but they still support
// some of the regular C++ type interface so they can be used, for example,
// in unordered_maps.)

bool operator==(untyped_request const& a, untyped_request const& b);
bool static inline
operator!=(untyped_request const& a, untyped_request const& b)
{ return !(a == b); }

} namespace std {
    template<>
    struct hash<cradle::untyped_request>
    {
        size_t operator()(cradle::untyped_request const& request) const
        {
            return request.hash;
        }
    };
} namespace cradle {

template<class Result>
struct request
{
    typedef Result result_type;
    untyped_request untyped;
};

template<class Result>
bool operator==(request<Result> const& a, request<Result> const& b)
{ return a.untyped == b.untyped; }
template<class Result>
bool operator!=(request<Result> const& a, request<Result> const& b)
{ return !(a == b); }

// general utilities for constructing requests

untyped_request
make_untyped_request(request_type type, any_by_ref const& contents,
    dynamic_type_interface const* result_interface);

template<class Contents>
untyped_request
make_untyped_request(request_type type, Contents const& contents,
    dynamic_type_interface const* result_interface)
{
    return make_untyped_request(type, any_by_ref(contents), result_interface);
}

template<class Contents>
untyped_request
replace_request_contents(
    untyped_request const& request,
    Contents const& new_contents)
{
    return
        make_untyped_request(request.type, new_contents,
            request.result_interface);
}

template<class Result, class Contents>
request<Result>
make_typed_request(request_type type, Contents const& contents)
{
    request<Result> typed;
    typed.untyped =
        make_untyped_request(type, contents,
            dynamic_type_interface_maker<Result>::invoke());
    return typed;
}

// utilities for STRUCTURE requests

struct structure_constructor_interface
{
    virtual untyped_immutable
    construct(std::map<string,untyped_immutable> const& fields) const = 0;
};

struct structure_request_info
{
    std::map<string,untyped_request> fields;
    structure_constructor_interface const* constructor;

    structure_request_info() {}
    structure_request_info(
        std::map<string,untyped_request> const& fields,
        structure_constructor_interface const* constructor)
      : fields(fields)
      , constructor(constructor)
    {}
};

template<class Structure>
struct structure_constructor : structure_constructor_interface
{
    untyped_immutable
    construct(std::map<string,untyped_immutable> const& fields) const
    {
        Structure structure;
        read_fields_from_immutable_map(structure, fields);
        return erase_type(make_immutable(structure));
    }
};

template<class Structure>
structure_constructor_interface const*
get_structure_constructor()
{
    static structure_constructor<Structure> the_constructor;
    return &the_constructor;
}

// utilities for PROPERTY requests

struct field_extractor_interface
{
    virtual untyped_immutable
    extract(untyped_immutable const& record) const = 0;
};

// info associated with a PROPERTY request
struct property_request_info
{
    untyped_request record;
    string field;
    field_extractor_interface const* extractor;

    property_request_info() {}
    property_request_info(
        untyped_request const& record,
        string const& field,
        field_extractor_interface const* extractor)
      : record(record)
      , field(field)
      , extractor(extractor)
    {}
};

template<class Record, class Result, Result Record::*Field>
struct field_extractor : field_extractor_interface
{
    untyped_immutable
    extract(untyped_immutable const& record) const
    {
        Record const* record_value;
        cast_immutable_value(&record_value, get_value_pointer(record));
        return erase_type(make_immutable(record_value->*Field));
    }
};

template<class Record, class Result, Result Record::*Field>
field_extractor_interface const*
get_field_extractor()
{
    static field_extractor<Record,Result,Field> the_extractor;
    return &the_extractor;
}

// utilities for UNION requests

struct union_constructor_interface
{
    virtual untyped_immutable
    construct(untyped_immutable const& member) const = 0;
};

// info associated with a UNION request
struct union_request_info
{
    untyped_request member_request;
    string member_name;
    alia__shared_ptr<union_constructor_interface> constructor;

    union_request_info() {}
    union_request_info(
        untyped_request const& member_request,
        string const& member_name,
        alia__shared_ptr<union_constructor_interface> const& constructor)
      : member_request(member_request)
      , member_name(member_name)
      , constructor(constructor)
    {}
};

template<class Union, class Member>
struct union_constructor : union_constructor_interface
{
    typedef Union(*constructor_type)(Member const&);

    constructor_type constructor;

    union_constructor(constructor_type constructor)
      : constructor(constructor)
    {}

    untyped_immutable
    construct(untyped_immutable const& member) const
    {
        Member const* member_value;
        cast_immutable_value(&member_value, get_value_pointer(member));
        return erase_type(make_immutable(constructor(*member_value)));
    }
};

// utilities for SOME requests

struct optional_wrapper_interface
{
    virtual untyped_immutable
    wrap(untyped_immutable const& value) const = 0;
};

struct some_request_info
{
    untyped_request value;
    optional_wrapper_interface const* wrapper;

    some_request_info() {}
    some_request_info(
        untyped_request const& value,
        optional_wrapper_interface const* wrapper)
      : value(value)
      , wrapper(wrapper)
    {}
};

template<class Value>
struct optional_wrapper : optional_wrapper_interface
{
    untyped_immutable
    wrap(untyped_immutable const& value) const
    {
        Value const* typed_value;
        cast_immutable_value(&typed_value, get_value_pointer(value));
        return erase_type(make_immutable(some(*typed_value)));
    }
};

template<class Value>
optional_wrapper_interface const*
get_optional_wrapper()
{
    static optional_wrapper<Value> the_wrapper;
    return &the_wrapper;
}

// utilities for REQUIRED requests

struct optional_unwrapper_interface
{
    virtual untyped_immutable
    unwrap(untyped_immutable const& value) const = 0;
};

struct required_request_info
{
    untyped_request optional_value;
    optional_unwrapper_interface const* unwrapper;

    required_request_info() {}
    required_request_info(
        untyped_request const& optional_value,
        optional_unwrapper_interface const* unwrapper)
      : optional_value(optional_value)
      , unwrapper(unwrapper)
    {}
};

template<class Value>
struct optional_unwrapper : optional_unwrapper_interface
{
    untyped_immutable
    unwrap(untyped_immutable const& value) const
    {
        optional<Value> const* typed_value;
        cast_immutable_value(&typed_value, get_value_pointer(value));
        if (!*typed_value)
            throw exception("missing optional value");
        return erase_type(make_immutable(get(*typed_value)));
    }
};

template<class Value>
optional_unwrapper_interface const*
get_optional_unwrapper()
{
    static optional_unwrapper<Value> the_unwrapper;
    return &the_unwrapper;
}

// utilities for function requests

struct api_function_interface;

struct function_request_info
{
    api_function_interface const* function;
    std::vector<untyped_request> args;
    // If this is set, the resolution system will execute the request in the
    // foreground, even if that's not the default behavior for the function.
    bool force_foreground_resolution;
    function_request_info() : force_foreground_resolution(false) {}
};

// untyped version of rq_foreground, below.
untyped_request
force_foreground_resolution(untyped_request const& r);

// Given a local function request, this will yield an identical request that
// is guaranteed to execute in the foreground.
// (This is NOT recursive.)
template<class Value>
request<Value>
rq_foreground(request<Value> const& r)
{
    request<Value> result;
    result.untyped = force_foreground_resolution(r.untyped);
    return result;
}

// utilities for remote calculation requests

// Given a request that can be done locally, this will yield the equivalent
// remote request.
template<class Value>
request<Value>
rq_remote(request<Value> const& local)
{
    return make_typed_request<Value>(request_type::REMOTE_CALCULATION,
        local.untyped);
}

// Manually set the priority level for a calculation
template<class Value>
request<Value>
rq_level(request<Value> const& local, int level)
{
    if (local.untyped.type != request_type::FUNCTION)
    {
        throw
            exception(
                "rq_level requires a request_type::function. Got: " +
                 to_string(local.untyped.type));
    }
    auto f = as_function(local.untyped);
    f.level = level;
    return make_typed_request<Value>(request_type::FUNCTION, local.untyped);
}

// Need to forward declare this here to use for rq_meta.
struct calculation_request;

// Make a META request.
// Value is the type of the final result (i.e., generator is a request for
// a calculation_request which yields a Value).
template<class Value>
request<Value>
rq_meta(request<calculation_request> const& generator)
{
    return make_typed_request<Value>(request_type::META, generator.untyped);
}

// other miscellaneous request constructors and utilities...

// Make a request representing an immediate value.
template<class Value>
request<Value>
rq_value(Value const& value)
{
    return make_typed_request<Value>(request_type::IMMEDIATE,
        erase_type(make_immutable(value)));
}

// Make a request for retrieving object data.
template<class Data>
request<Data>
rq_object(object_reference<Data> const& reference)
{
    return make_typed_request<Data>(request_type::OBJECT, reference.uid);
}

// Make a request for retrieving immutable data.
template<class Data>
request<Data>
rq_immutable(immutable_reference<Data> const& reference)
{
    return make_typed_request<Data>(request_type::IMMUTABLE, reference.uid);
}

// Make an ARRAY request.
template<class Value>
request<std::vector<Value> >
rq_array(std::vector<request<Value> > const& item_requests)
{
    std::vector<untyped_request>
        untyped_item_requests =
            map([](request<Value> const& i) { return i.untyped; },
                item_requests);
    return
        make_typed_request<std::vector<Value> >(
            request_type::ARRAY, untyped_item_requests);
}

// Make a SOME request.
template<class Value>
request<optional<Value> >
rq_some(request<Value> const& value_request)
{
    return
        make_typed_request<optional<Value> >(
            request_type::SOME,
            some_request_info(
                value_request.untyped,
                get_optional_wrapper<Value>()));
}

// Make a REQUIRED request.
template<class Value>
request<Value>
rq_required(request<optional<Value> > const& optional_request)
{
    return
        make_typed_request<Value>(
            request_type::REQUIRED,
            required_request_info(
                optional_request.untyped,
                get_optional_unwrapper<Value>()));
}

// Make a STRUCTURE request.
template<class Value>
request<Value>
rq_structure(std::map<string,untyped_request> const& field_requests)
{
    return
        make_typed_request<Value>(
            request_type::STRUCTURE,
            structure_request_info(
                field_requests,
                cradle::get_structure_constructor<Value>()));
}

// Make a PROPERTY request.
#define rq_property(record, field) \
    make_typed_request< \
        decltype(typename remove_const_reference<decltype(record)>::type:: \
            result_type().field)>( \
        request_type::PROPERTY, \
        property_request_info( \
            record.untyped, \
            #field, \
            cradle::get_field_extractor< \
                typename remove_const_reference<decltype(record)>::type:: \
                    result_type, \
                decltype(typename remove_const_reference<decltype(record)>:: \
                    type::result_type().field), \
                &remove_const_reference<decltype(record)>::type:: \
                    result_type::field>()))

// Make a UNION request.
#define rq_union(union_namespace, union_type, member_name, member_request) \
    make_typed_request<union_namespace::union_type>( \
        request_type::UNION, \
        union_request_info( \
            member_request.untyped, \
            #member_name, \
            alia__shared_ptr<union_constructor_interface>( \
                new cradle::union_constructor< \
                    union_namespace::union_type, \
                    typename remove_const_reference< \
                        decltype(member_request)>::type::result_type>( \
                    union_namespace::make_##union_type##_with_##member_name))))

// Make an ISOLATED request.
template<class Value>
request<Value>
rq_isolated(request<Value> const& wrapped)
{
    // Prevent doubly-isolated requests, as this causes problems in the
    // request resolution system.
    return
        wrapped.untyped.type == request_type::ISOLATED
      ? wrapped
      : make_typed_request<Value>(
            request_type::ISOLATED, wrapped.untyped);
}

// invert_optional_request converts an optional<request<Value> > to a
// request<optional<Value> >
template<class Value>
request<optional<Value> >
invert_optional_request(optional<request<Value> > const& rq)
{
    if (rq)
        return rq_some(get(rq));
    else
        return rq_value(optional<Value>());
}

// REQUEST EXTRACTORS - These extract the contents of each type of request.

static inline untyped_immutable const&
as_immediate(untyped_request const& r)
{
    assert(r.type == request_type::IMMEDIATE);
    return unsafe_any_cast<untyped_immutable>(r.contents);
}

static inline function_request_info const&
as_function(untyped_request const& r)
{
    assert(r.type == request_type::FUNCTION);
    return unsafe_any_cast<function_request_info>(r.contents);
}

static inline untyped_request const&
as_remote_calc(untyped_request const& r)
{
    assert(r.type == request_type::REMOTE_CALCULATION);
    return unsafe_any_cast<untyped_request>(r.contents);
}

static inline string const&
as_immutable(untyped_request const& r)
{
    assert(r.type == request_type::IMMUTABLE);
    return unsafe_any_cast<string>(r.contents);
}

static inline string const&
as_object(untyped_request const& r)
{
    assert(r.type == request_type::OBJECT);
    return unsafe_any_cast<string>(r.contents);
}

static inline std::vector<untyped_request> const&
as_array(untyped_request const& r)
{
    assert(r.type == request_type::ARRAY);
    return unsafe_any_cast<std::vector<untyped_request> >(r.contents);
}

static inline structure_request_info const&
as_structure(untyped_request const& r)
{
    assert(r.type == request_type::STRUCTURE);
    return unsafe_any_cast<structure_request_info>(r.contents);
}

static inline property_request_info const&
as_property(untyped_request const& r)
{
    assert(r.type == request_type::PROPERTY);
    return unsafe_any_cast<property_request_info>(r.contents);
}

static inline union_request_info const&
as_union(untyped_request const& r)
{
    assert(r.type == request_type::UNION);
    return unsafe_any_cast<union_request_info>(r.contents);
}

static inline some_request_info const&
as_some(untyped_request const& r)
{
    assert(r.type == request_type::SOME);
    return unsafe_any_cast<some_request_info>(r.contents);
}

static inline required_request_info const&
as_required(untyped_request const& r)
{
    assert(r.type == request_type::REQUIRED);
    return unsafe_any_cast<required_request_info>(r.contents);
}

static inline untyped_request const&
as_isolated(untyped_request const& r)
{
    assert(r.type == request_type::ISOLATED);
    return unsafe_any_cast<untyped_request>(r.contents);
}

static inline untyped_request const&
as_meta(untyped_request const& r)
{
    assert(r.type == request_type::META);
    return unsafe_any_cast<untyped_request>(r.contents);
}

}

#endif
