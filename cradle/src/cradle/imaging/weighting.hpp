#ifndef CRADLE_IMAGING_WEIGHTING_HPP
#define CRADLE_IMAGING_WEIGHTING_HPP

#include <cradle/imaging/image.hpp>
#include <cradle/common.hpp>
#include <cradle/imaging/channel.hpp>
#include <vector>

// This file implements images that are weighted combinations of other images.
//
// Calling make_weighted_combination(images, n), where 'images' is an
// array of n weighted_images (see immediately below), returns an image
// representing their weighted combination.
//
// The image is lazy. The actual values of its pixels are only computed when
// accessed.

namespace cradle {

template<unsigned N, class Weight, class Pixel, class SP>
struct weighted_image
{
    cradle::image<N,Pixel,SP> image;
    Weight weight;
};

template<unsigned N, class Weight, class Pixel, class SP>
struct weighted_image_pointer
{
    typename SP::template pointer_type<N,Pixel>::type pointer;
    Weight weight;
};

template<unsigned N, class Weight, class Pixel, class SP>
struct weighted_combination_pointer
{
    std::vector<weighted_image_pointer<N,Weight,Pixel,SP> > pointers;
    typename SP::template step_type<N,Pixel>::type index;
};

template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N-1,Weight,Pixel,SP>
slice_pointer(weighted_combination_pointer<N,Weight,Pixel,SP> const& src)
{
    weighted_combination_pointer<N-1,Weight,Pixel,SP> sliced;
    sliced.index = slice_index(src.index);
    size_t n_pointers = src.pointers.size();
    sliced.pointers.resize(n_pointers);
    for (size_t i = 0; i != n_pointers; ++i)
    {
        sliced.pointers[i].pointer = slice_pointer(src.pointers[i].pointer);
        sliced.pointers[i].weight = src.pointers[i].weight;
    }
    return sliced;
}

template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N,Weight,Pixel,SP>&
operator+=(
    weighted_combination_pointer<N,Weight,Pixel,SP>& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    i.index += offset;
    return i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N,Weight,Pixel,SP>
operator+(
    weighted_combination_pointer<N,Weight,Pixel,SP> const& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    weighted_combination_pointer<N,Weight,Pixel,SP> r = i;
    r += offset;
    return r;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N,Weight,Pixel,SP>
operator+(
    typename SP::template step_type<N,Pixel>::type offset,
    weighted_combination_pointer<N,Weight,Pixel,SP> const& i)
{
    return offset + i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N,Weight,Pixel,SP>&
operator-=(
    weighted_combination_pointer<N,Weight,Pixel,SP>& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    i.index -= offset;
    return i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_pointer<N,Weight,Pixel,SP>
operator-(
    weighted_combination_pointer<N,Weight,Pixel,SP> const& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    weighted_combination_pointer<N,Weight,Pixel,SP> r = i;
    r -= offset;
    return r;
}

template<unsigned N, class Weight, class Pixel, class SP>
struct weighted_combination_iterator
{
    size_t n_pointers;
    weighted_image_pointer<N,Weight,Pixel,SP> const* pointers;
    typename SP::template step_type<N,Pixel>::type index;

    typename replace_channel_type<Pixel,Weight>::type operator*()
    {
        typename replace_channel_type<Pixel,Weight>::type sum;
        fill_channels(sum, 0);
        for (size_t i = 0; i != n_pointers; ++i)
        {
            weighted_image_pointer<N,Weight,Pixel,SP> const& ptr = pointers[i];
            sum += *(get_iterator(ptr.pointer) + index) * ptr.weight;
        }
        return sum;
    }
};

template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP>&
operator+=(
    weighted_combination_iterator<N,Weight,Pixel,SP>& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    i.index += offset;
    return i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP>
operator+(
    weighted_combination_iterator<N,Weight,Pixel,SP> const& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    weighted_combination_iterator<N,Weight,Pixel,SP> r = i;
    r += offset;
    return r;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP>
operator+(
    typename SP::template step_type<N,Pixel>::type offset,
    weighted_combination_iterator<N,Weight,Pixel,SP> const& i)
{
    return offset + i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP>&
operator-=(
    weighted_combination_iterator<N,Weight,Pixel,SP>& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    i.index -= offset;
    return i;
}
template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP>
operator-(
    weighted_combination_iterator<N,Weight,Pixel,SP> const& i,
    typename SP::template step_type<N,Pixel>::type offset)
{
    weighted_combination_iterator<N,Weight,Pixel,SP> r = i;
    r -= offset;
    return r;
}

template<unsigned N, class Weight, class Pixel, class SP>
bool operator==(
    weighted_combination_iterator<N,Weight,Pixel,SP> const& a,
    weighted_combination_iterator<N,Weight,Pixel,SP> const& b)
{
    return a.index == b.index && a.pointers == b.pointers &&
        a.n_pointers == b.n_pointers;
}
template<unsigned N, class Weight, class Pixel, class SP>
bool operator!=(
    weighted_combination_iterator<N,Weight,Pixel,SP> const& a,
    weighted_combination_iterator<N,Weight,Pixel,SP> const& b)
{
    return !(a == b);
}

template<unsigned N, class Weight, class Pixel, class SP>
weighted_combination_iterator<N,Weight,Pixel,SP> get_iterator(
    weighted_combination_pointer<N,Weight,Pixel,SP> const& ptr)
{
    weighted_combination_iterator<N,Weight,Pixel,SP> i;
    i.n_pointers = ptr.pointers.size();
    i.pointers = ptr.pointers.empty() ? 0 : &ptr.pointers[0];
    i.index = ptr.index;
    return i;
}

template<class Pixel, class SP>
struct weighted_combination
{
    template<unsigned N, class WeightedPixel>
    struct pointer_type
    {
        typedef weighted_combination_pointer<N,
            typename pixel_channel_type<WeightedPixel>::type,Pixel,SP> type;
    };
    template<unsigned N, class WeightedPixel>
    struct iterator_type
    {
        typedef weighted_combination_iterator<N,
            typename pixel_channel_type<WeightedPixel>::type,Pixel,SP> type;
    };
    template<unsigned N, class WeightedPixel>
    struct reference_type
    {
        typedef typename replace_channel_type<Pixel,
            typename pixel_channel_type<WeightedPixel>::type>::type type;
    };
    template<unsigned N, class WeightedPixel>
    struct step_type
      : SP::template step_type<N,Pixel>
    {};
};

template<unsigned N, class Weight, class Pixel, class SP>
image<N,typename replace_channel_type<Pixel,Weight>::type,
    weighted_combination<Pixel,SP> >
make_weighted_combination(
    weighted_image<N,Weight,Pixel,SP> const* images,
    size_t n_images)
{
    if (n_images == 0)
        throw exception("weighted combination image list is empty");

    image<N,Pixel,SP> const& img0 = images[0].image;

    for (size_t i = 1; i != n_images; ++i)
    {
        image<N,Pixel,SP> const& img = images[i].image;
        if (img.size != img0.size ||
            !same_value_mapping(img, img0) ||
            !same_spatial_mapping(img, img0) ||
            img.step != img0.step)
        {
            throw exception(
                "weighted combination images aren't compatible");
        }
    }

    image<N,typename replace_channel_type<Pixel,Weight>::type,
        weighted_combination<Pixel,SP> > img;
    img.pixels.pointers.resize(n_images);
    double total_weight = 0;
    for (size_t i = 0; i != n_images; ++i)
    {
        weighted_image<N,Weight,Pixel,SP> const& wi = images[i];
        weighted_image_pointer<N,Weight,Pixel,SP>& ptr =
            img.pixels.pointers[i];
        ptr.pointer = wi.image.pixels;
        ptr.weight = wi.weight;
        total_weight += wi.weight;
    }
    img.pixels.index = img0.step[0] * 0;
    img.size = img0.size;
    copy_spatial_mapping(img, img0);
    copy_value_mapping(img, img0);
    img.value_mapping.intercept *= total_weight;
    img.step = img0.step;
    return img;
}

}

#endif
