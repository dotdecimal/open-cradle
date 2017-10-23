#include <cradle/gui/displays/graphing.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/external/opengl.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/gui/requests.hpp>
#include <cradle/gui/displays/inspection.hpp>

namespace cradle {

void embedded_graph_labels::initialize(ui_context& ctx)
{
    ctx_ = &ctx;
    active_ = false;
}

void embedded_graph_labels::begin(
    ui_context& ctx,
    accessor<string> const& x_label, accessor<string> const& y_label,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    active_ = true;
    x_label_ = make_persistent_copy(ctx, x_label);
    grid_.begin(ctx, layout_spec);
    row_.begin(grid_, GROW);
    {
        rotated_layout rotated(ctx);
        {
            panel p(ctx, text("graph-label"), UNPADDED);
            do_text(ctx, y_label, CENTER_X);
        }
    }
}
void embedded_graph_labels::end()
{
    ui_context& ctx = *ctx_;
    if (ctx.pass_aborted)
        return;
    alia_if (active_)
    {
        row_.end();
        {
            panel p(ctx, text("graph-label"), UNPADDED);
            {
                grid_row row(grid_);
                do_spacer(ctx);
                do_text(ctx, make_accessor(x_label_), CENTER_X);
            }
        }
        grid_.end();
        active_ = false;
    }
    alia_end
}

void draw_line_graph(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    std::vector<vector2d> const& points)
{
    set_color(color);
    set_line_style(style);
    glBegin(GL_LINE_STRIP);
    for (auto const& point : points)
        glVertex2d(point[0], point[1]);
    glEnd();
}

void draw_line_graph(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<vector2d> > const& points)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(style) &&
        is_gettable(points))
    {
        draw_line_graph(ctx, get(color), get(style), get(points));
    }
}

void static
draw_line_graph(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    std::vector<notable_data_point> const& points)
{
    set_color(color);
    set_line_style(style);
    glBegin(GL_LINE_STRIP);
    for (auto const& p : points)
        glVertex2d(p.position[0], p.position[1]);
    glEnd();
}

void static
draw_graph_points(
    dataless_ui_context& ctx,
    rgba8 point_color,
    float point_diameter,
    rgba8 hole_color,
    float hole_diameter,
    std::vector<vector2d> const& points)
{
    set_color(point_color);
    glPointSize(point_diameter);
    glBegin(GL_POINTS);
    for (auto const& point : points)
        glVertex2d(point[0], point[1]);
    glEnd();

    if (hole_diameter > 0)
    {
        set_color(hole_color);
        glPointSize(hole_diameter);
        glBegin(GL_POINTS);
        for (auto const& point : points)
            glVertex2d(point[0], point[1]);
        glEnd();
    }
}

void draw_graph_points(
    dataless_ui_context& ctx,
    accessor<rgba8> const& point_color,
    accessor<float> const& point_diameter,
    accessor<rgba8> const& hole_color,
    accessor<float> const& hole_diameter,
    accessor<std::vector<vector2d> > const& points)
{
    if (is_render_pass(ctx) &&
        is_gettable(point_color) && is_gettable(point_diameter) &&
        is_gettable(hole_color) && is_gettable(hole_diameter) &&
        is_gettable(points))
    {
        draw_graph_points(ctx,
            get(point_color), get(point_diameter),
            get(hole_color), get(hole_diameter),
            get(points));
    }
}

void static
draw_graph_points(
    dataless_ui_context& ctx,
    float point_diameter,
    rgba8 hole_color,
    float hole_diameter,
    std::vector<notable_data_point> const& points)
{
    glPointSize(point_diameter);
    glBegin(GL_POINTS);
    for (auto const& p : points)
    {
        set_color(p.color);
        glVertex2d(p.position[0], p.position[1]);
    }
    glEnd();

    if (hole_diameter > 0)
    {
        set_color(hole_color);
        glPointSize(hole_diameter);
        glBegin(GL_POINTS);
        for (auto const& p : points)
            glVertex2d(p.position[0], p.position[1]);
        glEnd();
    }
}

