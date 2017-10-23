#ifndef CRADLE_IMAGING_BLEND_HPP
#define CRADLE_IMAGING_BLEND_HPP

#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/channel.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

struct raw_image_blend_fn
{
    double factor1, factor2;
    template<class DstT, class SrcT1, class SrcT2>
    void operator()(DstT& dst, SrcT1 const& src1, SrcT2 const& src2)
    {
        dst = channel_cast<DstT>(src1 * factor1 + src2 * factor2);
    }
};

template<unsigned N, class DstT, class SrcT1, class SrcT2,
    class DstSP, class SrcSP1, class SrcSP2>
void raw_blend_images(image<N,DstT,DstSP> const& dst,
    image<N,SrcT1,SrcSP1> const& src1, image<N,SrcT2,SrcSP2> const& src2,
    double factor1, double factor2)
{
    check_matching_units(src1.units, src2.units);
    check_matching_units(dst.units, src1.units);
    raw_image_blend_fn fn;
    fn.factor1 = factor1;
    fn.factor2 = factor2;
    foreach_pixel3(dst, src1, src2, fn);
}

template<unsigned N, class DstT, class SrcT1, class SrcT2,
    class DstSP, class SrcSP1, class SrcSP2>
void raw_blend_images(image<N,DstT,DstSP> const& dst,
    image<N,SrcT1,SrcSP1> const& src1, image<N,SrcT2,SrcSP2> const& src2,
    double factor)
{
    return raw_blend_images(dst, src1, src2, factor, 1 - factor);
}

struct value_mapped_image_blend_fn
{
    double factor1, factor2, offset;
    template<class DstT, class SrcT1, class SrcT2>
    void operator()(DstT& dst, SrcT1 const& src1, SrcT2 const& src2)
    {
        dst = channel_cast<DstT>(src1 * factor1 + src2 * factor2 + offset);
    }
};

template<unsigned N, class DstT, class SrcT1, class SrcT2,
    class DstSP, class SrcSP1, class SrcSP2>
void blend_value_mapped_images(image<N,DstT,DstSP> const& dst,
    image<N,SrcT1,SrcSP1> const& src1, image<N,SrcT2,SrcSP2> const& src2,
    double factor1, double factor2)
{
    check_matching_units(src1.units, src2.units);
    check_matching_units(dst.units, src1.units);
    value_mapped_image_blend_fn fn;
    fn.factor1 = factor1 * src1.value_mapping.slope;
    fn.factor2 = factor2 * src2.value_mapping.slope;
    fn.offset = factor1 * src1.value_mapping.intercept +
        factor2 * src2.value_mapping.intercept;
    foreach_pixel3(dst, src1, src2, fn);
}

template<unsigned N, class DstT, class SrcT1, class SrcT2,
    class DstSP, class SrcSP1, class SrcSP2>
void blend_value_mapped_images(image<N,DstT,DstSP> const& dst,
    image<N,SrcT1,SrcSP1> const& src1, image<N,SrcT2,SrcSP2> const& src2,
    double factor)
{
    return blend_value_mapped_images(dst, src1, src2, factor, 1 - factor);
}

template<unsigned N, class Pixel, class SP1, class SP2>
image<N,Pixel,shared>
blend_images(
    image<N,Pixel,SP1> const& img1,
    image<N,Pixel,SP2> const& img2,
    double factor1, double factor2)
{
    image<N,Pixel,unique> tmp;
    create_image(tmp, img1.size);
    tmp.units = img1.units;
    copy_spatial_mapping(tmp, img1);
    if (img1.value_mapping == img2.value_mapping)
    {
        tmp.value_mapping = img1.value_mapping;
        raw_blend_images(tmp, img1, img2, factor1, factor2);
    }
    else
    {
        blend_value_mapped_images(tmp, img1, img2, factor1, factor2);
    }
    return share(tmp);
}

// Blend two images and store the result in a third.
// All three must have the same size.
// dst = src1 * factor + src2 * (1 - factor)
template<unsigned N, class Pixel, class SP1, class SP2>
image<N,Pixel,shared>
blend_images(
    image<N,Pixel,SP1> const& img1,
    image<N,Pixel,SP2> const& img2,
    double factor)
{
    return blend_images(img1, img2, factor, 1 - factor);
}

// Blend a list of images and return the result.
// All images must have the same size.
template<unsigned N, class Pixel, class SP1>
image<N, Pixel, shared>
blend_images(
    std::vector<image<N, Pixel, SP1> > const& imgs,
    std::vector<double> factors)
{
    if (imgs.size() == 0 || imgs.size() != factors.size())
    {
        return image<N, Pixel, shared>();
    }
    if (imgs.size() == 1)
    {
        auto img = imgs[0];
        img.value_mapping.slope *= factors[0];
        img.value_mapping.intercept *= factors[0];
        return img;
    }

    // Set up result image
    image<N, Pixel, unique> result;
    create_image(result, imgs[0].size);
    result.units = imgs[0].units;
    set_spatial_mapping(result, imgs[0].origin, get_spacing(imgs[0]));
    // Blend the image
    raw_blend_images(result, imgs[0], imgs[1], factors[0], factors[1]);
    double weight = factors[0] + factors[1];
    for (size_t i = 2; i < imgs.size(); ++i)
    {
        raw_blend_images(result, result, imgs[i], weight, factors[i]);
        weight += factors[i];
    }
    return share(result);
}

namespace impl {
    template<unsigned N, class SP2>
    struct blend_images_fn
    {
        image<N,variant,shared> dst;
        image<N,variant,SP2> img2;
        double factor1, factor2;
        template<class Pixel, class SP1>
        void operator()(image<N,Pixel,SP1> const& img1)
        {
            dst = as_variant(blend_images(img1, cast_variant<Pixel>(img2),
                factor1, factor2));
        }
    };
}
template<unsigned N, class SP1, class SP2>
image<N,variant,shared>
blend_images(
    image<N,variant,SP1> const& img1,
    image<N,variant,SP2> const& img2,
    double factor1, double factor2)
{
    impl::blend_images_fn<N,SP2> fn;
    fn.img2 = img2;
    fn.factor1 = factor1;
    fn.factor2 = factor2;
    apply_fn_to_gray_variant(fn, img1);
    return fn.dst;
}

template<unsigned N, class SP1, class SP2>
image<N,variant,shared>
blend_images(
    image<N,variant,SP1> const& img1,
    image<N,variant,SP2> const& img2,
    double factor)
{
    return blend_images(img1, img2, factor, 1 - factor);
}

}

#endif
