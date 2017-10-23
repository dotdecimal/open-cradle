#ifndef VISUALIZATION_UI_RENDERING_IMAGE_RENDERING_HPP
#define VISUALIZATION_UI_RENDERING_IMAGE_RENDERING_HPP

#include <visualization/data/types/image_types.hpp>
#include <visualization/ui/views/spatial_3d_views.hpp>

// This file provides functions for adding scene graph objects representing
// images to 2D and 3D spatial views.

// Note that in all cases, the image pointer must remain valid as long as the
// scene graph is valid (i.e., as long as the display is still processing the
// current UI pass).

namespace visualization {

// Add a grayscale view of a 3D image to a scene graph.
void
add_gray_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<gray_image_rendering_parameters> const& rendering,
    canvas_layer layer);

// Add a view of a 3D image where voxels are passed through a color map
// calculated from the given list of image levels.
void
add_image_color_wash(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<color_wash_rendering_parameters> const& rendering,
    canvas_layer layer);

// Add a view of a 3D image that draws lines at the given image levels.
void
add_image_isolines(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<isoline_rendering_parameters> const& rendering,
    canvas_layer layer = LINE_OVERLAY_CANVAS_LAYER);

// Add a view of a 3D image that draws bands between the given image levels.
void
add_image_isobands(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<isoband_rendering_parameters> const& rendering,
    canvas_layer layer = FILLED_OVERLAY_CANVAS_LAYER);

// Add a view of a 2D grayscale image projected into a 3D scene.
void
add_projected_gray_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_2d const* image,
    accessor<gray_image_rendering_parameters> const& rendering,
    accessor<rgba8> const& color,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up);

}

#endif