// graph_highlight stores information about what point is highlighted on a
// line graph. Since multiple point lists can be involved, it stores not only
// which point is highlighted but also the ID of the point list that contains
// the point.
//
// In order to update a graph_highlight, you should do the following every
// refresh pass.
//
// * Call clear_highlight(highlight) to clear the old highlight.
// * Call update_highlight(canvas, highlight, points) for each highlightable
//   point list.
//
struct graph_highlight
{
    owned_id point_list_id;
    string label;
    data_reporting_parameters y_parameters;
    vector2d point;
    rgba8 color;
};

bool static
is_empty(graph_highlight const& highlight)
{ return !highlight.point_list_id.is_initialized(); }

bool static
operator==(graph_highlight const& a, graph_highlight const& b)
{
    return a.point_list_id == b.point_list_id && a.point == b.point;
}
bool static
operator!=(graph_highlight const& a, graph_highlight const& b)
{
    return !(a == b);
}

void static
clear_highlight(graph_highlight& highlight)
{
    highlight.point_list_id.clear();
}

struct normal_data_point_list
{
    accessor<string> const* label;
    accessor<std::vector<vector2d> > const* points;
    rgba8 color;
};
static inline string const&
get_label(normal_data_point_list const& list, size_t i)
{ return get(*list.label); }
static inline rgba8
get_color(normal_data_point_list const& list, size_t i)
{ return list.color; }

static inline vector2d
get_position(vector2d const& p)
{ return p; }

struct notable_data_point_list
{
    accessor<std::vector<notable_data_point> > const* points;

};
static inline string const&
get_label(notable_data_point_list const& list, size_t i)
{ return get(*list.points)[i].label; }
static inline rgba8
get_color(notable_data_point_list const& list, size_t i)
{ return get(*list.points)[i].color; }

static inline vector2d
get_position(notable_data_point const& p)
{ return p.position; }

template<class Point>
optional<size_t>
find_closest_graph_point(
    embedded_canvas& canvas,
    std::vector<Point> const& points,
    optional<vector2d> const& target,
    double max_distance2)
{
    optional<size_t> closest_point;
    double closest_distance2 = max_distance2;
    size_t n_points = points.size();
    for (size_t i = 0; i != n_points; ++i)
    {
        auto const& p = get_position(points[i]);
        double distance2 = length2(scene_to_canvas(canvas, p) - get(target));
        if (distance2 < closest_distance2)
        {
            closest_point = i;
            closest_distance2 = distance2;
        }
    }
    return closest_point;
}

template<class PointList>
void static
update_highlight(embedded_canvas& canvas, graph_highlight& highlight,
    data_reporting_parameters const& y_parameters, PointList const& point_list)
{
    if (canvas.mouse_position())
    {
        auto const mouse = get(canvas.mouse_position());
        double const selection_radius = 10;
        double const selection_distance2 = selection_radius * selection_radius;
        double const max_distance2 =
            is_empty(highlight) ? selection_distance2 :
            (std::min)(
                length2(mouse - scene_to_canvas(canvas, highlight.point)),
                selection_distance2);
        auto const closer_point_index =
            find_closest_graph_point(canvas, get(*point_list.points), mouse,
                max_distance2);
        if (closer_point_index)
        {
            size_t index = get(closer_point_index);
            highlight.point_list_id.store(point_list.points->id());
            highlight.label = get_label(point_list, index);
            highlight.point = get_position(get(*point_list.points)[index]);
            highlight.color = get_color(point_list, index);
            highlight.y_parameters = y_parameters;
        }
    }
}

struct point_highlighter
{
    graph_highlight highlight;
    value_smoother<uint8_t> highlight_intensity;

    popup_positioning positioning;
    value_smoother<float> popup_intensity;
};

struct line_graph_style_info
{
    rgba8 background_color;
    bool draw_points;
    float point_diameter, highlight_diameter, hole_diameter;
    bool show_rulers, label_axes;
};

