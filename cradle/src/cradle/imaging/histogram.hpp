#ifndef CRADLE_IMAGING_HISTOGRAM_HPP
#define CRADLE_IMAGING_HISTOGRAM_HPP

#include <cradle/imaging/foreach.hpp>
#include <cradle/imaging/channel.hpp>
#include <vector>
#include <cradle/geometry/regular_grid.hpp>

namespace cradle {

// FULL HISTOGRAMS - Calculated over the whole image.

// Compute a histogram of a grayscale image.
// Note that the histogram itself is a 1D image where the spatial dimension
// ranges over the value space of the original image and the image values are
// occurrence counts.
// min_value and max_value should be the min and max values that occur within
// the image. However, they can be looser if you want the histogram to cover a
// larger range.
template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_histogram(image<N,Pixel,Storage> const& img,
    double min_value, double max_value, double bin_size);

// This is implemented using the following...

// This initializes a 1D image so that it can be used to store a histogram.
template<class Bin>
void create_image_for_histogram(image<1,Bin,unique>& bin_img,
    size_t n_bins, double min_value, double bin_size);

// accumulate_histogram() takes an existing array of bins and increments the
// each bin's value for each image value that falls in that bin.
template<class Bin, unsigned N, class Pixel, class Storage>
void accumulate_histogram(Bin* bins, size_t n_bins,
    double min_value, double bin_size, image<N,Pixel,Storage> const& img);

// PARTIAL HISTOGRAMS - Computed over only part of the image.

// This computes a partial histogram of an image. Only the pixels specified
// in the indices vector are sampled. The image must be contiguous.
template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_partial_histogram(
    image<N,Pixel,Storage> const& img,
    size_t const* indices, size_t n_indices,
    double min_value, double max_value, double bin_size);

// Similar to accumulate_histogram() above, but for partial histograms.
template<class Bin, class PixelIterator>
void accumulate_partial_histogram(
    Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    PixelIterator pixels, size_t n_pixels,
    linear_function<double> const& value_mapping,
    size_t const* indices, size_t n_indices);

// WEIGHTED PARTIAL HISTOGRAMS - Computed over only part of the image, and
// each index has an associated weight. Instead of each bin storing the
// number of values that fall within its range, it stores the sum of those
// values' weights.

// Compute a weighted partial histogram of an image.
// The image must be contiguous.
template<class Bin, unsigned N, class Pixel, class Storage>
image<1,Bin,shared>
compute_partial_histogram(
    image<N,Pixel,Storage> const& img,
    weighted_grid_index const* indices, size_t n_indices,
    double min_value, double max_value, double bin_size);

// Similar to accumulate_histogram() above, but for weighted partial
// histograms.
template<class Bin, class PixelIterator>
void accumulate_partial_histogram(
    Bin* bins, size_t n_bins,
    double min_value, double bin_size,
    PixelIterator pixels, size_t n_pixels,
    linear_function<double> const& value_mapping,
    weighted_grid_index const* indices, size_t n_indices);

// RAW HISTOGRAMS - Computed over the whole image but using the pixel values
// directly as indices. This is faster, but gives no control over bin sizes or
// offsets.

// This computes a histogram for an 8- or 16-bit image by directly using the
// pixel values as indices into the histogram vector.
template<unsigned N, class Pixel, class Storage>
void compute_raw_histogram(std::vector<unsigned>* hist,
    image<N,Pixel,Storage> const& img);

// Similar to accumulate_histogram() above, but for raw histograms.
template<unsigned N, class Pixel, class Storage>
void accumulate_raw_histogram(unsigned* hist,
    image<N,Pixel,Storage> const& img);

}

#include <cradle/imaging/histogram.ipp>

#endif
