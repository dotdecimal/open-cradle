#ifndef CRADLE_IMAGING_INTEGRAL_HPP
#define CRADLE_IMAGING_INTEGRAL_HPP

#include <cfloat>

#include <cradle/geometry/intersection.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/bounds.hpp>

namespace cradle {

// Compute the integral of all image values along the given line segment.
api(fun with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
// The integral of all image values along the line segment.
double
compute_image_integral_over_line_segment(
    // The image that the line segment will run through.
    image<N,Pixel,Storage> const& image,
    // The line segment that the image integral will be calculated over.
    line_segment<N,double> const& segment);

// Compute the integral of image values along the given line segment that lie within a range.
api(fun with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
// The integral of image values along the line segment that lie within the given range.
double
compute_image_integral_over_line_segment_min_max(
    // The image that the line segment will run through.
    image<N,Pixel,Storage> const& image,
    // The line segment that the image integral will be calculated over.
    line_segment<N,double> const& segment,
    // The minimum value that will be calculated in the integral.
    double min,
    // The maximum value that will be calculated in the integral.
    double max,
    // The value to add to the integral when image pixel is outside the range
    // of 'min' and 'max'.
    double zero_value);

// Compute the integral of all image values along the given ray.
api(fun with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
// The integral of all image values along the ray.
double
compute_image_integral_over_ray(
    // The image that the ray will run through.
    image<N,Pixel,Storage> const& image,
    // The ray that the image integral will be calculated over.
    ray<N,double> const& ray);

// This is the inverse of compute_image_integral_over_ray.
// It computes the distance along the given ray that yields the given integral.
api(fun with(N:2,3;Pixel:variant;Storage:shared))
template<unsigned N, class Pixel, class Storage>
double
compute_inverse_image_integral_over_ray(
    image<N,Pixel,Storage> const& image,
    ray<N,double> const& ray, double integral);

// If you need to compute the value integral along the same ray (in the same
// image) to many different points, using this class can be more efficient than
// repeatedly calling the stand-alone function.  Rather than always tracing
// from the origin, this class will trace from the last point requested to the
// new point and add/subtract to get the integral to the new point.  Due to
// this behavior, this class is most effective when the points are ordered.
// (In fact, there are worst-case scenarios where this would actually be less
// efficient.) It's also possible that it could accumulate round-off error if
// used carelessly, so beware.
//
template<unsigned N, class Pixel, class Storage>
class image_integral_computer
{
 public:
    image_integral_computer(image<N,Pixel,Storage> const& img,
        ray<N,double> const& ray, double min = -DBL_MAX, double max = DBL_MAX,
        double zero_value = 0.);

    // Get the integral from the ray origin to the point which is the given
    // distance along the ray.
    double compute_integral_to(double distance);

    // Get the integral from the ray origin to the given point, projected onto
    // the ray.
    double compute_integral_to(vector<N,double> const& p);

    // This is the inverse of integral_to().  It computes the distance that
    // must be traversed along the ray in order to accumulate a given integral.
    double compute_distance_to(double integral);

    // Get the intersection between the ray and the image volume.
    ray_box_intersection<N,double> const& get_intersection()
    { return intersection_; }

    typedef typename Storage::template iterator_type<N,Pixel>::type
        iterator_type;
    struct state
    {
        double distance;
        double integral;
        iterator_type pixel;
        vector<N,double> next_grid_line;
    };

    // Save the current state of the computer into the given state structure.
    void save_state(state& s)
    {
        s.distance = distance_;
        s.integral = integral_;
        s.pixel = pixel_;
        s.next_grid_line = next_grid_line_;
    }
    // Restore the state of the computer to that in the given state structure.
    void restore_state(state const& s)
    {
        distance_ = s.distance;
        integral_ = s.integral;
        pixel_ = s.pixel;
        next_grid_line_ = s.next_grid_line;
    }

 private:
    image<N,Pixel,Storage> const& img_;
    ray<N,double> const& ray_;

    // the intersection between the ray and the image volume
    ray_box_intersection<N,double> intersection_;

    // the current distance along the ray
    double distance_;

    // the integral from the origin to the current location
    double integral_;

    // low end of range of values to integrate
    double min_;

    // high end of range of values to integrate
    double max_;

    // an iterator referring to the image pixel at the current location
    iterator_type pixel_;

    // used for bounds checking on pixel pointer
    typename quick_bounds_check_type<iterator_type>::type bounds_;

    // the number of pixels to advance the iterator when stepping along each
    // axis
    typedef typename Storage::template step_type<N,Pixel>::type step_type;
    vector<N,step_type> steps_;

    // the distance along the ray to the next pixel grid line for each axis
    // ('next' meaning further away from the origin than the current location)
    vector<N,double> next_grid_line_;

    // reciprocal slope (distance along the ray per pixel, in each axis)
    vector<N,double> reciprocal_slope_;

    // the value of "zero" for the image, this is added when pixel value is
    // out of range
    double zero_value_;
};

}

#include <cradle/imaging/integral.ipp>

#endif
