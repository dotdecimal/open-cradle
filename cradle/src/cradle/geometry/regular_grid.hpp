#ifndef CRADLE_GEOMETRY_REGULAR_GRID_HPP
#define CRADLE_GEOMETRY_REGULAR_GRID_HPP

#include <cradle/geometry/common.hpp>

namespace cradle {

CRADLE_GEOMETRY_DEFINE_TYPEDEFS(regular_grid)

// Get the position of a point on a regular grid.
api(fun trivial with(N:1,2,3;T:double))
template<unsigned N, typename T>
vector<N,T>
get_grid_point(regular_grid<N,T> const& grid, vector<N,unsigned> const& index)
{
    vector<N,T> p;
    for (unsigned i = 0; i < N; ++i)
        p[i] = grid.p0[i] + grid.spacing[i] * index[i];
    return p;
}

// Get the center point of a regular grid.
api(fun trivial with(N:1,2,3;T:double) name(get_grid_center))
template<unsigned N, typename T>
vector<N,T> get_center(regular_grid<N,T> const& grid)
{
    vector<N,T> p;
    for (unsigned i = 0; i < N; ++i)
        p[i] = grid.p0[i] + (grid.spacing[i] * grid.n_points[i]) * 0.5;
    return p;
}

// Get the size of a regular grid.
api(fun trivial with(N:1,2,3;T:double) name(get_grid_size))
template<unsigned N, typename T>
vector<N,T> get_size(regular_grid<N,T> const& grid)
{
    vector<N,T> s;
    for (unsigned i = 0; i < N; ++i)
        s[i] = grid.spacing[i] * grid.n_points[i];
    return s;
}

template<unsigned N, typename T>
regular_grid<N-1,T> slice(regular_grid<N,T> const& grid, unsigned axis)
{
    regular_grid<N-1,T> r;
    r.p0 = slice(grid.p0, axis);
    r.spacing = slice(grid.spacing, axis);
    r.n_points = slice(grid.n_points, axis);
    return r;
}

template<unsigned N, typename T>
regular_grid<N+1,T> unslice(regular_grid<N,T> const& grid, unsigned axis,
    cradle::regular_grid<1,T> const& extra)
{
    regular_grid<N+1,T> r;
    r.p0 = unslice(grid.p0, axis, extra.p0[0]);
    r.spacing = unslice(grid.spacing, axis, extra.spacing[0]);
    r.n_points = unslice(grid.n_points, axis, extra.n_points[0]);
    return r;
}

template<unsigned N, typename T>
void create_grid_for_box(regular_grid<N,T>* grid,
    box<N,T> const& box, vector<N,T> const& spacing)
{
    grid->spacing = spacing;
    for (unsigned i = 0; i < N; ++i)
        grid->n_points[i] = int(std::ceil(box.size[i] / spacing[i]));
    grid->p0 = get_center(box) -
        (cradle::vector<N,T>(grid->n_points) * spacing - spacing) / 2;
}

template<unsigned N, typename T>
void create_grid_covering_box(regular_grid<N,T>* grid,
    box<N,T> const& box, vector<N,unsigned> const& counts)
{
        grid->n_points = counts;
    grid->p0 = box.corner;
    for (unsigned i = 0; i < N; ++i)
        grid->spacing[i] = box.size[i] / (double(counts[i]) - 1.0);
}

// Create a regular grid that covers a box (from cell centers).
// make_grid_for_box(b, s) creates a grid that covers the box b with spacing s.
api(fun trivial with(N:1,2,3,4;T:double))
template<unsigned N, typename T>
regular_grid<N,T> make_grid_for_box(
    box<N,T> const& box, vector<N,T> const& spacing)
{
    regular_grid<N,T> grid;
    create_grid_for_box(&grid, box, spacing);
    return grid;
}

// Create a regular grid that fully covers a box (from cell corners).
// make_grid_covering_box(b, c) creates a grid that covers the box b with point count c.
api(fun trivial with(N:1,2,3,4;T:double))
template<unsigned N, typename T>
regular_grid<N,T> make_grid_covering_box(
    box<N,T> const& box, vector<N,unsigned> const& counts)
{
    regular_grid<N,T> grid;
    create_grid_covering_box(&grid, box, counts);
    return grid;
}

// Create a regular grid.
// creates a grid with start point p0, spacing s, and point count np.
api(fun trivial with(N:1,2,3,4;T:double))
template<unsigned N, typename T>
regular_grid<N,T> make_regular_grid(
    vector<N,T> const& p0, vector<N,T> const& spacing, vector<N,unsigned> const& np)
{
    regular_grid<N,T> grid;
        grid.p0 = p0;
        grid.spacing = spacing;
        grid.n_points = np;
    return grid;
}

api(fun trivial with(N:1,2,3,4;T:double) name(grid_bounding_box))
template<unsigned N, typename T>
box<N,T> bounding_box(regular_grid<N,T> const& grid)
{
    return box<N,T>(grid.p0, grid.spacing * vector<N,T>(grid.n_points -
        uniform_vector<N,unsigned>(1)));
}
template<unsigned N, typename T>
void compute_bounding_box(optional<box<N,T> >& box,
    regular_grid<N,T> const& grid)
{
    cradle::box<N,T> bb = bounding_box(grid);

    if (box)
    {
        for (unsigned i = 0; i < N; ++i)
        {
            if (bb.corner[i] < box.get().corner[i])
                box.get().corner[i] = bb.corner[i];
            if (high_corner(bb)[i] > high_corner(box.get())[i])
                box.get().size[i] = high_corner(bb)[i] - box.get().corner[i];
        }
    }
    else
        box = bb;
}

api(struct)
struct weighted_grid_index
{
    // 1D index into the grid
    uint32_t index;
    float weight;
};

api(fun)
double
sum_grid_index_weights(std::vector<weighted_grid_index> const& cells);

// Get the volume of the voxels in a grid.
api(fun trivial)
double
regular_grid_voxel_volume(regular_grid<3, double> const& grid)
{
    return product(grid.spacing);
}

}

#endif
