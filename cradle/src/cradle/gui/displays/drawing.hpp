#ifndef CRADLE_GUI_DISPLAYS_DRAWING_HPP
#define CRADLE_GUI_DISPLAYS_DRAWING_HPP

#include <cradle/gui/common.hpp>
#include <cradle/geometry/polygonal.hpp>
#include <cradle/imaging/color.hpp>
#include <alia/ui/backends/opengl.hpp>
#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/imaging/color_map.hpp>

#include <cradle/gui/displays/types.hpp>

namespace cradle {

struct embedded_canvas;

// COLOR UTILITIES

rgba8 static
apply_uint8_alpha_to_rgb8(rgb8 color, uint8_t alpha)
{
    return apply_alpha(color, alpha);
}

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<uint8_t> const& alpha);

rgba8 static
apply_float_alpha_to_rgb8(rgb8 color, float alpha)
{
    return apply_alpha(color, uint8_t(alpha * 0xff + 0.5));
}

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<float> const& alpha);

rgba8 static
apply_double_alpha_to_rgb8(rgb8 color, double alpha)
{
    return apply_alpha(color, uint8_t(alpha * 0xff + 0.5));
}

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<double> const& alpha);

void set_color(rgba8 const& color);

// LINE DRAWING

// Line styles are specified in the same format as in OpenGL.

api(struct internal)
struct line_stipple
{
    int factor;
    uint16_t pattern;
};

// common line stipples
static line_stipple
    no_line(1, 0),
    solid_line(1, 0xffff),
    dashed_line(10, 0x5555),
    dotted_line(3, 0x5555);

typedef float line_width;

api(struct internal)
struct line_style
{
    line_width width;
    line_stipple stipple;
};

void set_line_style(line_style const& style);

void draw_line(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    vector<2,double> const& p0,
    vector<2,double> const& p1);

void draw_line(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<line_segment<2,double> > const& line);

// POLYGON DRAWING

void draw_poly_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polygon2> const& poly);

void draw_poly_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polygon2 const& poly);

void draw_poly_outline_3d(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polygon2 const& poly,
    double const& z);

void draw_filled_poly(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<polygon2> const& poly);

void draw_filled_poly(
    dataless_ui_context& ctx,
    rgba8 const& color,
    polygon2 const& poly);

// BOX DRAWING

void draw_box_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<box<2,double> > const& box);

void draw_box_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    box<2,double> const& box);

void draw_filled_box(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<box<2,double> > const& box);

void draw_filled_box(
    dataless_ui_context& ctx,
    rgba8 const& color,
    box<2,double> const& box);

// POLYSET DRAWING

void draw_polyset_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polyset> const& set);

void draw_polyset_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polyset const& set);

void draw_polyset_outlines(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<polyset>> const& sets);

void draw_polyset_outlines(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    std::vector<polyset> const& sets);

void draw_filled_polyset(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<polyset> const& set);

void draw_filled_polyset(
    gui_context& ctx,
    rgba8 const& color,
    polyset const& set);

// IMAGE DRAWING

// 8-bit grayscale image
void draw_gray8_image(
    gui_context& ctx,
    accessor<image<2,uint8_t,shared> > const& image,
    accessor<rgba8> const& color = in(rgba8(0xff, 0xff, 0xff, 0xff)));

// 8-bit RGBA image
void draw_rgba8_image(
    gui_context& ctx,
    accessor<image<2,rgba8,shared> > const& image,
    accessor<rgba8> const& color = in(rgba8(0xff, 0xff, 0xff, 0xff)));



// Draw a grayscale image.
void draw_gray_image(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<gray_image_display_options> const& options,
    accessor<rgba8> const& color = in(rgba8(0xff, 0xff, 0xff, 0xff)));

// Draw an isoline for a gray image.
void draw_image_isoline(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    image_interface_2d const& image,
    accessor<double> const& level);

// Draw an isoband for a gray image.
void draw_image_isoband(
    gui_context& ctx,
    accessor<rgba8> const& color,
    image_interface_2d const& image,
    accessor<double> const& low_level,
    accessor<double> const& high_level);

// Draw a shaded isoband for a gray image.
// Shaded isobands have different colors for the high and low levels.
void draw_shaded_image_isoband(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<rgba8> const& low_color,
    accessor<double> const& low_level,
    accessor<rgba8> const& high_color,
    accessor<double> const& high_level);

typedef std::vector<color_map_level<rgba8> > color_map;

void draw_color_mapped_image(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<color_map> const& map);

// MESH DRAWING

void draw_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<triangle_mesh> const& mesh);

void draw_outlined_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<rgba8> const& outline_color,
    accessor<triangle_mesh> const& mesh);

void draw_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<triangle_mesh_with_normals> const& mesh);

void draw_outlined_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<rgba8> const& outline_color,
    accessor<triangle_mesh_with_normals> const& mesh);

}

#endif