void static
read_style_info(dataless_ui_context& ctx, line_graph_style_info* info,
    style_search_path const* path)
{
    info->background_color = get_color_property(path, "background");
    info->draw_points =
        get_property(path, "draw-points", UNINHERITED_PROPERTY, false);
    info->show_rulers =
        get_property(path, "show-rulers", UNINHERITED_PROPERTY, true);
    info->label_axes =
        get_property(path, "label-axes", UNINHERITED_PROPERTY, true);
    info->point_diameter =
        resolve_absolute_length(get_layout_traversal(ctx), 0,
            get_property(path, "point-diameter", UNINHERITED_PROPERTY,
                absolute_length(8, PIXELS)));
    info->hole_diameter =
        resolve_absolute_length(get_layout_traversal(ctx), 0,
            get_property(path, "hole-diameter", UNINHERITED_PROPERTY,
                absolute_length(4, PIXELS)));
    info->highlight_diameter =
        resolve_absolute_length(get_layout_traversal(ctx), 0,
            get_property(path, "highlight-diameter", UNINHERITED_PROPERTY,
                absolute_length(16, PIXELS)));
}

void static
draw_circle(vector2d const& center, float diameter, rgba8 color)
{
    if (diameter > 0)
    {
        set_color(color);
        glPointSize(diameter);
        glBegin(GL_POINTS);
        glVertex2d(center[0], center[1]);
        glEnd();
    }
}

void static
draw_graph_highlight(
    gui_context& ctx,
    embedded_canvas& canvas,
    line_graph_style_info const& style,
    graph_highlight const& highlight,
    accessor<data_reporting_parameters> const& x_parameters)
{
    point_highlighter* data;
    if (get_data(ctx, &data))
    {
        reset_smoothing(data->highlight_intensity, uint8_t(0x00));
        reset_smoothing(data->popup_intensity, 0.f);
    }

    alia_untracked_if (is_refresh_pass(ctx) && !is_empty(highlight))
    {
        if (data->highlight != highlight)
        {
            data->highlight = highlight;
            reset_smoothing(data->highlight_intensity, uint8_t(0x00));
            set_active_overlay(ctx, data);
        }
    }
    alia_end

    alia_untracked_if (is_render_pass(ctx) && !is_empty(highlight))
    {
        {
            scoped_transformation st(ctx);
            canvas.set_canvas_coordinates();
            layout_scalar x = as_layout_size(style.highlight_diameter / 2);
            layout_vector center = layout_vector(
                scene_to_canvas(canvas, highlight.point));
            layout_box box;
            box.corner = center - make_layout_vector(x, x);
            box.size = 2 * make_layout_vector(x, x);
            position_overlay(ctx, data->positioning, box);
        }
    }
    alia_end

    uint8_t highlight_intensity =
        smooth_raw_value(ctx, data->highlight_intensity,
            is_empty(highlight) ? uint8_t(0x00) : uint8_t(0xff),
            animated_transition(default_curve, 250));
    float popup_intensity =
        smooth_raw_value(ctx, data->popup_intensity,
            is_empty(highlight) ? 0.f : 1.f,
            animated_transition(default_curve, 250));

    alia_if (popup_intensity > 0 && is_gettable(x_parameters))
    {
        auto const& p = highlight.point;

        alia_untracked_if (is_render_pass(ctx))
        {
            draw_circle(p, style.highlight_diameter,
                apply_alpha(highlight.color, highlight_intensity / 2));
            draw_circle(p, style.point_diameter,
                apply_alpha(highlight.color, highlight_intensity));
            draw_circle(p, style.hole_diameter,
                apply_alpha(style.background_color, highlight_intensity));
        }
        alia_end

        {
            scoped_transformation st(ctx);
            canvas.set_canvas_coordinates();
            {
                nonmodal_popup popup(ctx, data, data->positioning);
                scoped_surface_opacity scoped_opacity(ctx, popup_intensity);
                panel panel(ctx, text("overlay"));
                do_styled_text(ctx, text("heading"), in(highlight.label));
                grid_layout grid(ctx);
                {
                    grid_row row(grid);
                    do_styled_text(ctx, text("label"),
                        _field(ref(&x_parameters), label), LEFT);
                    char format[12];
                    sprintf(format, "%%8.%dlf",
                        get(x_parameters).digits);
                    do_styled_text(ctx, text("value"),
                        printf(ctx, format, in(p[0])), RIGHT);
                    do_styled_text(ctx, text("units"),
                        _field(ref(&x_parameters), units), LEFT);
                }
                {
                    grid_row row(grid);
                    do_styled_text(ctx, text("label"),
                        in_ptr(&highlight.y_parameters.label), LEFT);
                    char format[12];
                    sprintf(format, "%%8.%dlf",
                        highlight.y_parameters.digits);
                    do_styled_text(ctx, text("value"),
                        printf(ctx, format, in(p[1])), RIGHT);
                    do_styled_text(ctx, text("units"),
                        in_ptr(&highlight.y_parameters.units), LEFT);
                }
            }
        }
    }
    alia_end
}

