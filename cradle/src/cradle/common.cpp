#include <cradle/common.hpp>

#include <cctype>
#include <cstring>

#include <boost/format.hpp>

#include <cradle/api.hpp>
#include <cradle/date_time.hpp>
#include <cradle/encoding.hpp>
#include <cradle/io/generic_io.hpp>

namespace cradle {

// EXCEPTIONS

type_mismatch::type_mismatch(value_type expected, value_type got)
  : exception(str(boost::format("type mismatch\n    "
        "expected: %s\n    got: %s") % expected % got))
  , expected_(expected)
  , got_(got)
{
}

array_size_error::array_size_error(size_t expected_size, size_t actual_size)
  : cradle::exception(
        "incorrect array size\nexpected size: " + to_string(expected_size) +
        "\nactual size: " + to_string(actual_size))
  , expected_size(expected_size), actual_size(actual_size)
{
}

void check_array_size(size_t expected_size, size_t actual_size)
{
    if (expected_size != actual_size)
        throw cradle::array_size_error(expected_size, actual_size);
}

void check_index_bounds(char const* label, size_t index, size_t upper_bound)
{
    if (index >= upper_bound)
            throw index_out_of_bounds(label, index, upper_bound);
}

// MAPS

value get_field(value_map const& r, string const& field)
{
    value v;
    if (!get_field(&v, r, field))
        throw cradle::exception("missing field: " + field);
    return v;
}

bool get_field(value* v, value_map const& r, string const& field)
{
    auto i = r.find(value(field));
    if (i == r.end())
        return false;
    *v = i->second;
    return true;
}

value const& get_union_value_type(value_map const& map)
{
    if (map.size() != 1)
        throw cradle::exception("unions must have exactly one field");
    return map.begin()->first;
}

// VALUES

std::ostream& operator<<(std::ostream& s, value_type t)
{
    switch (t)
    {
     case value_type::NIL:
        s << "nil";
        break;
     case value_type::BOOLEAN:
        s << "boolean";
        break;
     case value_type::INTEGER:
        s << "integer";
        break;
     case value_type::FLOAT:
        s << "float";
        break;
     case value_type::STRING:
        s << "string";
        break;
     case value_type::BLOB:
        s << "blob";
        break;
     case value_type::DATETIME:
        s << "datetime";
        break;
     case value_type::LIST:
        s << "list";
        break;
     case value_type::MAP:
        s << "map";
        break;
    }
    return s;
}
std::istream& operator>>(std::istream& s, value_type& t)
{
    string type_string;
    while (1)
    {
        int c = s.peek();
        if (!std::isalpha(c) && c != '_')
            break;
        type_string.push_back(s.get());
    }
    if (type_string == "nil")
        t = value_type::NIL;
    else if (type_string == "boolean")
        t = value_type::BOOLEAN;
    else if (type_string == "integer")
        t = value_type::INTEGER;
    else if (type_string == "float")
        t = value_type::FLOAT;
    else if (type_string == "string")
        t = value_type::STRING;
    else if (type_string == "blob")
        t = value_type::BLOB;
    else if (type_string == "datetime")
        t = value_type::DATETIME;
    else if (type_string == "list")
        t = value_type::LIST;
    else if (type_string == "map")
        t = value_type::MAP;
    else
        s.setstate(std::ios::failbit);
    return s;
}

static void write_escaped_string(std::ostream& s, string const& x)
{
    s << '\"';
    for (auto c : x)
    {
        switch (c)
        {
         case '\\':
            s << "\\\\";
            break;
         case '\"':
            s << "\\\"";
            break;
         default:
            s << c;
        }
    }
    s << '\"';
}

struct value_ostream_operator
{
    value_ostream_operator(std::ostream& s) : s(s) {}
    void operator()(nil_type _)
    {
        s << "()";
    }
    void operator()(bool v)
    {
        s << (v ? "true" : "false");
    }
    void operator()(integer v)
    {
        s << v;
    }
    void operator()(double v)
    {
        s << v;
    }
    void operator()(string const& v)
    {
        write_escaped_string(s, v);
    }
    void operator()(blob const& v)
    {
        s << "(blob: " << v.size << " bytes)";
    }
    void operator()(boost::posix_time::ptime const& v)
    {
        s << v;
    }
    void operator()(value_list const& v)
    {
        s << "[";
        size_t n_elements = v.size();
        for (std::size_t i = 0; i != n_elements; ++i)
        {
            if (i != 0)
                s << ", ";
            s << v[i];
        }
        s << "]";
    }
    void operator()(value_map const& v)
    {
        s << "{";
        for (auto i = v.begin(); i != v.end(); ++i)
        {
            if (i != v.begin())
                s << ", ";
            s << i->first;
            s << ": ";
            s << i->second;
        }
        s << "}";
    }
    std::ostream& s;
};

std::ostream& operator<<(std::ostream& s, value const& v)
{
    value_ostream_operator fn(s);
    apply_fn_to_value(fn, v);
    return s;
}

void value::swap_with(value& other)
{
    using cradle::swap;
    swap(type_, other.type_);
    swap(value_, other.value_);
}

static void check_type(value_type got, value_type expected)
{
    if (got != expected)
        throw type_mismatch(expected, got);
}

void value::get(bool const** v) const
{
    check_type(type_, value_type::BOOLEAN);
    *v = &unsafe_any_cast<bool>(value_);
}
void value::get(integer const** v) const
{
    check_type(type_, value_type::INTEGER);
    *v = &unsafe_any_cast<integer>(value_);
}
void value::get(double const** v) const
{
    check_type(type_, value_type::FLOAT);
    *v = &unsafe_any_cast<double>(value_);
}
void value::get(string const** v) const
{
    check_type(type_, value_type::STRING);
    *v = &unsafe_any_cast<string>(value_);
}
void value::get(blob const** v) const
{
    check_type(type_, value_type::BLOB);
    *v = &unsafe_any_cast<blob>(value_);
}
void value::get(boost::posix_time::ptime const** v) const
{
    check_type(type_, value_type::DATETIME);
    *v = &unsafe_any_cast<boost::posix_time::ptime>(value_);
}
void value::get(value_list const** v) const
{
    // Certain ways of encoding values (e.g., JSON) have the same
    // representation for empty arrays and empty maps, so if we encounter an
    // empty map here, we should treat it as an empty list.
    if (type_ == value_type::MAP && unsafe_any_cast<value_map>(value_).empty())
    {
        static const value_list empty_list;
        *v = &empty_list;
        return;
    }
    check_type(type_, value_type::LIST);
    *v = &unsafe_any_cast<value_list>(value_);
}
void value::get(value_map const** v) const
{
    // Same logic as in the list case.
    if (type_ == value_type::LIST &&
        unsafe_any_cast<value_list>(value_).empty())
    {
        static const value_map empty_map;
        *v = &empty_map;
        return;
    }
    check_type(type_, value_type::MAP);
    *v = &unsafe_any_cast<value_map>(value_);
}

void value::set(nil_type _)
{
    type_ = value_type::NIL;
}
void value::set(bool v)
{
    type_ = value_type::BOOLEAN;
    value_ = v;
}
void value::set(integer v)
{
    type_ = value_type::INTEGER;
    value_ = v;
}
void value::set(double v)
{
    type_ = value_type::FLOAT;
    value_ = v;
}
void value::set(string const& v)
{
    type_ = value_type::STRING;
    value_ = v;
}
void value::set(blob const& v)
{
    type_ = value_type::BLOB;
    value_ = v;
}
void value::set(boost::posix_time::ptime const& v)
{
    type_ = value_type::DATETIME;
    value_ = v;
}
void value::set(value_list const& v)
{
    type_ = value_type::LIST;
    value_ = v;
}
void value::set(value_map const& v)
{
    type_ = value_type::MAP;
    value_ = v;
}

void value::swap_in(value_list& v)
{
    this->set(v);
}
void value::swap_in(value_map& v)
{
    this->set(v);
}

struct value_deep_sizeof_operator
{
    size_t result;
    value_deep_sizeof_operator() : result(0) {}
    template<class Contents>
    void operator()(Contents const& contents)
    {
        result = sizeof(value) + deep_sizeof(contents);
    }
};

size_t deep_sizeof(value const& v)
{
    value_deep_sizeof_operator fn;
    apply_fn_to_value(fn, v);
    return fn.result;
}

struct hash_fn
{
    template<class T>
    void operator()(T const& a)
    {
        result = invoke_hash(a);
    }
    size_t result;
};

} namespace std {
    size_t hash<cradle::value>::operator()(cradle::value const& x) const
    {
        cradle::hash_fn fn;
        apply_fn_to_value(fn, x);
        return fn.result;
    }
} namespace cradle {

// COMPARISON OPERATORS

struct equality_test
{
    template<class T>
    void operator()(T const& a, T const& b)
    {
        result = a == b;
    }
    bool result;
};

bool operator==(value const& a, value const& b)
{
    if (a.type() != b.type())
        return false;
    equality_test fn;
    apply_fn_to_value_pair(fn, a, b);
    return fn.result;
}
bool operator!=(value const& a, value const& b)
{ return !(a == b); }

struct less_than_test
{
    template<class T>
    void operator()(T const& a, T const& b)
    {
        result = a < b;
    }
    bool result;
};

bool operator<(value const& a, value const& b)
{
    if (a.type() != b.type())
        return a.type() < b.type();
    less_than_test fn;
    apply_fn_to_value_pair(fn, a, b);
    return fn.result;
}
bool operator<=(value const& a, value const& b)
{ return !(b < a); }
bool operator>(value const& a, value const& b)
{ return b < a; }
bool operator>=(value const& a, value const& b)
{ return !(a < b); }

bool operator==(blob const& a, blob const& b)
{
    return a.size == b.size &&
        (a.data == b.data || std::memcmp(a.data, b.data, a.size) == 0);
}
bool operator<(blob const& a, blob const& b)
{
    return a.size < b.size ||
        (a.size == b.size &&
        (a.data != b.data && std::memcmp(a.data, b.data, a.size) < 0));
}

// BUILT-IN C TYPE INTERFACES

void to_value(value* v, bool x)
{
    set(*v, x);
}
void from_value(bool* x, value const& v)
{
    *x = cast<bool>(v);
}

void to_value(value* v, string const& x)
{
    set(*v, x);
}
void from_value(string* x, value const& v)
{
    // Strings are also used to encode datetimes in JSON, so it's possible we
    // might misinterpret a string as a datetime.
    if (v.type() == value_type::DATETIME)
        *x = to_value_string(cast<time>(v));
    else
        *x = cast<string>(v);
}

void to_value(value* v, blob const& x)
{
    set(*v, x);
}
void from_value(blob* x, value const& v)
{
    *x = cast<blob>(v);
}

#define CRADLE_DEFINE_INTEGER_INTERFACE(T) \
    void to_value(value* v, T x) \
    { set(*v, to_integer(x)); } \
    void from_value(T* x, value const& v) \
    { \
        /* Floats can also be acceptable. */ \
        if (v.type() == value_type::FLOAT) \
            from_float(x, (cast<double>(v))); \
        else \
            from_integer(x, (cast<integer>(v))); \
    }

#define CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(T) \
    integer to_integer(T x) \
    { return integer(x); } \
    static char const* type_name(T) \
    { \
        static string name = to_string(sizeof(T) * 8) + "-bit integer"; \
        return name.c_str(); \
    } \
    void from_float(T* x, double n) \
    { \
        T t = (T)(n); \
        if (double(t) != n) \
            throw exception("expected " + string(type_name(t))); \
        *x = t; \
    } \
    void from_integer(T* x, integer n) \
    { \
        T t = (T)(n); \
        if (integer(t) != n) \
            throw exception("expected " + string(type_name(t))); \
        *x = t; \
    } \
    raw_type_info get_type_info(T) \
    { return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::INTEGER)); } \
    CRADLE_DEFINE_INTEGER_INTERFACE(T)

CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(signed char)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(unsigned char)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(signed short)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(unsigned short)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(signed int)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(unsigned int)

#define CRADLE_DEFINE_LARGE_INTEGER_INTERFACE(T) \
    integer to_integer(T x) \
    { \
        integer n = integer(x); \
        if ((T)(n) != x) \
            throw exception("integer too large to store as double"); \
        return n; \
    } \
    static char const* type_name(T) \
    { \
        static string name = to_string(sizeof(T) * 8) + "-bit integer"; \
        return name.c_str(); \
    } \
    void from_float(T* x, double n) \
    { \
        T t = (T)(n); \
        if (double(t) != n) \
            throw exception("expected " + string(type_name(t))); \
        *x = t; \
    } \
    void from_integer(T* x, integer n) \
    { \
        T t = (T)(n); \
        if (integer(t) != n) \
            throw exception("expected " + string(type_name(t))); \
        *x = t; \
    } \
    raw_type_info get_type_info(T) \
    { return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::INTEGER)); } \
    CRADLE_DEFINE_INTEGER_INTERFACE(T)

CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(signed long)
CRADLE_DEFINE_SMALL_INTEGER_INTERFACE(unsigned long)
CRADLE_DEFINE_LARGE_INTEGER_INTERFACE(signed long long)
CRADLE_DEFINE_LARGE_INTEGER_INTERFACE(unsigned long long)

