#ifndef VISUALIZATION_UI_RENDERING_GRID_RENDERING_HPP
#define VISUALIZATION_UI_RENDERING_GRID_RENDERING_HPP

#include <visualization/data/types/geometry_types.hpp>
#include <visualization/ui/views/spatial_3d_views.hpp>

// This file provides functions for adding scene graph objects representing
// common geometric types to 2D and 3D spatial views.

namespace visualization {

// Add an adaptive 3D grid to the sliced views.
void
add_sliced_adaptive_grid(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<adaptive_grid> > const& grid,
    accessor<double> const& min_spacing,
    canvas_layer layer = FILLED_OVERLAY_CANVAS_LAYER);

// Add a generic 3d grid to the sliced views.
// The grid is specified as a list of boxes.
void
add_sliced_grid_boxes(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<std::vector<box3d> > > const& grid_boxes,
    canvas_layer layer = FILLED_OVERLAY_CANVAS_LAYER);

}

#endif