void line_graph::begin(
    gui_context& ctx, box<2,double> const& scene_box,
    accessor<data_reporting_parameters> const& x_parameters,
    accessor<data_reporting_parameters> const& default_y_parameters,
    accessor<string> const& style,
    layout const& layout_spec,
    optional_storage<cradle::camera> const& camera)
{
    get_cached_style_info(ctx, &style_, style);

    substyle_.begin(ctx, style);

    auto nested_layout_spec = layout_spec;

    alia_if (style_->label_axes)
    {
        labels_.begin(ctx,
            printf(ctx, "%s (%s)", _field(ref(&x_parameters), label),
                _field(ref(&x_parameters), units)),
            printf(ctx, "%s (%s)", _field(ref(&default_y_parameters), label),
                _field(ref(&default_y_parameters), units)),
            nested_layout_spec);
        nested_layout_spec = GROW | UNPADDED;
    }
    alia_end

    x_parameters_ = make_persistent_copy(ctx, x_parameters);

    canvas_.initialize(ctx, scene_box, base_zoom_type::STRETCH_TO_FIT,
        camera, CANVAS_FLIP_Y);

    alia_if (style_->show_rulers)
    {
        rulers_.begin(ctx, canvas_, LEFT_RULER | BOTTOM_RULER,
            nested_layout_spec);
        nested_layout_spec = GROW | UNPADDED;
    }
    alia_end

    canvas_.begin(nested_layout_spec);

    auto const& bg = style_->background_color;
    clear_canvas(canvas_, rgb8(bg.r, bg.g, bg.b));

    get_cached_data(ctx, &highlight_);
    if (is_refresh_pass(ctx))
        clear_highlight(*highlight_);

    active_ = true;
}
void line_graph::end()
{
    if (active_)
    {
        canvas_.end();
        rulers_.end();
        labels_.end();
        substyle_.end();
        active_ = false;
    }
}

void read_style_info(dataless_ui_context& ctx, graph_line_style_info* info,
    style_search_path const* path)
{
    info->color = get_color_property(path, "color");
}

void line_graph::do_line(gui_context& ctx,
    accessor<string> const& label, accessor<string> const& line_style,
    accessor<data_reporting_parameters> const& y_axis_parameters,
    accessor<std::vector<vector2d> > const& points)
{
    graph_line_style_info const* line_style_info;
    get_cached_style_info(ctx, &line_style_info, line_style);

    do_line(label, *line_style_info, y_axis_parameters, points);
}

