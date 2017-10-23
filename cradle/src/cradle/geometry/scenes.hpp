#ifndef CRADLE_GUI_DISPLAYS_SCENES_HPP
#define CRADLE_GUI_DISPLAYS_SCENES_HPP

#include <cradle/geometry/slicing.hpp>

namespace cradle {

// sliced_scene_geometry describes the geometry of a sliced scene.
api(struct with(N:2,3))
template<unsigned N>
struct sliced_scene_geometry
{
    // the list of slices along each axis
    c_array<N,slice_description_list> slicing;
};

api(fun with(N:2,3))
template<unsigned N>
sliced_scene_geometry<N>
make_sliced_scene_geometry(
    c_array<N,slice_description_list> const& slicing)
{
    sliced_scene_geometry<N> geometry;
    geometry.slicing = slicing;
    return geometry;
}

api(fun with(N:2,3))
template<unsigned N>
sliced_scene_geometry<N>
make_regular_sliced_scene_geometry(regular_grid<N,double> const& scene_grid)
{
    sliced_scene_geometry<N> geometry;
    for (unsigned axis = 0; axis != N; ++axis)
        geometry.slicing[axis] = get_slices_for_grid(scene_grid, axis);
    return geometry;
}

// Get a bounding box for the given scene geometry.
template<unsigned N>
box<N,double> get_bounding_box(sliced_scene_geometry<N> const& scene)
{
    box<N,double> bounds;
    for (unsigned i = 0; i != N; ++i)
    {
        box1d axis_bounds = get_slice_list_bounds(scene.slicing[i]);
        bounds.corner[i] = axis_bounds.corner[0];
        bounds.size[i] = axis_bounds.size[0];
    }
    return bounds;
}

}

#endif
