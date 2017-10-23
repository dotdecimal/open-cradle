#include <cradle/math/interpolate.hpp>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace cradle {

regular_grid<1,double>
compute_interpolation_grid(
    std::vector<double> const& sample_positions,
    double growth_tolerance)
{
    size_t n_samples = sample_positions.size();
    if (n_samples <= 1)
    {
        throw exception(
            "compute_interpolation_grid requires at least two samples");
    }

    double const lower_edge = sample_positions[0] -
        (sample_positions[1] - sample_positions[0]) / 2;
    double const upper_edge = sample_positions.back() +
        (sample_positions.back() - sample_positions[n_samples - 2]) / 2;

    double const average_spacing = (upper_edge - lower_edge) / n_samples;

    double smallest_spacing = average_spacing;
    std::vector<double>::const_iterator
        i = sample_positions.begin(), next_i = i + 1;
    while (next_i != sample_positions.end())
    {
        assert(*next_i > *i);
        double const spacing = *next_i - *i;
        if (spacing < smallest_spacing)
            smallest_spacing = spacing;
        i = next_i;
        ++next_i;
    }

    // Use the smallest spacing in the list, unless it would result in
    // creating too many extra samples.
    double best_spacing = (std::max)(average_spacing / growth_tolerance,
        smallest_spacing);

    regular_grid<1,double> grid;
    grid.n_points[0] =
        unsigned((upper_edge - lower_edge) / best_spacing + 0.5);
    grid.spacing[0] = (upper_edge - lower_edge) / grid.n_points[0];
    grid.p0[0] = lower_edge + grid.spacing[0] / 2;
    return grid;
}

}