void line_graph::do_line(
    accessor<string> const& label,
    graph_line_style_info const& line_style_info,
    accessor<data_reporting_parameters> const& y_axis_parameters,
    accessor<std::vector<vector2d> > const& points)
{
    dataless_ui_context& ctx = *ctx_;
    embedded_canvas& c = canvas_;

    alia_untracked_if (is_gettable(y_axis_parameters) && is_gettable(points))
    {
        alia_untracked_if (is_refresh_pass(ctx))
        {
            normal_data_point_list list;
            list.label = &label;
            list.color = line_style_info.color;
            list.points = &points;
            update_highlight(c, *highlight_, get(y_axis_parameters), list);
        }
        alia_untracked_else_if (is_render_pass(ctx))
        {
            draw_line_graph(ctx,
                line_style_info.color,
                line_style(2, solid_line), get(points));
            alia_untracked_if (style_->draw_points)
            {
                draw_graph_points(ctx,
                    line_style_info.color,
                    style_->point_diameter,
                    style_->background_color,
                    style_->hole_diameter,
                    get(points));
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

void line_graph::do_line(gui_context& ctx,
    accessor<string> const& label, accessor<string> const& style,
    accessor<data_reporting_parameters> const& y_axis_parameters,
    accessor<std::vector<notable_data_point> > const& points)
{
    graph_line_style_info const* line_style_info;
    get_cached_style_info(ctx, &line_style_info, style);

    do_line(label, *line_style_info, y_axis_parameters, points);
}

void line_graph::do_line(
    accessor<string> const& label,
    graph_line_style_info const& line_style_info,
    accessor<data_reporting_parameters> const& y_axis_parameters,
    accessor<std::vector<notable_data_point> > const& points)
{
    dataless_ui_context& ctx = *ctx_;
    embedded_canvas& c = canvas_;

    alia_untracked_if (is_gettable(y_axis_parameters) && is_gettable(points))
    {
        alia_untracked_if (is_refresh_pass(ctx))
        {
            notable_data_point_list list;
            list.points = &points;
            update_highlight(c, *highlight_, get(y_axis_parameters), list);
        }
        alia_untracked_else_if (is_render_pass(ctx))
        {
            draw_line_graph(ctx,
                line_style_info.color,
                line_style(2, solid_line), get(points));
            alia_untracked_if (style_->draw_points)
            {
                draw_graph_points(ctx,
                    style_->point_diameter,
                    style_->background_color,
                    style_->hole_diameter,
                    get(points));
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

void line_graph::do_highlight(gui_context& ctx)
{
    embedded_canvas& c = canvas_;

    draw_graph_highlight(ctx, c, *style_, *highlight_,
        make_accessor(x_parameters_));
}

struct plot_image_fn
{
    template<class T>
    void operator()(image<1,T,shared> const& img)
    {
        auto image_grid = get_grid(img);
        auto grid_points = make_grid_point_list(image_grid);
        auto pixel = get_begin(img);
        glBegin(GL_LINE_STRIP);
        for (auto const& p : grid_points)
        {
            glVertex2d(p[0], apply(img.value_mapping, double(*pixel)));
            ++pixel;
        }
        glEnd();
    }
};

void plot_image(gui_context& ctx, image_interface_1d const& img,
    accessor<rgba8> const& color, accessor<line_style> const& style)
{
    alia_if (is_gettable(color) && is_gettable(style))
    {
        auto regular = img.get_regularly_spaced_image(ctx);
        alia_if (is_gettable(regular))
        {
            set_color(get(color));
            set_line_style(get(style));
            plot_image_fn fn;
            apply_fn_to_gray_variant(fn, get(regular));
        }
        alia_end
    }
    alia_end
}

struct extract_image_points_fn
{
    std::vector<vector2d> points;
    template<class T>
    void operator()(image<1,T,shared> const& img)
    {
        points.clear();
        points.reserve(img.size[0]);
        auto image_grid = get_grid(img);
        auto grid_points = make_grid_point_list(image_grid);
        auto pixel = get_begin(img);
        for (auto const& p : grid_points)
        {
            points.push_back(
                make_vector(p[0], apply(img.value_mapping, double(*pixel))));
            ++pixel;
        }
    }
};

std::vector<vector2d>
extract_image_points(image<1,variant,shared> const& img)
{
    extract_image_points_fn fn;
    apply_fn_to_gray_variant(fn, img);
    return fn.points;
}

indirect_accessor<std::vector<vector2d> >
make_image_plottable(gui_context& ctx, image_interface_1d const& img)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            extract_image_points,
            img.get_regularly_spaced_image(ctx)));
}

}
