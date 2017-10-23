/*
 * Author(s):  Salvadore Gerace <sgerace@dotdecimal.com>
 *             Thomas Madden <tmadden@mgh.harvard.edu>
 * Date:       03/27/2013
 *
 * Copyright:
 * This work was developed as a joint effort between .decimal, Inc. and
 * Partners HealthCare under research agreement A213686; as such, it is
 * jointly copyrighted by the participating organizations.
 * (c) 2013 .decimal, Inc. All rights reserved.
 * (c) 2013 Partners HealthCare. All rights reserved.
 */

#ifndef CRADLE_GUI_DISPLAYS_PROJECTED_CANVAS_HPP
#define CRADLE_GUI_DISPLAYS_PROJECTED_CANVAS_HPP

#include <cradle/geometry/meshing.hpp>
#include <cradle/geometry/polygonal.hpp>
#include <cradle/geometry/multiple_source_view.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/canvas.hpp>
#include <cradle/external/opengl.hpp>

namespace cradle {

// A camera includes a zoom level and a position.
// The position is the point in the scene where the canvas will be centered.
api(struct internal)
struct camera3
{
    double zoom;
    alia::vector<3,double> position;
    alia::vector<3,double> direction;
    alia::vector<3,double> up;
};

struct projected_canvas_flag_tag {};
typedef alia::flag_set<projected_canvas_flag_tag> projected_canvas_flag_set;

camera3 make_default_camera(alia::box<3,double> const& scene_box);

struct projected_canvas : noncopyable
{
    // uses an already-initialized, already-begun embedded canvas for drawing onto
    projected_canvas(embedded_canvas & ec,multiple_source_view const& view) :
        embedded_canvas_(ec), view_(view), transforms_to_pop_(0) {}
    ~projected_canvas() { end(); }

    void begin();
    void end();

    embedded_canvas& canvas() const { return embedded_canvas_; }

    cradle::camera3 camera() const;
    void set_camera(cradle::camera3 const& cam);

    cradle::multiple_source_view const& view() const { return view_; }
    void set_view(cradle::multiple_source_view const& view) { view_ = view; }

    double get_zoom_level();

    void draw_polyset_outline(
        dataless_ui_context& ctx,
        accessor<rgba8> const& color,
        accessor<line_style> const& style,
        accessor<polyset> const& set,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up) const;

    void draw_poly_outline(
        dataless_ui_context& ctx,
        accessor<rgba8> const& color,
        accessor<line_style> const& style,
        accessor<polygon2> const& poly,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up) const;

    void draw_filled_poly(
        dataless_ui_context& ctx,
        accessor<rgba8> const& color,
        accessor<line_style> const& style,
        accessor<polygon2> const& poly,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up) const;

    void draw_polyline(
        dataless_ui_context& ctx,
        accessor<rgba8> const& color,
        accessor<line_style> const& style,
        accessor<std::vector<vector2d> > const& polyline,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up) const;

    // draw a range compensator onto a plane in space somewhere
    void draw_image(
        gui_context& ctx,
        image_interface_2d const& image,
        accessor<gray_image_display_options> const& options,
        accessor<rgba8> const& color,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up) const;

    // draw a range compensator onto a plane in space somewhere
    void draw_image_isoline(
        gui_context& ctx,
        image_interface_2d const& image,
        accessor<rgba8> const& color,
        accessor<line_style> const& style,
        accessor<plane<double> > const& draw_plane,
        accessor<vector3d> const& draw_plane_up,
        accessor<double> const& level) const;

    // push the current (pre-camera /pre-view) transform onto the stack, to be restored later by pop_transform
    void push_transform();

    // restore the last-pushed transform
    void pop_transform();

    // offset whatever is rendered after this
    void translate(vector3d const& offset);

    // scale whatever is rendered after this by <scale>[i] along each axis
    void scale(vector3d const& scale);

    // rotate whatever is rendered after this by <angle_in_degrees> about the given axis
    void rotate(double angle_in_degrees, vector3d const& axis);

    // disable writing to the depth buffer. Good for when rendering things with alpha < 1
    void disable_depth_write();

    // enable writing to the depth buffer
    void enable_depth_write();

    embedded_canvas const& get_embedded_canvas() const { return embedded_canvas_; }

 private:
    void set_scene_coordinates();
    void set_canvas_coordinates();

    embedded_canvas & embedded_canvas_;
    cradle::multiple_source_view view_;
    bool active_;
    bool in_scene_coordinates_;
    int transforms_to_pop_;
    GLuint shader_program_;
};

void set_camera(projected_canvas& canvas, cradle::camera3 const& new_camera);

vector3d canvas_to_world(projected_canvas & c, vector2d const& p);

void clear_canvas(projected_canvas& canvas, alia::rgb8 const& color);

void clear_depth(projected_canvas& canvas);

}

#endif
