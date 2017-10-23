#ifndef CRADLE_MATH_INTERPOLATE_HPP
#define CRADLE_MATH_INTERPOLATE_HPP

#include <cradle/math/common.hpp>
#include <vector>

namespace cradle {

template<class Value, class Fraction, bool IsInteger>
struct interpolator
{
    static Value apply(Value a, Value b, Fraction f)
    { return Value(std::floor(a * (1 - f) + b * f + 0.5)); }
};
template<class Value, class Fraction>
struct interpolator<Value,Fraction,false>
{
    static Value apply(Value a, Value b, Fraction f)
    { return Value(a * (1 - f) + b * f); }
};
template<class Value, class Fraction>
Value interpolate(Value a, Value b, Fraction f)
{
    return interpolator<Value,Fraction,
        std::numeric_limits<Value>::is_integer>::apply(a, b, f);
}

// Interpolate the given data samples onto an evenly spaced grid.
// interpolated_values will receive the interpolated values.
// source_values and source_positions specify the values and positions of the
// source data.
// Note that interpolated_values should have the same number of elements as
// interpolated_grid has points, and source_values and source_points should
// have the same number of elements.
template<class Value>
void interpolate(
    std::vector<Value>* interpolated_values,
    regular_grid<1,double> const& interpolation_grid,
    std::vector<Value> const& source_values,
    std::vector<double> const& source_positions)
{
    interpolated_values->resize(interpolation_grid.n_points[0]);

    typename std::vector<Value>::iterator
        interpolated_value = interpolated_values->begin();
    typename std::vector<Value>::iterator
        interpolated_values_end = interpolated_values->end();

    typename std::vector<Value>::const_iterator
        source_value = source_values.begin();
    typename std::vector<Value>::const_iterator
        next_source_value = source_value + 1;
    typename std::vector<Value>::const_iterator
        source_values_end = source_values.end();

    typename std::vector<double>::const_iterator
        source_position = source_positions.begin();
    typename std::vector<double>::const_iterator
        next_source_position = source_position + 1;

    double interpolated_position = interpolation_grid.p0[0];

    // Fill all interpolated values located before the first source point with
    // the value of the first source point.  In the future, it might be better
    // to have a policy to dictate this behavior.
    while (interpolated_position < *source_position &&
        interpolated_value != interpolated_values_end)
    {
        *interpolated_value++ = *source_value;
        interpolated_position += interpolation_grid.spacing[0];
    }

    // Interpolate.
    while (next_source_value != source_values_end &&
        interpolated_value != interpolated_values_end)
    {
        double f =
            (interpolated_position - *source_position) /
            (*next_source_position - *source_position);
        *interpolated_value++ =
            interpolate(*source_value, *next_source_value, f);

        interpolated_position += interpolation_grid.spacing[0];

        while (next_source_value != source_values_end &&
            *next_source_position < interpolated_position)
        {
            source_value = next_source_value;
            ++next_source_value;
            source_position = next_source_position;
            ++next_source_position;
        }
    }

    // Fill all interpolated values located after the last source point with
    // the value of the last source point.
    while (interpolated_value != interpolated_values_end)
        *interpolated_value++ = *source_value;
}

// Given a list of sample positions (one-dimensional), this calculates an
// evenly spaced grid whose points will lie close to the sample positions.
// The sample positions should be sorted beforehand.
// Note that this is an optimization problem, and for certain position sets,
// different solutions may appear better depending on your preferences.
regular_grid<1,double>
compute_interpolation_grid(
    std::vector<double> const& sample_positions,
    double growth_tolerance = 10.);

}

#endif
