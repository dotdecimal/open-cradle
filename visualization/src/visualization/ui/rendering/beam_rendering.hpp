#ifndef VISUALIZATION_UI_RENDERING_BEAM_RENDERING_HPP
#define VISUALIZATION_UI_RENDERING_BEAM_RENDERING_HPP

#include <dosimetry/geometry.hpp>

#include <visualization/data/types/geometry_types.hpp>
#include <visualization/ui/views/spatial_3d_views.hpp>

// This file provides functions for adding scene graph objects representing
// various beam-related objects to 3D spatial views.

namespace visualization {

// Add a beam axis to the sliced views.
void
add_sliced_beam_axis(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<beam_geometry> const& geometry,
    accessor<rgba8> const& color,
    accessor<optional<polyset> > const& field_shape,
    canvas_layer layer = LINE_OVERLAY_CANVAS_LAYER);

// Add a beam axis to the projected view.
void
add_projected_beam_axis(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<beam_geometry> const& geometry,
    accessor<rgba8> const& color,
    accessor<optional<polyset> > const& field_shape,
    accessor<bool> const& highlighted = in(false));

}

#endif
