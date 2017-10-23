#include <cradle/imaging/image.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/api.hpp>

namespace cradle {

// All this has already been defined for variant images, and since we're only
// defining this for images that are convertible to variants, the easiest way
// is to simply convert them to variants and apply those functions.
// This is a little inefficient for the comparisons, but the performance of
// those is dominated by the actual comparison of the pixels anyway.
// Also, note that this method is the desired behavior for the conversion to
// and from dynamic values, since we want those to have the same dynamic
// representation as the variant form.
#define CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(N, T) \
    bool operator==(image<N,T,shared> const& a, image<N,T,shared> const& b) \
    { return as_variant(a) == as_variant(b); } \
    bool operator!=(image<N,T,shared> const& a, image<N,T,shared> const& b) \
    { return !(a == b); } \
    bool operator<(image<N,T,shared> const& a, image<N,T,shared> const& b) \
    { return as_variant(a) < as_variant(b); } \
    void to_value(value* v, image<N,T,shared> const& x) \
    { to_value(v, as_variant(x)); } \
    void from_value(image<N,T,shared>* x, value const& v) \
    { \
        image<N,variant,shared> tmp; \
        from_value(&tmp, v); \
        *x = cast_variant<T>(tmp); \
    } \
    void read_fields_from_immutable_map(image<N,T,shared>& x, \
        std::map<string,untyped_immutable> const& fields) \
    { \
        image<N,variant,shared> tmp; \
        read_fields_from_immutable_map(tmp, fields); \
        x = cast_variant<T>(tmp); \
    } \
    size_t deep_sizeof(image<N,T,shared> const& x) \
    { return deep_sizeof(as_variant(x)); } \
    std::ostream& operator<<(std::ostream& s, image<N,T,shared> const& x) \
    { return (s << as_variant(x)); } \
    raw_type_info get_proper_type_info(image<N,T,shared> const& x) \
    { return get_proper_type_info(as_variant(x)); } \
    } namespace std { \
    size_t hash<cradle::image<N,T,cradle::shared> >:: \
        operator()(cradle::image<N,T,cradle::shared> const& x) const \
    { return alia::invoke_hash(as_variant(x)); } \
    } namespace cradle {

#define CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(T) \
    CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(1,T) \
    CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(2,T) \
    CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE(3,T)

CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int8_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint8_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int16_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint16_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int32_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint32_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::int64_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::uint64_t)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(float)
CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(double)

CRADLE_DEFINE_REGULAR_IMAGE_INTERFACE_FOR_TYPE(cradle::rgba8)

void register_image_types(api_implementation& api)
{
    register_api_named_type(api, "image_1d", 0, "1D image",
        make_api_type_info(get_proper_type_info(image<1,double,shared>())));
    register_api_named_type(api, "image_2d", 0, "2D image",
        make_api_type_info(get_proper_type_info(image<2,double,shared>())));
    register_api_named_type(api, "image_3d", 0, "3D image",
        make_api_type_info(get_proper_type_info(image<3,double,shared>())));
    register_api_named_type(api, "rgb8", 0, "RGB triplet",
        make_api_type_info(get_proper_type_info(rgb8())));
    register_api_named_type(api, "rgba8", 0,
        "RGB triplet with alpha",
        make_api_type_info(get_proper_type_info(rgba8())));
}

}
