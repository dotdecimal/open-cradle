#ifndef CRADLE_DISPLAYS_GEOMETRY_UTILITIES_HPP
#define CRADLE_DISPLAYS_GEOMETRY_UTILITIES_HPP

#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/types.hpp>

namespace cradle {

struct triangle_mesh;
struct triangle_mesh_with_normals;
struct sliced_3d_canvas;

void do_spatial_region_display_controls(
    gui_context& ctx,
    alia::grid_layout& g,
    accessor<spatial_region_display_options> const& options);

line_style make_line_style(line_stipple_type type, float width);

ALIA_DEFINE_FLAG_TYPE(spatial_region_drawing)
ALIA_DEFINE_FLAG(spatial_region_drawing, 0x1, SPATIAL_REGION_HIGHLIGHTED)

// POINT DRAWING

void draw_point(
    gui_context& ctx, sliced_3d_canvas& canvas,
    accessor<point_rendering_options> const& options,
    accessor<gui_point> const& point);

// MESH DRAWING

void draw_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgb8> const& color,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags = NO_FLAGS);

void draw_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags = NO_FLAGS);

void draw_filled_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgba8> const& color);

void draw_mesh_slice_outline(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgba8> const& color,
    accessor<line_style> const& style);

indirect_accessor<request<triangle_mesh> >
remove_normals(
    gui_context& ctx,
    accessor<request<triangle_mesh_with_normals> > const& mesh);

// STRUCTURE DRAWING

request<polyset>
compose_structure_slice_request(
    request<structure_geometry> const& structure,
    sliced_scene_geometry<3> const& scene,
    unsigned slice_axis, double slice_position);

void draw_structure_slice(
    gui_context& ctx, sliced_3d_canvas& canvas,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags = NO_FLAGS);

void do_structure_selection_controls(gui_context& ctx,
    accessor<std::map<string,gui_structure> > const& structures,
    accessor<std::map<string,bool> > const& structure_visibility);

}

#endif
