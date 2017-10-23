#ifndef CRADLE_IMAGING_SLICING_HPP
#define CRADLE_IMAGING_SLICING_HPP

#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/blend.hpp>
#include <cradle/imaging/view_transforms.hpp>
#include <vector>
#include <cradle/geometry/transformations.hpp>
#include <cradle/geometry/slicing.hpp>
#include <cradle/imaging/sample.hpp>

namespace cradle {

api(struct with(N:1,2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
struct image_slice
{
    unsigned axis;
    double position;
    double thickness;
    image<N,Pixel,Storage> content;
};

//api(struct with(N:1,2,3;Pixel:variant;Storage:shared))
//template<unsigned N, class Pixel, class Storage>
//struct image_slice_dicom
//{
//    image_slice<N,Pixel,Storage> slice;
//        patient_position_type patient_position;
//};

enum class slice_position_type
{
    OUT_OF_BOUNDS,
    SINGLE,
    DOUBLE,
};
struct decoded_slice_position
{
    slice_position_type type;
    // If type is SINGLE, slice0 is the slice.
    // If type is DOUBLE, the position lies between slice0 and slice1 and
    // offset is the relative distance from the actual position to slice0.
    unsigned slice0, slice1;
    double offset;
};

template<unsigned N>
void decode_slice_position(decoded_slice_position* decoded,
    untyped_image_base<N> const& img, unsigned axis, double position)
{
    vector<N,double> p = uniform_vector<N,double>(0);
    p[axis] = position;
    vector<N,double> image_p =
        transform_point(inverse(get_spatial_mapping(img)), p);
    double index = image_p[axis];
    double floored_index = std::floor(index);
    int integer_index = int(floored_index);
    double offset = index - floored_index;
    double const epsilon = 0.0001;
    if (integer_index < 0 || integer_index >= int(img.size[axis]))
    {
        decoded->type = slice_position_type::OUT_OF_BOUNDS;
    }
    else if (offset < 0.5 - epsilon)
    {
        if (integer_index > 0)
        {
            decoded->type = slice_position_type::DOUBLE;
            decoded->slice0 = integer_index - 1;
            decoded->slice1 = integer_index;
            decoded->offset = offset + 0.5;
        }
        else
        {
            decoded->type = slice_position_type::SINGLE;
            decoded->slice0 = integer_index;
        }
    }
    else if (offset > 0.5 + epsilon)
    {
        if (integer_index < int(img.size[axis]) - 1)
        {
            decoded->type = slice_position_type::DOUBLE;
            decoded->slice0 = integer_index;
            decoded->slice1 = integer_index + 1;
            decoded->offset = offset - 0.5;
        }
        else
        {
            decoded->type = slice_position_type::SINGLE;
            decoded->slice0 = integer_index;
        }
    }
    else
    {
        decoded->type = slice_position_type::SINGLE;
        decoded->slice0 = integer_index;
    }
}

template<unsigned N, class Pixel1, class SP1, class Pixel2, class SP2>
void copy_slice_properties(image_slice<N,Pixel1,SP1>& dst,
    image_slice<N,Pixel2,SP2> const& src)
{
    dst.axis = src.axis;
    dst.thickness = src.thickness;
    dst.position = src.position;
}

template<class Pointer>
Pointer slice_pointer(Pointer const& ptr)
{ return ptr; }

template<class Index>
Index slice_index(Index const& index)
{ return index; }

// sliced_view(img, axis, n) returns a view of the nth slice of img (slices
// run perpendicular to the specified axis).
template<unsigned N, class T, class SP>
image_slice<N-1,T,SP>
sliced_view(image<N,T,SP> const& img, unsigned axis, int at)
{
    image_slice<N-1,T,SP> r;

    r.content.pixels = slice_pointer(img.pixels + at * img.step[axis]);

    r.content.size = slice(img.size, axis);
    r.content.step = slice(img.step, axis);

    r.content.origin = slice(img.origin, axis);
    for (unsigned i = 0; i < N; ++i)
        r.content.axes[i] = slice(img.axes[i >= axis ? i + 1 : i], axis);

    r.content.value_mapping = img.value_mapping;
    r.content.units = img.units;

    r.position =
        get_origin(img)[axis] + get_spacing(img)[axis] * (at + 0.5);
    r.thickness = get_spacing(img)[axis];
    r.axis = axis;

    return r;
}

namespace impl {
    template<unsigned N, class DstSP>
    struct sliced_view_fn
    {
        image_slice<N-1,variant,DstSP> dst;
        unsigned axis;
        int at;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& img)
        {
            auto slice = sliced_view(img, axis, at);
            dst.content = as_variant(slice.content);
            copy_slice_properties(dst, slice);
        }
    };
}
// same, but for variant images
template<unsigned N, class SP>
image_slice<N-1,variant,SP>
sliced_view(
    image<N,variant,SP> const& img, unsigned axis, int at)
{
    impl::sliced_view_fn<N,SP> fn;
    fn.axis = axis;
    fn.at = at;
    apply_fn_to_gray_variant(fn, img);
    return fn.dst;
}

template<unsigned N, class Pixel, class SP>
image<N-1,Pixel,shared>
interpolated_slice(
    image<N,Pixel,SP> const& img, unsigned axis,
    unsigned index0, unsigned index1, double offset)
{
    image<N-1,Pixel,SP> slice0 = sliced_view(img, axis, index0).content;
    image<N-1,Pixel,SP> slice1 = sliced_view(img, axis, index1).content;
    image<N-1,Pixel,unique> tmp;
    create_image(tmp, slice0.size);
    copy_untyped_image_info(tmp, slice0);
    raw_blend_images(tmp, slice0, slice1, 1 - offset);
    return share(tmp);
}

namespace impl {
    template<unsigned N>
    struct interpolated_slice_fn
    {
        image<N-1,variant,shared> dst;
        unsigned axis;
        unsigned index0, index1;
        double offset;
        template<class Pixel, class SP>
        void operator()(image<N,Pixel,SP> const& img)
        {
            image<N-1,Pixel,shared> slice =
                interpolated_slice(img, axis, index0, index1, offset);
            dst = as_variant(slice);
        }
    };
}
template<unsigned N, class SP>
image<N-1,variant,shared>
interpolated_slice(
    image<N,variant,SP> const& img, unsigned axis,
    unsigned index0, unsigned index1, double offset)
{
    impl::interpolated_slice_fn<N> fn;
    fn.axis = axis;
    fn.index0 = index0;
    fn.index1 = index1;
    fn.offset = offset;
    apply_fn_to_gray_variant(fn, img);
    return fn.dst;
}

template<unsigned N, class Pixel, class SP>
optional<image<N-1,Pixel,shared> >
interpolated_slice(
    image<N,Pixel,SP> const& img, unsigned axis, double position)
{
    decoded_slice_position decoded;
    decode_slice_position(&decoded, img, axis, position);
    switch (decoded.type)
    {
     case slice_position_type::OUT_OF_BOUNDS:
     default:
        return optional<image<N-1,Pixel,shared> >();
     case slice_position_type::SINGLE:
        return make_eager_image_copy(
            sliced_view(img, axis, decoded.slice0).content);
     case slice_position_type::DOUBLE:
        return interpolated_slice(img, axis, decoded.slice0, decoded.slice1,
            decoded.offset);
    }
}

api(fun trivial with(N:1,2,3;Pixel:variant;SP:shared))
template<unsigned N, class Pixel, class SP>
std::vector<image<N,Pixel,SP> >
extract_slice_images(std::vector<image_slice<N,Pixel,SP> > const& slices)
{
    size_t n_slices = slices.size();
    std::vector<image<N,Pixel,SP> > images(n_slices);
    for (size_t i = 0; i != n_slices; ++i)
        images[i] = slices[i].content;
    return images;
}

api(fun trivial with(N:1,2,3;Pixel:variant;SP:shared))
template<unsigned N, class Pixel, class SP>
slice_description_list
extract_slice_descriptions(std::vector<image_slice<N,Pixel,SP> > const& slices)
{
    size_t n_slices = slices.size();
    slice_description_list descriptions(n_slices);
    for (size_t i = 0; i != n_slices; ++i)
    {
        descriptions[i].position = slices[i].position;
        descriptions[i].thickness = slices[i].thickness;
    }
    return descriptions;
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
unsigned
get_slice_axis(std::vector<image_slice<N,variant,shared> > const& slices)
{
    if (slices.empty())
        throw exception("empty image slice list");
    return slices[0].axis;
}

// Note this function relies on the image slice descriptions being sorted by
// position low to high.
api(fun trivial with(N:1,2,3))
template<unsigned N>
optional<image_slice<N,variant,shared> >
find_sliced_image_slice(
    std::vector<image_slice<N,variant,shared> > const& slices,
    double position)
{
    if (slices.size() == 0 ||
        position < slices[0].position - 0.5 * slices[0].thickness ||
        position > slices.back().position + 0.5 * slices.back().thickness)
    {
        return none;
    }

    for (size_t i = 1; i < slices.size(); ++i)
    {
        if (position < slices[i].position)
        {
            return
                (position - slices[i-1].position < slices[i].position - position) ?
                slices[i-1] : slices[i];
        }
    }
    return slices.back();
}

api(fun trivial with(N:1,2,3;Pixel:variant;SP:shared))
template<unsigned N>
optional<double>
sliced_image_sample(
    std::vector<image_slice<N,variant,shared> > const& slices,
    vector<N + 1,double> const& p)
{
    unsigned slice_axis = get_slice_axis(slices);
    auto slice = find_sliced_image_slice(slices, p[slice_axis]);
    if (slice)
        return image_sample(get(slice).content, alia::slice(p, slice_axis));
    else
        return none;
}

api(fun trivial with(N:1,2,3;Pixel:variant;SP:shared))
template<unsigned N>
string
sliced_image_units(
    std::vector<image_slice<N,variant,shared> > const& slices)
{
    if (slices.empty())
        return string();
    auto merged_units = slices.front().content.units;
    for (auto const& slice : slices)
    {
        if (slice.content.units != merged_units)
        {
            throw cradle::exception(
                "value units are inconsistent across slices");
        }
    }
    return get_name(merged_units);
}

}

#endif
