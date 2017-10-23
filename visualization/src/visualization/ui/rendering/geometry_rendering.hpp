#ifndef VISUALIZATION_UI_RENDERING_GEOMETRY_RENDERING_HPP
#define VISUALIZATION_UI_RENDERING_GEOMETRY_RENDERING_HPP

#include <visualization/data/types/geometry_types.hpp>
#include <visualization/ui/views/spatial_3d_views.hpp>

// This file provides functions for adding scene graph objects representing
// common geometric types to 2D and 3D spatial views.

namespace visualization {

// STRUCTURES

// Add a filled structure to the sliced views.
void
add_sliced_filled_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<float> const& opacity,
    canvas_layer layer = FILLED_OVERLAY_CANVAS_LAYER);

// Add an outlined structure to the sliced views.
void
add_sliced_outlined_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_outline_parameters> const& rendering,
    canvas_layer layer = LINE_OVERLAY_CANVAS_LAYER);

void
add_projected_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_projected_rendering_parameters> const& rendering,
    accessor<optional<double>> const& transverse_position);

// MESHES

// Add a filled 3D mesh to the sliced views.
void
add_sliced_filled_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<float> const& opacity,
    canvas_layer layer = LINE_OVERLAY_CANVAS_LAYER);

// Add an outlined 3D mesh to the sliced views.
void
add_sliced_outlined_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_outline_parameters> const& rendering,
    canvas_layer layer = LINE_OVERLAY_CANVAS_LAYER);

// Add a projected view of a 3D mesh.
void
add_projected_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_projected_rendering_parameters> const& rendering);

// Add a projected view of a 3D structure.
void
add_projected_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<rgb8> const& color,
    accessor<spatial_region_projected_rendering_parameters> const& rendering,
    accessor<optional<double>> const& transverse_position);

// POINTS

// Add a point.
void
add_sliced_point(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_point> const& point,
    accessor<point_rendering_parameters> const& rendering,
    canvas_layer layer = POINT_OVERLAY_CANVAS_LAYER);

}

#endif

