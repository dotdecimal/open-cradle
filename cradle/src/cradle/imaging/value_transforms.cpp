#include <cradle/imaging/value_transforms.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

struct copy_transformed_pixel_fn
{
    linear_function<double> mapping;
    interpolated_function transform;
    template<class Src>
    void operator()(double& dst, Src const& src)
    { dst = sample(transform, apply(mapping, src)); }
};

template<unsigned N, class SP1, class Pixel2, class SP2>
void static
transform_pixels(
    image<N,double,SP1> const& dst,
    image<N,Pixel2,SP2> const& src,
    interpolated_function const& transform)
{
    copy_transformed_pixel_fn fn;
    fn.mapping = src.value_mapping;
    fn.transform = transform;
    foreach_pixel2(dst, src, fn);
}

template<unsigned N, class Pixel2, class SP2>
image<N,double,shared>
transform_image_values_typed(
    image<N,Pixel2,SP2> const& src,
    interpolated_function const& transform,
    units const& transformed_units)
{
    image<N,double,unique> copy;
    create_image(copy, src.size);
    transform_pixels(copy, src, transform);
    copy_spatial_mapping(copy, src);
    copy.units = transformed_units;
    return share(copy);
}

template<unsigned N>
struct variant_image_value_transformer
{
    image<N,double,shared>* dst;
    interpolated_function const* transform;
    units const* transformed_units;
    template<class Pixel, class SrcSP>
    void operator()(image<N,Pixel,SrcSP> const& src)
    {
        *dst =
            transform_image_values_typed(src, *transform, *transformed_units);
    }
};

template<unsigned N>
image<N,double,shared>
transform_variant_image_values(
    image<N,variant,shared> const& src,
    interpolated_function const& transform,
    units const& transformed_units)
{
    image<N,double,shared> dst;
    if (!empty(src))
    {
        variant_image_value_transformer<N> fn;
        fn.dst = &dst;
        fn.transform = &transform;
        fn.transformed_units = &transformed_units;
        apply_fn_to_gray_variant(fn, src);
    }
    return dst;
}

image3
transform_image_values_3d(
    image3 const& image, interpolated_function const& transform,
    units const& transformed_units)
{
    return as_variant(transform_variant_image_values(image, transform,
        transformed_units));
}

}
