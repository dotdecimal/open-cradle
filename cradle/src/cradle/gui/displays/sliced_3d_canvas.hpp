#ifndef CRADLE_GUI_DISPLAYS_SLICED_3D_CANVAS_HPP
#define CRADLE_GUI_DISPLAYS_SLICED_3D_CANVAS_HPP

#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/types.hpp>
#include <cradle/geometry/scenes.hpp>
#include <cradle/geometry/polygonal.hpp>

// A sliced_3d_canvas provides a way of rendering a 3D scene on a 2D plane
// by showing a 2D slice of the scene.
// Currently, slices can only be oriented perpendicular to one of the three
// primary axes of the scene.

namespace cradle {

struct canvas;

// sliced_3d_canvas
// Note that sliced_3d_canvas isn't an actual UI element following the scoped
// begin/end pattern. It doesn't actually exist in the widget tree or provide
// its own mapping from screen pixels to scene space.
// It's designed to be embedded within another canvas, such as a normal 2D
// canvas.
struct sliced_3d_canvas : noncopyable
{
    sliced_3d_canvas() {}
    sliced_3d_canvas(
        gui_context& ctx,
        accessor<sliced_scene_geometry<3> > const& scene,
        unsigned slice_axis,
        optional_storage<sliced_3d_view_state> const& state =
            optional_storage<sliced_3d_view_state>(none))
    { initialize(ctx, scene, slice_axis, state); }

    // initialize() initializes the canvas such that the queries below will
    // function properly.
    void initialize(
        gui_context& ctx,
        accessor<sliced_scene_geometry<3> > const& scene,
        unsigned slice_axis,
        optional_storage<sliced_3d_view_state> const& state =
            optional_storage<sliced_3d_view_state>(none));

    dataless_ui_context& context() const { return *ctx_; }

        // direction normal to slice plane
    unsigned slice_axis() const { return slice_axis_; }

    keyed_data_accessor<sliced_scene_geometry<3> > scene_accessor() const;

    sliced_scene_geometry<3> const& scene() const;

    sliced_3d_view_state const& state() const { return state_; }
    // used for transmitting set_value_events to set the state of this canvas
    widget_id get_state_id() const;

 private:
    dataless_ui_context* ctx_;
    struct data;
    data* data_;
    sliced_3d_view_state state_;
    unsigned slice_axis_;
};

// Get the 2D scene space for the canvas's sliced view of the 3D scene.
box2d get_sliced_scene_box(sliced_3d_canvas& canvas);

// Get the scene-space axis that corresponds to the given canvas-space axis.
// camera_axis is the Z-axis of the camera.
unsigned sliced_3d_canvas_axis_to_scene_axis(
    unsigned camera_axis, unsigned canvas_axis);

// Get the canvas-space axis that corresponds to the given scene-space axis.
unsigned sliced_3d_scene_axis_to_canvas_axis(
    unsigned camera_axis, unsigned scene_axis);

// Get the slice position for the current slice axis.
static inline double
get_slice_position(sliced_3d_canvas& canvas)
{ return canvas.state().slice_positions[canvas.slice_axis()]; }

// Get the slice positions for all three axes.
static inline vector3d const&
get_slice_positions(sliced_3d_canvas& canvas)
{ return canvas.state().slice_positions; }

// Set the slice positions for all three axes.
void set_slice_positions(sliced_3d_canvas& canvas, vector3d const& positions);

// Set the slice position for a single axis, leaving the others unchanged.
void set_slice_position(
    sliced_3d_canvas& canvas, unsigned axis, double position);

// Given a 3D image, get the 2D slice that should be displayed on the canvas.
image_interface_2d const&
get_image_slice(gui_context& ctx, sliced_3d_canvas& canvas,
    image_interface_3d const& img);

// slice wheel tool - Allows the user to move from one slice to another (in the
// out-of-plane direction) using the mouse wheel.
void apply_slice_wheel_tool(
    sliced_3d_canvas& canvas, widget_id id, layout_box const& region);

// Same as above, but more convenient when the sliced canvas is embedded
// within a 2D canvas.
void apply_slice_wheel_tool(sliced_3d_canvas& canvas3, canvas& canvas2);

// slice line tool - Draws a slice line and allows the user to drag it.
// If principal_axis is omitted, lines are drawn along both axes and the user
// can drag them simultaneously at their intersection.
void apply_slice_line_tool(
    gui_context& ctx, sliced_3d_canvas& canvas3, canvas& canvas2,
    mouse_button button, rgba8 const& color, line_style const& style,
    int principal_axis = -1);

// If the viewer is currently rendering a side view of the principal axis, this
// will draw a line to indicate the slice position along the principal axis.
void draw_slice_line(
    sliced_3d_canvas& canvas3, canvas& canvas2, unsigned principal_axis,
    rgba8 const& color, line_style const& style = line_style(1, solid_line));

}

#endif
