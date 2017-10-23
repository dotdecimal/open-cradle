#ifndef CRADLE_GUI_DISPLAYS_VIEWS_SLICED_3D_VIEW_HPP
#define CRADLE_GUI_DISPLAYS_VIEWS_SLICED_3D_VIEW_HPP

#include <cradle/gui/displays/sliced_3d_canvas.hpp>
#include <cradle/gui/displays/canvas.hpp>

namespace cradle {

struct sliced_3d_view_controller
{
    virtual void do_content(gui_context& ctx, sliced_3d_canvas& c3d,
        embedded_canvas & c2d) const = 0;
    virtual void do_overlays(gui_context& ctx, sliced_3d_canvas& c3d,
        embedded_canvas & c2d) const = 0;
};

void
do_sliced_3d_view(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis,
    layout const& layout_spec,
    vector<3,canvas_flag_set> const& view_flags = make_vector<canvas_flag_set>(CANVAS_FLIP_Y,CANVAS_FLIP_Y,NO_FLAGS));

void
do_locked_sliced_3d_view(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis,
    accessor<double> const& slice_position,
    layout const& layout_spec,
    vector<3,canvas_flag_set> const& view_flags = make_vector<canvas_flag_set>(CANVAS_FLIP_Y,CANVAS_FLIP_Y,NO_FLAGS));

void
do_sliced_3d_view_controls(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis);

}

#endif