#define CRADLE_DEFINE_FLOAT_INTERFACE(T) \
    void to_value(value* v, T x) \
    { set(*v, double(x)); } \
    void from_value(T* x, value const& v) \
    { \
        /* Integers are also acceptable as floats. */ \
        if (v.type() == value_type::INTEGER) \
            *x = T(cast<integer>(v)); \
        else \
            *x = T(cast<double>(v)); \
    } \
    raw_type_info get_type_info(T) \
    { return raw_type_info(raw_kind::SIMPLE, any(raw_simple_type::FLOAT)); }

CRADLE_DEFINE_FLOAT_INTERFACE(double)
// TODO: Check for overflow/underflow on floats.
CRADLE_DEFINE_FLOAT_INTERFACE(float)

std::ostream& operator<<(std::ostream& s, resolved_function_uid const& id)
{
    value_map v;
    v[value("app")] = value(id.app);
    v[value("uid")] = value(id.uid);
    auto json = value_to_json(value(v));
    s <<
        base64_encode(reinterpret_cast<uint8_t const*>(json.c_str()),
            json.length(), get_mime_base64_character_set());
    return s;
}

// IMMUTABLES

immutable_data_type_mismatch::immutable_data_type_mismatch(
    raw_type_info const& expected, raw_type_info const& got)
  : exception("type mismatch\n"
        "expected type: " + to_string(make_api_type_info(expected)) +
        "actual type: " + to_string(make_api_type_info(got)))
  , expected_(new raw_type_info(expected))
  , got_(new raw_type_info(got))
{}

