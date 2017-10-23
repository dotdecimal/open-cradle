#ifndef CRADLE_GEOMETRY_SLICING_HPP
#define CRADLE_GEOMETRY_SLICING_HPP

#include <cradle/geometry/regular_grid.hpp>
#include <cradle/geometry/forward.hpp>

namespace cradle {

// Geometric description of the slice.
api(struct)
struct slice_description
{
    // The postion value of the slice along the slice axis.
    double position;
    // The thinkness value of the slice
    double thickness;
};

// Is the point p inside the slice s?
api(fun trivial name(point_inside_slice))
bool is_inside(slice_description const& s, double p);

// Is the point p inside one of the slices in the given list?
api(fun name(point_inside_slice_list))
bool is_inside(slice_description_list const& slices, double p);

// Given a list of slices and a point p, get the slice position that's
// closest to p.
api(fun)
double round_slice_position(slice_description_list const& slices, double p);

// Given a list of slices and a point p, this advances p by n slices.
// n can be negative to move backwards through the list.
// The return value is the new position of p, clamped to stay within the list.
api(fun)
double advance_slice_position(
    slice_description_list const& slices, double p, int n);

// Given a regular grid, this gets the list of slices on that grid for a
// particular axis.
api(fun with(N:1,2,3))
template<unsigned N>
slice_description_list
get_slices_for_grid(regular_grid<N,double> const& grid, unsigned axis)
{
    check_index_bounds("axis", axis, N);
    unsigned n_slices = grid.n_points[axis];
    slice_description_list slices(n_slices);
    for (unsigned i = 0; i != n_slices; ++i)
    {
        slice_description& slice = slices[i];
        slice.position = grid.p0[axis] + grid.spacing[axis] * i;
        slice.thickness = grid.spacing[axis];
    }
    return slices;
}

// Get an interpolation grid that covers the list of slice positions.
template<unsigned N>
regular_grid<N,double>
compute_interpolation_grid(slice_description_list const& slices)
{
    return map(
        [](slice_description const& slice) { return slice.position; },
        slices);
}

// Get the bounds of the given slice list.
// The bounds include the thickness of the slices.
// The slices must be sorted.
box1d get_slice_list_bounds(slice_description_list const& slices);

}

#endif
