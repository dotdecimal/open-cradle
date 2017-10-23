#ifndef CRADLE_IMAGING_API_HPP
#define CRADLE_IMAGING_API_HPP

#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/utilities.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/statistics.hpp>
#include <cradle/imaging/merge_slices.hpp>
#include <cradle/imaging/discretize.hpp>
#include <cradle/imaging/sample.hpp>
#include <cradle/imaging/histogram.hpp>

namespace cradle {

// Creates an image with constant value throughout the region defined by the provided box
api(fun trivial with(N:1,2,3) name(create_uniform_image))
template<unsigned N>
// A uniform valued image covering a box
image<N,variant,shared>
create_uniform_image_no_units(
    // The box defining the image position
    box<N,double> const& box,
    // The value of the image pixels
    double intensity)
{
    return create_uniform_image(box, intensity, no_units);
}

// Creates an image with constant value throughout the region defined by the provided box
api(fun trivial with(N:1,2,3))
template<unsigned N>
// A uniform valued image covering a box
image<N,variant,shared>
create_uniform_image_with_units(
    // The box defining the image position
    box<N,double> const& box,
    // The value of the image pixels
    double intensity,
    // The units of the image pixel values
    units const& units)
{
    return create_uniform_image(box, intensity, units);
}

// Creates an image with constant pixel values at all points in the provided grid
api(fun with(N:1,2,3))
template<unsigned N>
// A uniform valued image coincident with grid
image<N,variant,shared>
create_uniform_image_on_grid(
    // The grid over which to produce the image
    regular_grid<N,double> const& grid,
    // The value at which to set all pixels
    double intensity,
    // The units of the value for the image
    units const& units)
{
    image<N,double,unique> image;
    create_image_on_grid(image, grid);
    fill_pixels(image, intensity);
    image.units = units;
    return as_variant(share(image));
}

// Creates an image based on a grid with a corresponding (cell centered) data array
api(fun with(N:1,2,3))
template<unsigned N>
// An image of the provided grid based data
image<N,variant,shared>
create_image(
    // The grid which specifies the locations of the scalar data array values
    regular_grid<N,double> const& grid,
    // The values (at cell centers) of each grid point
    std::vector<double> const& values)
{
    if (values.size() != product(grid.n_points))
        throw exception("value array size is inconsistent with grid size");

    image<N,double,unique> img;
    create_image_on_grid(img, grid);

    for (std::size_t i = 0; i != values.size(); ++i)
        img.pixels.ptr[i] = values[i];

    return as_variant(share(img));
}

// Gets a regular grid with points corresponding to the image pixel centers
api(fun trivial with(N:1,2,3))
template<unsigned N>
// The grid of pixel centers
regular_grid<N,double>
get_image_grid(
    // The image whose point grid is desired
    image<N,variant,shared> const& image)
{ return get_grid(image); }

api(fun trivial with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
optional<image_slice<N-1,Pixel,Storage> >
uninterpolated_image_slice(
    image<N,Pixel,Storage> const& image,
    unsigned slice_axis,
    double slice_position)
{
    if (slice_axis > N - 1)
        throw exception("uninterpolated_image_slice: invalid axis");

    vector<N,double> slice_p = uniform_vector<N>(0.);
    slice_p[slice_axis] = slice_position;
    int slice_number = int(transform_point(
        inverse(get_spatial_mapping(image)), slice_p)[slice_axis]);

    if (slice_number < 0 || slice_number >= int(image.size[slice_axis]))
        return none;

    return sliced_view(image, slice_axis, slice_number);
}

api(fun with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
optional<image<N-1,Pixel,Storage> >
interpolated_image_slice(
    image<N,Pixel,Storage> const& image,
    unsigned slice_axis,
    double slice_position)
{
    if (slice_axis > N - 1)
        throw exception("interpolated_slice_image: invalid axis");
    return interpolated_slice(image, slice_axis, slice_position);
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
vector<N,unsigned>
image_size(image<N,variant,shared> const& image)
{
    return image.size;
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
vector<N,double>
image_origin(image<N,variant,shared> const& image)
{
    return image.origin;
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
vector<N,double>
image_spacing(image<N,variant,shared> const& image)
{
    return get_spacing(image);
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
matrix<N+1,N+1,double>
image_transformation(image<N,variant,shared> const& image)
{
    return get_spatial_mapping(image);
}

api(fun trivial)
image<2,variant,shared>
rotate_2d_image(image<2,variant,shared> const& image, int angle)
{
    if (angle % 90 != 0)
        throw exception(
            "rotate_2d_image: angle must be a multiple of 90 degrees");
    image2 rotated =
        aligned_view(transformed_view(
            image, rotation(cradle::angle<double,degrees>(angle))));
    return rotated;
}

api(fun trivial with(N:1,2,3))
template<unsigned N>
image<N,variant,shared>
convert_image_to_8bit(image<N,variant,shared> const& image)
{
    auto min_max = image_min_max(image);
    cradle::image<N,uint8_t,shared> discretized;
    if (min_max)
    {
        discretize(discretized, image,
            linear_function<double>(get(min_max).min,
                (get(min_max).max - get(min_max).min) / 255));
    }
    return as_variant(discretized);
}

// Scale an image's value mapping so that its pixel values appear scaled.
api(fun trivial with(N:1,2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
image<N,Pixel,Storage>
scale_image_values(image<N,Pixel,Storage> const& image, double scale_factor,
    units const& expected_units, units const& new_units)
{
    check_units(expected_units, image.units);
    cradle::image<N,variant,shared> result = image;
    result.value_mapping.slope *= scale_factor;
    result.value_mapping.intercept *= scale_factor;
    result.units = new_units;
    return result;
}

// Creates a histogram using the specified 1D image
// Note: the minimum and maximum values can be looser than the min/max in the image if you want
// the histogram to cover a larger range, values less than min are not counted, values
// greater than max are counted in the last bin
api(fun trivial with(N:1,2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
// 1D image where the spatial dimension
// ranges over the value space of the original image and the image values are
// occurrence counts
image<1,variant,shared>
image_histogram(
    // Image to create the histogram from
    image<N,Pixel,Storage> const& image,
    // Minimum value for output histogram bins
    double min_value,
    // Maximum value for output histogram bins
    double max_value,
    // Size of each bin
    double bin_size)
{
    return as_variant(compute_histogram<uint32_t>(
        image, min_value, max_value, bin_size));
}

// Creates a histogram using the specified 1D image which only includes the points specified in a list of weighted grid indices
api(fun trivial with(N:1,2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
// 1D image where the spatial dimension
// ranges over the value space of the original image and the image values are
// weight totals specified in the given list of weighted grid indices
image<1,variant,shared>
partial_image_histogram(
    // Image to create the histogram from
    image<N,Pixel,Storage> const& image,
    // List of weighted grid indices which will serve as data points in the histogram
    std::vector<weighted_grid_index> const& indices,
    // Minimum value for output histogram bins
    double min_value,
    // Maximum value for output histogram bins
    double max_value,
    // Size of each bin
    double bin_size)
{
    return as_variant(compute_partial_histogram<float>(
        image, get_elements_pointer(indices), indices.size(),
        min_value, max_value, bin_size));
}

// Calculates the bounding box of the image of a specific size
api(fun trivial with(N:1,2,3))
template<unsigned N>
// Bounding box of image size
box<N,double>
image_bounding_box(
    // Image to compute bounding box of
    image<N,variant,shared> const& image)
{
    return get_bounding_box(image);
}

api(fun trivial with(N:1,2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
string
image_value_units(image<N,Pixel,Storage> const& image)
{
    return get_name(image.units);
}

}

#endif
