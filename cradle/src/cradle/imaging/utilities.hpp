#ifndef CRADLE_IMAGING_UTILITIES_HPP
#define CRADLE_IMAGING_UTILITIES_HPP

#include <cradle/imaging/foreach.hpp>
#include <cradle/imaging/variant.hpp>

namespace cradle {

struct copy_pixel_fn
{
    template<class Dst, class Src>
    void operator()(Dst& dst, Src const& src)
    { dst = src; }
};

// This copies all the pixels from one image to another.
// The two images must be the same size.
template<unsigned N, class Pixel1, class SP1, class Pixel2, class SP2>
void copy_pixels(image<N,Pixel1,SP1> const& dst,
    image<N,Pixel2,SP2> const& src)
{
    copy_pixel_fn fn;
    foreach_pixel2(dst, src, fn);
}

template<unsigned N, class Pixel1, class SP1, class Pixel2, class SP2>
void copy_image(image<N,Pixel1,SP1>& dst, image<N,Pixel2,SP2> const& src)
{
    copy_pixels(dst, src);
    copy_spatial_mapping(dst, src);
    copy_value_mapping(dst, src);
    dst.units = src.units;
}

struct copy_value_mapped_pixel_fn
{
    linear_function<double> mapping;
    template<class Src>
    void operator()(double& dst, Src const& src)
    { dst = apply(mapping, src); }
};

// This copies all the pixels from one image to another.
// The two images must be the same size.
template<unsigned N, class SP1, class Pixel2, class SP2>
void copy_value_mapped_pixels(image<N,double,SP1> const& dst,
    image<N,Pixel2,SP2> const& src)
{
    copy_value_mapped_pixel_fn fn;
    fn.mapping = src.value_mapping;
    foreach_pixel2(dst, src, fn);
}

template<unsigned N, class Pixel2, class SP2>
image<N,double,shared>
copy_value_mapped_image(image<N,Pixel2,SP2> const& src)
{
    image<N,double,unique> copy;
    create_image(copy, src.size);
    copy_value_mapped_pixels(copy, src);
    copy_spatial_mapping(copy, src);
    copy.units = src.units;
    return share(copy);
}

namespace impl {

    template<unsigned N, class Pixel, class Storage>
    image<N,Pixel,shared>
    make_image_copy(image<N,Pixel,Storage> const& src)
    {
        image<N,Pixel,unique> tmp;
        create_image(tmp, src.size);
        copy_image(tmp, src);
        return share(tmp);
    }

    template<unsigned N, class Pixel>
    image<N,Pixel,shared>
    make_image_copy(image<N,Pixel,shared> const& src)
    {
        return src;
    }

    template<unsigned N>
    struct variant_image_copier
    {
        image<N,variant,shared>* dst;
        template<class Pixel, class SrcSP>
        void operator()(image<N,Pixel,SrcSP> const& src)
        {
            image<N,Pixel,unique> tmp;
            create_image(tmp, src.size);
            copy_image(tmp, src);
            *dst = as_variant(share(tmp));
        }
    };

    template<unsigned N, class Storage>
    image<N,variant,shared>
    make_image_copy(image<N,variant,Storage> const& src)
    {
        image<N,variant,shared> dst;
        if (!empty(src))
        {
            variant_image_copier<N> fn;
            fn.dst = &dst;
            apply_fn_to_variant(fn, src);
        }
        return dst;
    }

    template<unsigned N>
    image<N,variant,shared>
    make_image_copy(image<N,variant,shared> const& src)
    {
        return src;
    }
}

template<unsigned N, class Pixel, class Storage>
image<N,Pixel,shared>
make_eager_image_copy(image<N,Pixel,Storage> const& src)
{
    return impl::make_image_copy(src);
}

template<class Pixel>
struct fill_pixel_fn
{
    Pixel value;
    template<class Dst>
    void operator()(Dst& dst)
    { dst = value; }
};

template<unsigned N, class Pixel, class SP>
void fill_pixels(image<N,Pixel,SP> const& dst, Pixel value)
{
    fill_pixel_fn<Pixel> fn;
    fn.value = value;
    foreach_pixel(dst, fn);
}

template<unsigned N>
image<N,variant,shared>
create_uniform_image(box<N,double> const& box, double intensity, units units)
{
    image<N,uint8_t,unique> img;
    create_image(img, uniform_vector<N,unsigned>(1));
    img.pixels.ptr[0] = 1;
    set_value_mapping(img, 0, intensity, units);
    set_spatial_mapping(img, box.corner, box.size);
    return as_variant(share(img));
}

}

#endif
