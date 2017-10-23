#ifndef CRADLE_IMAGING_STATISTICS_HPP
#define CRADLE_IMAGING_STATISTICS_HPP

#include <cradle/imaging/image.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/geometry/regular_grid.hpp>

namespace cradle {

// Get the minimum and maximum values in the given image.
api(fun with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The minimum and maximum of the values in the image
optional<min_max<double> >
image_min_max(
    // Image to sample for values
    image<N,T,SP> const& img);

// Get the minimum and maximum values in the given image, as raw pixel values.
template<unsigned N, class T, class SP>
optional<min_max<T> >
raw_image_min_max(image<N,T,SP> const& img);

// Given a vector representing the individual min/max values for a set of
// images, this merges them to create the overall min/max values for the set.
api(fun with(T:double))
template<class T>
optional<min_max<T> >
merge_min_max_values(std::vector<optional<min_max<T> > > const& values);

// Get the overall minimum and maximum values for a vector of images.
api(fun disk_cached with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The minimum and maximum of the values of the images in the vector.
optional<min_max<double> >
image_list_min_max(
    // List of images to sample for values
    std::vector<image<N,T,SP> > const& imgs);

// Get the minimum, maximum, and mean values in the given image.
api(fun with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The statistics (minimum, maximum, mean, and number of values) of the given image.
statistics<double>
image_statistics(
    // The image from which the statistics will be calculated.
    image<N,T,SP> const& img);

// Get the minimum, maximum, and mean values in the given image, as raw pixel
// values.
template<unsigned N, class T, class SP>
statistics<T> raw_image_statistics(image<N,T,SP> const& img);

// Get the minimum, maximum, and mean values over a subset of the given image.
api(fun with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The statistics (minimum, maximum, mean, and number of values) over a subset of the given image.
statistics<double>
partial_image_statistics(
    // The image from which the statistics will be calculated.
    image<N,T,SP> const& img,
    // The subset of image indices from which the satistics will be calculated.
    std::vector<size_t> const& indices);

// Get the minimum, maximum, and mean values over a subset of the given image,
// where each pixel specified in the subset also has an associated weight.
api(fun with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The statistics (minimum, maximum, mean, and number of values) over a weighted subset of the given image.
statistics<double>
weighted_partial_image_statistics(
    // The image from which the statistics will be calculated.
    image<N,T,SP> const& img,
    // The subset of image indices and their weights from which the satistics will be calculated.
    std::vector<weighted_grid_index> const& indices);

// Given a vector representing the individual statistics for a set of images,
// this merges them to create the overall statistics for the set.
api(fun with(T:double))
template<class T>
// The statistics (minimum, maximum, mean, and number of values) merged from a vector of statistics.
statistics<T>
merge_statistics(
    // The vector of statistics to be merged.
    std::vector<statistics<T> > const& stats);

// Get the overall statistics for a vector of images.
api(fun with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The overall statistics (minimum, maximum, mean, and number of values) of a vector of images.
statistics<double>
image_list_statistics(
    // The vector of images from which the statistics will be calculated.
    std::vector<image<N,T,SP> > const& imgs);

// Given the statisitcs of an image and its histogram, compute its standard
// deviation.
template<class Bin>
optional<double>
standard_deviation_from_image_stats(
    image<1,Bin,shared> const& histogram,
    statistics<double> const& stats);

}

#include <cradle/imaging/statistics.ipp>

#endif
