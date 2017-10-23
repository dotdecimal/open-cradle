#include <limits>
#include <cmath>

#include <cradle/imaging/geometry.hpp>

namespace cradle {

template<unsigned N, class Pixel, class Storage>
double
compute_image_integral_over_line_segment(
    image<N,Pixel,Storage> const& img,
    line_segment<N,double> const& segment)
{
    ray<N,double> ray(segment[0], unit(as_vector(segment)));
    image_integral_computer<N,Pixel,Storage> computer(img, ray, std::numeric_limits<double>::lowest(), (std::numeric_limits<double>::max)());
    return computer.compute_integral_to(length(segment));
}

template<unsigned N, class Pixel, class Storage>
double
compute_image_integral_over_line_segment_min_max(
    image<N,Pixel,Storage> const& img,
    line_segment<N,double> const& segment,
    double min,
    double max,
    double zero_value)
{
    ray<N,double> ray(segment[0], unit(as_vector(segment)));
    image_integral_computer<N,Pixel,Storage> computer(img, ray, min, max, zero_value);
    return computer.compute_integral_to(length(segment));
}

namespace impl {
    template<unsigned N>
    struct line_segment_image_integral_computer
    {
        line_segment<N,double> segment;
        double result;
        double min;
        double max;
        double zero_value = 0.;
        template<class Pixel, class Storage>
        void operator()(image<N,Pixel,Storage> const& img)
        { result = compute_image_integral_over_line_segment_min_max(img, segment, min, max, zero_value); }
    };
}

template<unsigned N, class Storage>
double
compute_image_integral_over_line_segment(
    image<N,variant,Storage> const& img,
    line_segment<N,double> const& segment)
{
    impl::line_segment_image_integral_computer<N> fn;
    fn.segment = segment;
    fn.min = std::numeric_limits<double>::lowest();
    fn.max = (std::numeric_limits<double>::max)();
    apply_fn_to_gray_variant(fn, img);
    return fn.result;
}

template<unsigned N, class Storage>
double
compute_image_integral_over_line_segment_min_max(
    image<N,variant,Storage> const& img,
    line_segment<N,double> const& segment,
    double min,
    double max,
    double zero_value = 0.)
{
    impl::line_segment_image_integral_computer<N> fn;
    fn.segment = segment;
    fn.min = min;
    fn.max = max;
    fn.zero_value = zero_value;
    apply_fn_to_gray_variant(fn, img);
    return fn.result;
}

template<unsigned N, class Pixel, class Storage>
double compute_image_integral_over_ray(
    image<N,Pixel,Storage> const& img,
    ray<N,double> const& ray)
{
    image_integral_computer<N,Pixel,Storage> computer(img, ray);
    return computer.compute_integral_to(
        computer.get_intersection().exit_distance);
}

namespace impl {
    template<unsigned N>
    struct ray_image_integral_computer
    {
        cradle::ray<N,double> ray;
        double result;
        template<class Pixel, class Storage>
        void operator()(image<N,Pixel,Storage> const& img)
        { result = compute_image_integral_over_ray(img, ray); }
    };
}
template<unsigned N, class Storage>
double
compute_image_integral_over_ray(
    image<N,variant,Storage> const& img,
    ray<N,double> const& ray)
{
    impl::ray_image_integral_computer<N> fn;
    fn.ray = ray;
    apply_fn_to_gray_variant(fn, img);
    return fn.result;
}

template<unsigned N, class Pixel, class Storage>
double
compute_inverse_image_integral_over_ray(
    image<N,Pixel,Storage> const& img,
    ray<N,double> const& ray, double integral)
{
    image_integral_computer<N,Pixel,Storage> computer(img, ray);
    return computer.compute_distance_to(integral);
}

namespace impl {
    template<unsigned N>
    struct inverse_ray_image_integral_computer
    {
        cradle::ray<N,double> ray;
        double integral;
        double result;
        template<class Pixel, class Storage>
        void operator()(image<N,Pixel,Storage> const& img)
        { result = compute_inverse_image_integral_over_ray(img, ray, integral); }
    };
}
template<unsigned N, class Storage>
double
compute_inverse_image_integral_over_ray(
    image<N,variant,Storage> const& img,
    ray<N,double> const& ray, double integral)
{
    impl::inverse_ray_image_integral_computer<N> fn;
    fn.ray = ray;
    fn.integral = integral;
    apply_fn_to_gray_variant(fn, img);
    return fn.result;
}

template<unsigned N, class Pixel, class Storage>
image_integral_computer<N,Pixel,Storage>::image_integral_computer(
    image<N,Pixel,Storage> const& img,
    ray<N,double> const& ray,
    double min,
    double max,
    double zero_value)
  : img_(img), ray_(ray), min_(min), max_(max), zero_value_(zero_value)
{
    intersection_ = intersection(ray, get_bounding_box(img));
    if (intersection_.n_intersections == 0)
        return;

    matrix<N+1,N+1,double> inverse_spatial_mapping =
        inverse(get_spatial_mapping(img));

    // the origin of the ray in image (pixel) space
    vector<N,double> origin_in_image =
        transform_point(inverse_spatial_mapping, ray.origin);

    // Calculate the slope and the reciprocal slope.
    // The slope is the distance the ray travels along each image axis per
    // unit length of the ray. Note that the distance traveled along the
    // image axes is in image space, whereas the distance traveled along the
    // ray is in the external space, so this accounts for the spatial mapping's
    // affect on the result. (If each pixel is two units wide, then the
    // integral will be twice as large.)
    vector<N,double> slope = transform_vector(
        inverse_spatial_mapping, ray.direction);

    // Avoid divide-by-zero when calculating the reciprocal slope.
    double const min_slope = std::numeric_limits<double>::epsilon() * 10;
    double const max_reciprocal_slope = 1 / min_slope;
    for (unsigned i = 0; i < N; ++i)
    {
        reciprocal_slope_[i] = (std::fabs(slope[i]) < min_slope) ?
            max_reciprocal_slope : std::fabs(1 / slope[i]);
    }

    distance_ = intersection_.entrance_distance;
    integral_ = distance_ * zero_value_;

    // Calculate the starting (entrance) point of the ray, in image space.
    vector<N,double> start = origin_in_image + slope *
        intersection_.entrance_distance;

    // Calculate the distance along the ray to the next voxel grid line for
    // each of the voxel axes.
    for (unsigned i = 0; i < N; ++i)
    {
        if (slope[i] > 0)
        {
            next_grid_line_[i] = (std::floor(start[i]) + 1 -
                origin_in_image[i]) * reciprocal_slope_[i];
        }
        else
        {
            next_grid_line_[i] = (origin_in_image[i] - std::floor(start[i])) *
                reciprocal_slope_[i];
        }
    }

    // Calculate how many bytes we need to advance the image pixel pointer
    // whenever we step to a new pixel along each axis.
    for (unsigned i = 0; i < N; ++i)
        steps_[i] = ((slope[i] < 0) ? -1 : 1) * img.step[i];

    // Initialize the image pixel pointer.
    vector<N,int> index;
    for (unsigned i = 0; i < N; ++i)
        index[i] = int(std::floor(start[i]));
    pixel_ = get_pixel_iterator(img, index);

    bounds_ = get_quick_bounds(img);
}

template<unsigned N, class Pixel, class SP>
double
image_integral_computer<N,Pixel,SP>::compute_integral_to(double distance)
{
    if (intersection_.n_intersections == 0)
        return distance * zero_value_;

    // Check that the distance is within the range in which the ray intersects
    // the image volume.
    double extra_integral = 0.;
    if (distance > intersection_.exit_distance)
    {
        extra_integral = (distance - intersection_.exit_distance) * zero_value_;
        distance = intersection_.exit_distance;
    }
    if (distance < intersection_.entrance_distance)
        return distance * zero_value_;

    double integral = integral_;

    // If the requested distance is farther than our current distance, we need
    // to walk forward along the ray.
    if (distance > distance_)
    {
        double value = 0;
        double d = distance_;
        while (1)
        {
            if (within_bounds(bounds_, pixel_))
			{
                value = apply(img_.value_mapping, double(*pixel_));
			}
			else
			{
				value = zero_value_;
			}
            // Determine which axis's grid line the ray will hit first.
            unsigned axis = 0;
            for (unsigned i = 1; i < N; ++i)
            {
                if (next_grid_line_[i] < next_grid_line_[axis])
                    axis = i;
            }
            // If that grid line is beyond the requested distance, we're done.
            if (next_grid_line_[axis] > distance)
                break;
            // Check if value is within bounds
            if ((min_ > value) || (value > max_))
            {
                value = zero_value_;
            }
            // Accumulate the integral.
            integral += (next_grid_line_[axis] - d) * value;
            // Advance to that grid line.
            d = next_grid_line_[axis];
            // Move next_grid_line_(axis) to the one after this.
            next_grid_line_[axis] += reciprocal_slope_[axis];
            // And move to the next pixel.
            pixel_ += steps_[axis];
        }

        // Remember where we left off.
        integral_ = integral;
        distance_ = d;

        // Since we're stepping along pixel boundaries, we don't necessarily
        // stop exactly where we want to, so we have to correct for that.
        integral += (distance - d) * value;
    }

    // Otherwise, we need to walk back towards the origin of the ray.
    // This case is fairly analogous to the above one.
    else
    {
        // Since we're tracing backwards, the next grid line is actually the
        // one before our current position, not after.
        vector<N,double> next_grid_line =
            next_grid_line_ - reciprocal_slope_;

        double value = 0;
        double d = distance_;
        while (1)
        {
            if (within_bounds(bounds_, pixel_))
                value = apply(img_.value_mapping, double(*pixel_));
            unsigned axis = 0;
            for (unsigned i = 1; i < N; ++i)
            {
                if (next_grid_line[i] > next_grid_line[axis])
                    axis = i;
            }
            if (next_grid_line[axis] < distance)
                break;
            if ((min_ <= value) && (value <= max_))
            {
                integral -= (d - next_grid_line[axis]) * value;
            }
            d = next_grid_line[axis];
            next_grid_line[axis] -= reciprocal_slope_[axis];
            pixel_ -= steps_[axis];
        }

        integral_ = integral;
        distance_ = d;
        next_grid_line_ = next_grid_line + reciprocal_slope_;

        if ((min_ <= value) && (value <= max_))
        {
            integral -= (d - distance) * value;
        }
    }

    return integral + extra_integral;
}

template<unsigned N, class Pixel, class SP>
double image_integral_computer<N,Pixel,SP>::compute_integral_to(
    vector<N,double> const& p)
{
    return compute_integral_to(
        dot(p - ray_.origin, ray_.direction));
}

template<unsigned N, class Pixel, class SP>
double image_integral_computer<N,Pixel,SP>::compute_distance_to(
       double integral)
{
    // not yet implemented for limited value ranges
    assert(min_ == std::numeric_limits<double>::lowest());
    assert(max_ == (std::numeric_limits<double>::max)());

    if (intersection_.n_intersections == 0)
        return std::numeric_limits<double>::infinity();

    double d = distance_;

    // If the requested integral is larger than our current integral, we need
    // to walk forward along the ray.
    if (integral > integral_)
    {
        double value = 0;
        double i = integral_;
        unsigned axis;
        double d_integral;
        while (1)
        {
            if (within_bounds(bounds_, pixel_))
                value = apply(img_.value_mapping, double(*pixel_));
            // Determine which axis's grid line the ray will hit first.
            axis = 0;
            for (unsigned j = 1; j < N; ++j)
            {
                if (next_grid_line_[j] < next_grid_line_[axis])
                    axis = j;
            }
            d_integral = (next_grid_line_[axis] - d) * value;
            if (i + d_integral >= integral ||
                next_grid_line_[axis] > intersection_.exit_distance)
            {
                break;
            }
            i += d_integral;
            // Advance to that grid line.
            d = next_grid_line_[axis];
            // Move next_grid_line_(axis) to the one after this.
            next_grid_line_[axis] += reciprocal_slope_[axis];
            // And move to the next pixel.
            pixel_ += steps_[axis];
        }

        // Remember where we left off.
        integral_ = i;
        distance_ = d;

        // Since we're stepping along pixel boundaries, we don't necessarily
        // stop exactly where we want to, so we have to correct for that.
        if (i + d_integral >= integral)
            return d + (integral - i) / value;
        else
            return std::numeric_limits<double>::infinity();
    }

    // Otherwise, we need to walk back towards the origin of the ray.
    // This case is fairly analogous to the above one.
    else
    {
        // Since we're tracing backwards, the next grid line is actually the
        // one before our current position, not after.
        vector<N,double> next_grid_line =
            next_grid_line_ - reciprocal_slope_;

        double value = 0;
        double i = integral_;
        unsigned axis;
        double d_integral;
        while (1)
        {
            if (within_bounds(bounds_, pixel_))
                value = apply(img_.value_mapping, double(*pixel_));
            axis = 0;
            for (unsigned j = 1; j < N; ++j)
            {
                if (next_grid_line[j] > next_grid_line[axis])
                    axis = j;
            }
            d_integral = (d - next_grid_line[axis]) * value;
            if (i - d_integral <= integral ||
                next_grid_line[axis] < intersection_.entrance_distance)
            {
                break;
            }
            i -= d_integral;
            d = next_grid_line[axis];
            next_grid_line[axis] -= reciprocal_slope_[axis];
            pixel_ -= steps_[axis];
        }

        integral_ = i;
        distance_ = d;
        next_grid_line_ = next_grid_line + reciprocal_slope_;

        if (i - d_integral <= integral)
            return d - (i - integral) / value;
        else
            return -std::numeric_limits<double>::infinity();

    }
}

}