untyped_immutable
get_field(std::map<string,untyped_immutable> const& fields,
    string const& field_name)
{
    auto field = fields.find(field_name);
    if (field == fields.end())
    {
        throw exception("missing field: " + field_name);
    }
    return field->second;
}

// REQUESTS

// This is used to compute the hash that's stored within an untyped_request.
size_t static
hash_request(untyped_request const& request)
{
    size_t content_hash;
    switch (request.type)
    {
     case request_type::IMMEDIATE:
        content_hash = as_immediate(request).ptr->hash();
        break;
     case request_type::FUNCTION:
      {
        auto const& calc = as_function(request);
        content_hash = invoke_hash(calc.function->api_info.name);
        for (auto const& i : calc.args)
            content_hash = combine_hashes(invoke_hash(i), content_hash);
        break;
      }
     case request_type::REMOTE_CALCULATION:
        content_hash = invoke_hash(as_remote_calc(request));
        break;
     case request_type::META:
        content_hash = invoke_hash(as_meta(request));
        break;
     case request_type::IMMUTABLE:
        content_hash = invoke_hash(as_immutable(request));
        break;
     case request_type::OBJECT:
        content_hash = invoke_hash(as_object(request));
        break;
     case request_type::ARRAY:
        content_hash = 0;
        for (auto const& i : as_array(request))
            content_hash = combine_hashes(invoke_hash(i), content_hash);
        break;
     case request_type::STRUCTURE:
        content_hash = 0;
        for (auto const& i : as_structure(request).fields)
        {
            content_hash =
                combine_hashes(invoke_hash(i.second), content_hash);
        }
        break;
     case request_type::PROPERTY:
      {
        auto const& property = as_property(request);
        content_hash =
            combine_hashes(
                invoke_hash(property.record),
                invoke_hash(property.field));
        break;
      }
     case request_type::UNION:
      {
        content_hash =
            combine_hashes(
                invoke_hash(as_union(request).member_request),
                invoke_hash(as_union(request).member_name));
        break;
      }
     case request_type::SOME:
        content_hash = invoke_hash(as_some(request).value);
        break;
     case request_type::REQUIRED:
        content_hash = invoke_hash(as_required(request).optional_value);
        break;
     case request_type::ISOLATED:
        content_hash = invoke_hash(as_isolated(request));
        break;
     default:
        assert(0);
        content_hash = 0;
        break;
    }
    return combine_hashes(size_t(request.type), content_hash);
}

