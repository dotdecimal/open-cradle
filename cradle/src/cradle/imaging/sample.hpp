#ifndef CRADLE_IMAGING_SAMPLE_HPP
#define CRADLE_IMAGING_SAMPLE_HPP

#include <cradle/imaging/image.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/channel.hpp>

// This file provides various functions for sampling images.
//
// The following methods of sampling are provided:
//
// * uninterpolated - simply gets the pixel that the specified point lies in
// * linearly interpolated - linearly interpolates at the specified point
// * over a box - gets the average value over the specified region
//
// All methods return an optional pixel value. If the sampling point (or
// box) lies outside the image, an empty value is returned.
//
// All methods come in two forms. The version with the raw_ prefix does NOT
// apply the value mapping to the sample before returning it. (The version
// without the prefix does.)
//

namespace cradle {

// no interpolation
template<unsigned N, class T, class SP>
optional<T>
raw_image_sample(image<N,T,SP> const& img, vector<N,double> const& p);

// Sample an image at a point.
// No interpolation is applied.
api(fun trivial with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The sample value of the image at the specified point. If the point lies outside the image, and empty value is returned.
optional<typename replace_channel_type<T,double>::type>
image_sample(
    // The image that is being sampled.
    image<N,T,SP> const& img,
    // The point where the sample is taken from.
    vector<N,double> const& p);

// with linear interpolation
template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
raw_interpolated_image_sample(image<N,T,SP> const& img,
    vector<N,double> const& p);

// Sample an image at a point.
// Linear interpolation is applied.
api(fun trivial with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The interpolated sample value of the image at the specified point. If the point lies outside the image, and empty value is returned.
optional<typename replace_channel_type<T,double>::type>
interpolated_image_sample(
    // The image that is being sampled.
    image<N,T,SP> const& img,
    // The point where the sample is taken from.
    vector<N,double> const& p);

// over a box
template<unsigned N, class T, class SP>
optional<typename replace_channel_type<T,double>::type>
raw_image_sample_over_box(
    image<N,T,SP> const& img, box<N,double> const& box);

// Sample an image over a box.
// The result is the average image value over that box.
api(fun trivial with(N:1,2,3;T:variant;SP:shared))
template<unsigned N, class T, class SP>
// The average sample value of the image over the specified box. If the box lies outside the image, and empty value is returned.
optional<typename replace_channel_type<T,double>::type>
image_sample_over_box(
    // The image that is being sampled.
    image<N,T,SP> const& img,
    // The box from which the average sample value is calculated.
    box<N,double> const& box);

}

#include <cradle/imaging/sample.ipp>

#endif
