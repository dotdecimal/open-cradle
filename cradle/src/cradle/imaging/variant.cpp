#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/contiguous.hpp>

namespace cradle {

string get_pixel_format_name(pixel_format fmt)
{
    switch (fmt)
    {
     case pixel_format::RGB:
        return "rgb";
     case pixel_format::RGBA:
        return "rgba";
     case pixel_format::GRAY:
        return "gray";
     default:
        throw exception("invalid pixel format");
    }
}

unsigned get_channel_count(pixel_format fmt)
{
    switch (fmt)
    {
     case pixel_format::RGB:
        return 3;
     case pixel_format::RGBA:
        return 4;
     case pixel_format::GRAY:
        return 1;
     default:
        throw exception("invalid pixel format");
    }
}

string get_channel_type_name(channel_type type)
{
    switch (type)
    {
     case channel_type::INT8:
        return "8-bit signed integer";
     case channel_type::UINT8:
        return "8-bit unsigned integer";
     case channel_type::INT16:
        return "16-bit signed integer";
     case channel_type::UINT16:
        return "16-bit unsigned integer";
     case channel_type::INT32:
        return "32-bit signed integer";
     case channel_type::UINT32:
        return "32-bit unsigned integer";
     case channel_type::INT64:
        return "64-bit signed integer";
     case channel_type::UINT64:
        return "64-bit unsigned integer";
     case channel_type::FLOAT:
        return "32-bit float";
     case channel_type::DOUBLE:
        return "64-bit float";
     default:
        throw exception("invalid channel type");
    }
}

size_t get_channel_size(channel_type type)
{
    switch (type)
    {
     case channel_type::INT8:
        return 1;
     case channel_type::UINT8:
        return 1;
     case channel_type::INT16:
        return 2;
     case channel_type::UINT16:
        return 2;
     case channel_type::INT32:
        return 4;
     case channel_type::UINT32:
        return 4;
     case channel_type::INT64:
        return 8;
     case channel_type::UINT64:
        return 8;
     case channel_type::FLOAT:
        return 4;
     case channel_type::DOUBLE:
        return 8;
     default:
        throw exception("invalid channel type");
    }
}

struct pixel_equality_comparison
{
    bool equal;
    pixel_equality_comparison() : equal(true) {}
    template<class T>
    void operator()(T const& a, T const& b)
    {
        if (a != b)
            equal = false;
    }
};
template<unsigned N>
struct image_equality_comparison
{
    bool equal;
    image<N,variant,shared> const* variant_b;
    template<class Pixel>
    void operator()(image<N,Pixel,const_view> const& a)
    {
        image<N,Pixel,const_view> b =
            cast_variant<Pixel>(as_const_view(*variant_b));
        pixel_equality_comparison comparison;
        foreach_pixel2(a, b, comparison);
        equal = comparison.equal;
    }
};

template<unsigned N, class Pixel>
bool variant_images_equal(
    image<N,Pixel,shared> const& a, image<N,Pixel,shared> const& b)
{
    if (a.pixels.type_info != b.pixels.type_info ||
        a.size != b.size || a.origin != b.origin || a.axes != b.axes ||
        a.value_mapping != b.value_mapping || a.units != b.units)
    {
        return false;
    }
    image_equality_comparison<N> comparison;
    comparison.variant_b = &b;
    apply_fn_to_variant(comparison, as_const_view(a));
    return comparison.equal;
}

struct pixel_less_than_comparison
{
    int state;
    pixel_less_than_comparison() : state(0) {}
    template<class T>
    void operator()(T const& a, T const& b)
    {
        if (state == 0)
        {
            if (a < b)
                state = -1;
            else if (b < a)
                state = 1;
        }
    }
};
template<unsigned N>
struct image_less_than_comparison
{
    bool less_than;
    image<N,variant,shared> const* variant_b;
    template<class Pixel>
    void operator()(image<N,Pixel,const_view> const& a)
    {
        image<N,Pixel,const_view> b =
            cast_variant<Pixel>(as_const_view(*variant_b));
        pixel_less_than_comparison comparison;
        foreach_pixel2(a, b, comparison);
        less_than = comparison.state < 0;
    }
};
template<unsigned N>
bool variant_less_than(
    image<N,variant,shared> const& a, image<N,variant,shared> const& b)
{
    if (a.pixels.type_info < b.pixels.type_info)
        return true;
    if (b.pixels.type_info < a.pixels.type_info)
        return false;
    if (a.size < b.size)
        return true;
    if (b.size < a.size)
        return false;
    if (a.origin < b.origin)
        return true;
    if (b.origin < a.origin)
        return false;
    for (unsigned i = 0; i != N; ++i)
    {
        if (a.axes[i] < b.axes[i])
            return true;
        if (b.axes[i] < a.axes[i])
            return false;
    }
    if (a.value_mapping < b.value_mapping)
        return true;
    if (b.value_mapping < a.value_mapping)
        return false;
    if (a.units < b.units)
        return true;
    if (b.units < a.units)
        return false;

    image_less_than_comparison<N> comparison;
    comparison.variant_b = &b;
    apply_fn_to_variant(comparison, as_const_view(a));
    return comparison.less_than;
}

template<unsigned N>
void variant_from_value(image<N,variant,shared>* x, value const& v)
{
    value_map const& r = cast<value_map>(v);
    from_value(&x->pixels.type_info, get_field(r, "type_info"));
    from_value(&x->size, get_field(r, "size"));
    x->step = get_contiguous_steps(x->size);
    from_value(&x->origin, get_field(r, "origin"));
    from_value(&x->axes, get_field(r, "axes"));
    from_value(&x->value_mapping, get_field(r, "value_mapping"));
    from_value(&x->units, get_field(r, "units"));
    value pixels = get_field(r, "pixels");
    blob const& b = cast<blob>(pixels);
    size_t expected_size =
        product(x->size) * get_channel_size(x->pixels.type_info.type) *
        get_channel_count(x->pixels.type_info.format);
    check_array_size(expected_size, b.size);
    x->pixels.ownership = b.ownership;
    x->pixels.view = b.data;
}

template<unsigned N>
void variant_to_value(value* v, image<N,variant,shared> const& y)
{
    image<N,variant,shared> x = get_contiguous_version(y);
    value_map r;
    to_value(&r[value("type_info")], x.pixels.type_info);
    to_value(&r[value("size")], x.size);
    to_value(&r[value("origin")], x.origin);
    to_value(&r[value("axes")], x.axes);
    to_value(&r[value("value_mapping")], x.value_mapping);
    to_value(&r[value("units")], x.units);
    blob b;
    b.ownership = x.pixels.ownership;
    b.data = x.pixels.view;
    b.size =
        product(x.size) * get_channel_size(x.pixels.type_info.type) *
        get_channel_count(x.pixels.type_info.format);
    set(r[value("pixels")], b);
    set(*v, r);
}

template<unsigned N>
void read_variant_fields_from_immutable_map(
    image<N,variant,shared>* x,
    std::map<string,untyped_immutable> const& r)
{
    from_immutable(&x->pixels.type_info, get_field(r, "type_info"));
    from_immutable(&x->size, get_field(r, "size"));
    x->step = get_contiguous_steps(x->size);
    from_immutable(&x->origin, get_field(r, "origin"));
    from_immutable(&x->axes, get_field(r, "axes"));
    from_immutable(&x->value_mapping, get_field(r, "value_mapping"));
    from_immutable(&x->units, get_field(r, "units"));
    blob b;
    from_immutable(&b, get_field(r, "pixels"));
    size_t expected_size =
        product(x->size) * get_channel_size(x->pixels.type_info.type) *
        get_channel_count(x->pixels.type_info.format);
    check_array_size(expected_size, b.size);
    x->pixels.ownership = b.ownership;
    x->pixels.view = b.data;
}

template<unsigned N>
size_t deep_sizeof_variant(image<N,variant,shared> const& x)
{
    return sizeof(x) +
        product(x.size) * get_channel_size(x.pixels.type_info.type) *
        get_channel_count(x.pixels.type_info.format);
}

template<unsigned N>
raw_type_info
get_variant_type_info(image<N,variant,shared> const& x)
{
    static string name = "image" + cradle::to_string(N);
    static string description = cradle::to_string(N) + "D image";
    std::vector<cradle::raw_structure_field_info> fields;
    fields.push_back(cradle::raw_structure_field_info(
        "type_info", "the type of the image pixels",
        get_type_info(x.pixels.type_info)));
    fields.push_back(cradle::raw_structure_field_info(
        "size", "the size (in pixels) of the image",
        get_type_info(x.size)));
    fields.push_back(cradle::raw_structure_field_info(
        "origin", "the location in space of the outside corner of the first pixel",
        get_type_info(x.origin)));
    fields.push_back(cradle::raw_structure_field_info(
        "axes", "vectors describing the orientation of the image axes in space - Each vector is one pixel long.", get_type_info(x.axes)));
    fields.push_back(cradle::raw_structure_field_info(
        "value_mapping", "a linear function mapping raw pixel values to image values",
        get_type_info(x.value_mapping)));
    fields.push_back(cradle::raw_structure_field_info(
        "units", "the units of the image values", get_type_info(x.units)));
    fields.push_back(cradle::raw_structure_field_info(
        "pixels", "the array of raw pixel values (d)",
            cradle::raw_type_info(cradle::raw_kind::SIMPLE,
                cradle::any(cradle::raw_simple_type::BLOB))));
    return cradle::raw_type_info(cradle::raw_kind::STRUCTURE,
        cradle::any(cradle::raw_structure_info(name, description, fields)));
}

template<unsigned N>
size_t
hash_variant_image(image<N,variant,shared> const& x)
{
    // TODO: hash for real?
    return 0;
}

#define CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(T) \
    bool operator==(T const& a, T const& b) \
    { return variant_images_equal(a, b); } \
    bool operator!=(T const& a, T const& b) \
    { return !(a == b); } \
    bool operator<(T const& a, T const& b) \
    { return variant_less_than(a, b); } \
    void to_value(value* v, T const& x) \
    { return variant_to_value(v, x); } \
    void from_value(T* x, value const& v) \
    { return variant_from_value(x, v); } \
    void read_fields_from_immutable_map(T& x, \
        std::map<string,untyped_immutable> const& v) \
    { return read_variant_fields_from_immutable_map(&x, v); } \
    size_t deep_sizeof(T const& x) \
    { return deep_sizeof_variant(x); } \
    std::ostream& operator<<(std::ostream& s, T const& x) \
    { value v; variant_to_value(&v, x); return s << v; } \
    raw_type_info get_proper_type_info(T const& x) \
    { return get_variant_type_info(x); } \
    } namespace std { \
    size_t hash<T>::operator()(T const& x) const \
    { return cradle::hash_variant_image(x); } \
    } namespace cradle {

CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(cradle::image1)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(cradle::image2)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(cradle::image3)

}
