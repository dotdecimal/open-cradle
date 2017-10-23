#ifndef CRADLE_GUI_DISPLAYS_GRAPHING_HPP
#define CRADLE_GUI_DISPLAYS_GRAPHING_HPP

#include <alia/alia.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/canvas.hpp>

#include <cradle/gui/displays/types.hpp>

namespace cradle {

// graph_labels is a container that adds labels to the sides of its child
// (generally a canvas).
struct embedded_graph_labels : noncopyable
{
    void initialize(ui_context& ctx);
    void begin(
        ui_context& ctx,
        accessor<string> const& x_label, accessor<string> const& y_label,
        layout const& layout_spec = default_layout);
    void end();
 private:
    ui_context* ctx_;
    bool active_;
    grid_layout grid_;
    grid_row row_;
    keyed_data<string>* x_label_;
};
struct graph_labels : raii_adaptor<embedded_graph_labels>
{
    graph_labels(ui_context& ctx)
    { initialize(ctx); }
    graph_labels(
        ui_context& ctx,
        accessor<string> const& x_label, accessor<string> const& y_label,
        layout const& layout_spec = default_layout)
    { begin(ctx, x_label, y_label, layout_spec); }
};

// Draw a line graph.
void draw_line_graph(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<vector2d> > const& points);

void draw_line_graph(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    std::vector<vector2d> const& points);

// Draw markers around a list of points on a graph.
void draw_graph_points(
    dataless_ui_context& ctx,
    accessor<rgba8> const& point_color,
    accessor<float> const& point_diameter,
    accessor<rgba8> const& hole_color,
    accessor<float> const& hole_diameter,
    accessor<std::vector<vector2d> > const& points);



api(struct internal)
struct data_reporting_parameters
{
    string label;
    string units;
    // how many digits to display after the decimal point
    unsigned digits;
};

api(struct internal)
struct graph_line_style_info
{
    rgba8 color;
};

void read_style_info(dataless_ui_context& ctx, graph_line_style_info* info,
    style_search_path const* path);

// line_graph is a higher level container that takes of setting up a graph,
// canvas, drawing lines, and allowing highlighting of points.
// It applies external styling information to the graph.
struct line_graph_style_info;
struct graph_highlight;
struct line_graph : noncopyable
{
    line_graph(gui_context& ctx)
      : ctx_(&ctx), active_(false)
    {
        labels_.initialize(ctx);
        rulers_.initialize(ctx);
    }
    line_graph(
        gui_context& ctx, box<2,double> const& scene_box,
        accessor<data_reporting_parameters> const& x_axis_parameters,
        accessor<data_reporting_parameters> const& default_y_axis_parameters,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        optional_storage<cradle::camera> const& camera =
            optional_storage<cradle::camera>(none))
      : ctx_(&ctx), active_(false)
    {
        labels_.initialize(ctx);
        rulers_.initialize(ctx);
        begin(ctx, scene_box, x_axis_parameters,
            default_y_axis_parameters, style, layout_spec, camera);
    }
    ~line_graph() { end(); }

    void begin(
        gui_context& ctx, box<2,double> const& scene_box,
        accessor<data_reporting_parameters> const& x_axis_parameters,
        accessor<data_reporting_parameters> const& default_y_axis_parameters,
        accessor<string> const& style,
        layout const& layout_spec = default_layout,
        optional_storage<cradle::camera> const& camera =
            optional_storage<cradle::camera>(none));
    void end();

    embedded_canvas& canvas() { return canvas_; }

    // with a list of normal points, using external styling
    void do_line(gui_context& ctx,
        accessor<string> const& label, accessor<string> const& line_style,
        accessor<data_reporting_parameters> const& y_axis_parameters,
        accessor<std::vector<vector2d> > const& points);

    // with a list of normal points, using custom styling
    void do_line(
        accessor<string> const& label,
        graph_line_style_info const& line_style_info,
        accessor<data_reporting_parameters> const& y_axis_parameters,
        accessor<std::vector<vector2d> > const& points);

    // with a list of notable data points, using external styling
    void do_line(gui_context& ctx,
        accessor<string> const& label, accessor<string> const& line_style,
        accessor<data_reporting_parameters> const& y_axis_parameters,
        accessor<std::vector<notable_data_point> > const& points);

    // with a list of notable data points, using custom styling
    void do_line(
        accessor<string> const& label,
        graph_line_style_info const& line_style_info,
        accessor<data_reporting_parameters> const& y_axis_parameters,
        accessor<std::vector<notable_data_point> > const& points);

    void do_highlight(gui_context& ctx);

 private:
    dataless_ui_context* ctx_;
    bool active_;
    scoped_substyle substyle_;
    embedded_graph_labels labels_;
    embedded_side_rulers rulers_;
    embedded_canvas canvas_;
    keyed_data<data_reporting_parameters>* x_parameters_;
    graph_highlight* highlight_;
    line_graph_style_info const* style_;
};

// Draw a one dimensional image as a line graph.
void plot_image(gui_context& ctx, image_interface_1d const& img,
    accessor<rgba8> const& color, accessor<line_style> const& style);

// Get a one dimensional image as a plottable list of points.
indirect_accessor<std::vector<vector2d> >
make_image_plottable(gui_context& ctx, image_interface_1d const& img);

}

#endif
