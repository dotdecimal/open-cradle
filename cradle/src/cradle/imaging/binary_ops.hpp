#ifndef CRADLE_BINARY_OPS_HPP
#define CRADLE_BINARY_OPS_HPP

#include <cradle/imaging/geometry.hpp>

// This file provides utilities for doing pixel-by-pixel binary operations on
// pairs of images. This is non-trivial when the images lie on different grids.

namespace cradle {

// GENERAL BINARY OPS

// Given a pair of images, this creates a grid that occupies the region in
// which both images are defined. The spacing of the grid in each dimension is
// the smaller of the spacing of the two images in that dimension.
// It returns false iff there is no space shared by the two images.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
bool calculate_common_grid(
    regular_grid<N,double>* grid,
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2);

// Applies a binary operation to two images and stores the result in a third
// image.
// Both images must be gray scale, but they can have different channel types
// and can be on different grids. If they occupy different physical spaces, the
// operation is only computed over the intersection of the two images.
// There is no unit checking done. The result image has no units.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Op>
image<N,double,shared>
compute_binary_op(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2,
    Op& op);

// This is the same as above, but instead of storing the result in an image,
// it simply applies your operation to each pair of pixels. This allows you to
// accumulate information about the resulting image without actually storing
// it anywhere.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Op>
void reduce_binary_op(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2,
    Op& op);

// SUM, WEIGHTED SUM

// Computes the sum of two images.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_sum(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2);

// Computes the weighted sum of two images.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_weighted_sum(
    image<N,Pixel1,Storage1> const& img1, double weight1,
    image<N,Pixel2,Storage2> const& img2, double weight2);

// Computes the sum of a list of images.
api(fun with(N:1,2,3))
template<unsigned N>
// The resulting sum image of the list of images.
image<N,variant,shared>
sum_image_list(
    // The list of images which will have their values added together.
    std::vector<image<N,variant,shared> > const& images);

// DIFFERENCE

// Computes the difference between two images.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
image<N,double,shared>
compute_difference(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2);

// Computes the maximum difference between two images.
template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2>
double compute_max_difference(
    image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2);

}

#include <cradle/imaging/binary_ops.ipp>

#endif