untyped_request
make_untyped_request(request_type type, any_by_ref const& contents,
    dynamic_type_interface const* result_interface)
{
    untyped_request request;
    request.type = type;
    request.contents = contents;
    request.result_interface = result_interface;
    request.hash = hash_request(request);
    return request;
}

untyped_request
force_foreground_resolution(untyped_request const& request)
{
    if(request.type != request_type::FUNCTION)
        throw exception("foreground request type must be a function");
    auto new_spec = as_function(request);
    new_spec.force_foreground_resolution = true;
    return replace_request_contents(request, new_spec);
}

bool operator==(untyped_request const& a, untyped_request const& b)
{
    // If the hashes are different, this is a very quick way to tell that the
    // requests are different.
    if (a.hash != b.hash)
        return false;

    if (a.type != b.type)
        return false;

    if (a.result_interface->cpp_typeid() != b.result_interface->cpp_typeid())
        return false;

    // Since request contents are copied by reference, checking to see if two
    // requests actually share the same contents is a quick way to confirm that
    // they're the same.
    if (get_value_pointer(a.contents) == get_value_pointer(b.contents))
        return true;

    switch (a.type)
    {
     case request_type::IMMEDIATE:
        return as_immediate(a).ptr->equals(as_immediate(b).ptr.get());
     case request_type::FUNCTION:
      {
        auto const& calc_a = as_function(a);
        auto const& calc_b = as_function(b);
        return calc_a.function == calc_b.function &&
            calc_a.args == calc_b.args;
      }
     case request_type::REMOTE_CALCULATION:
        return as_remote_calc(a) == as_remote_calc(b);
     case request_type::OBJECT:
        return as_object(a) == as_object(b);
     case request_type::IMMUTABLE:
        return as_immutable(a) == as_immutable(b);
     case request_type::ARRAY:
        return as_array(a) == as_array(b);
     case request_type::STRUCTURE:
      {
        auto const& structure_a = as_structure(a);
        auto const& structure_b = as_structure(b);
        return structure_a.constructor == structure_b.constructor &&
            structure_a.fields == structure_b.fields;
      }
     case request_type::PROPERTY:
      {
        auto const& property_a = as_property(a);
        auto const& property_b = as_property(b);
        return property_a.field == property_b.field &&
            property_a.record == property_b.record;
        break;
      }
     case request_type::UNION:
      {
        auto const& union_a = as_union(a);
        auto const& union_b = as_union(b);
        return union_a.member_request == union_b.member_request &&
            union_a.member_name == union_b.member_name;
        break;
      }
     case request_type::SOME:
        return as_some(a).value == as_some(b).value;
     case request_type::REQUIRED:
        return as_required(a).optional_value == as_required(b).optional_value;
     case request_type::ISOLATED:
        return as_isolated(a) == as_isolated(b);
     case request_type::META:
         return as_meta(a) == as_meta(b);
     default:
        assert(0);
        return false;
    }
}

}
