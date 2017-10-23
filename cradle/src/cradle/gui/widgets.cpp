#include <cradle/gui/widgets.hpp>

#include <cmath>
#include <stdio.h>
#include <string.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>

#include <SkGradientShader.h>
#include <SkPath.h>

#include <alia/ui/library/controls.hpp>
#include <alia/ui/library/panels.hpp>

#include <cradle/math/common.hpp>

namespace cradle {

// PANEL EXPANDERS

struct bottom_expander
{
};
double static inline
get_arrow_rotation(bottom_expander _, double expansion)
{
    return expansion * 60 + 90;
}
void static inline
recenter_triangle(bottom_expander _, box_control_renderer& renderer,
    double expansion)
{
    renderer.canvas().translate(
        SkIntToScalar(0),
        SkDoubleToScalar(
            (expansion - 0.5) * 0.17 * renderer.content_region().size[1]));
}

struct top_expander
{
};
double static inline
get_arrow_rotation(top_expander _, double expansion)
{
    return expansion * -60 + 150;
}
void static inline
recenter_triangle(top_expander _, box_control_renderer& renderer,
    double expansion)
{
    renderer.canvas().translate(
        SkIntToScalar(0),
        SkDoubleToScalar(
            (expansion - 0.5) * -0.17 * renderer.content_region().size[1]));
}

struct right_expander
{
};
double static inline
get_arrow_rotation(right_expander _, double expansion)
{
    return expansion * 60;
}
void static inline
recenter_triangle(right_expander _, box_control_renderer& renderer,
    double expansion)
{
    renderer.canvas().translate(
        SkDoubleToScalar(
            (expansion - 0.5) * 0.17 * renderer.content_region().size[0]),
        SkIntToScalar(0));
}

struct left_expander
{
};
double static inline
get_arrow_rotation(left_expander _, double expansion)
{
    return (1 - expansion) * 60;
}
void static inline
recenter_triangle(left_expander _, box_control_renderer& renderer,
    double expansion)
{
    renderer.canvas().translate(
        SkDoubleToScalar(
            (expansion - 0.5) * -0.17 * renderer.content_region().size[0]),
        SkIntToScalar(0));
}

template<class Direction>
struct panel_expander_renderer : simple_control_renderer<bool>
{};

template<class Direction>
struct default_panel_expander_renderer : panel_expander_renderer<Direction>
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "panel-expander");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<bool> const& expanded, widget_state state) const
    {
        double smoothed_expansion =
            smooth_raw_value(ctx,
                is_gettable(expanded) && get(expanded) ? 0. : 1.,
                animated_transition(linear_curve, 200));

        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(make_id(smoothed_expansion), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache,
                "panel-expander", state);

            renderer.canvas().translate(
                renderer.content_region().size[0] / SkIntToScalar(2),
                renderer.content_region().size[1] / SkIntToScalar(2));

            recenter_triangle(Direction(), renderer, smoothed_expansion);

            renderer.canvas().rotate(
                float(get_arrow_rotation(Direction(), smoothed_expansion)));

            {
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, renderer.style().fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    (std::min)(
                        renderer.content_region().size[0],
                        renderer.content_region().size[1]) /
                    SkDoubleToScalar(1.5);
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = SkScalarMul(a, SkDoubleToScalar(-0.34));
                p0.fY = SkScalarMul(a, SkDoubleToScalar(-0.5));
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = SkScalarMul(a, SkDoubleToScalar(0.5));
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + SkScalarMul(a, SkDoubleToScalar(0.866));
                p2.fY = 0;
                path.lineTo(p2);
                path.lineTo(p0);
                renderer.canvas().drawPath(path, paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

template<class Direction>
panel_expander_result
do_unsafe_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    panel_expander_result result;
    if (do_simple_control<panel_expander_renderer<Direction>,
            default_panel_expander_renderer<Direction> >(
            ctx, expanded, layout_spec, NO_FLAGS, id))
    {
        result.changed = true;
        set(expanded, expanded.is_gettable() ? !expanded.get() : true);
    }
    else
        result.changed = false;
    return result;
}

template<class Direction>
void do_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    if (do_unsafe_panel_expander<Direction>(ctx, expanded, layout_spec, id))
        end_pass(ctx);
}

void do_bottom_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    do_panel_expander<bottom_expander>(ctx, expanded, layout_spec, id);
}

void do_top_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    do_panel_expander<top_expander>(ctx, expanded, layout_spec, id);
}

void do_right_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    do_panel_expander<right_expander>(ctx, expanded, layout_spec, id);
}

void do_left_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec,
    widget_id id)
{
    do_panel_expander<left_expander>(ctx, expanded, layout_spec, id);
}

// ENUMS

string
enum_as_string(string const& name)
{
    string acronyms[] = {"oar", "poi", "sfo", "impt", "advanced"};
    string str = name;

    if (name == "gyrbe")
    {
        return "Absolute";
    }

    if (name == "percent")
    {
        return "Relative";
    }

    if(name == "constant")
        return "Constant (mm)";

    for(auto const& a : acronyms)
    {
        if(name == a)
            return boost::to_upper_copy(str);
    }

    size_t f = str.find("_");
    str.replace(0, 1, boost::to_upper_copy(str.substr(0, 1)));

    if(f != string::npos)
    {
        str.replace(f, 1, " ");
        if(str.length() - 1 > f)
            str.replace(f+1, 1, boost::to_upper_copy(str.substr(f+1, 1)));
    }

    return str;
}

enum_drop_down_result
do_unsafe_enum_drop_down(ui_context& ctx, raw_enum_info const& type_info,
    accessor<unsigned> const& value, layout const& layout_spec,
    ddl_flag_set flags)
{
    drop_down_list<unsigned> ddl(ctx, value, layout_spec, flags);
    do_text(ctx,
        in(
            is_gettable(value) &&
                get(value) < unsigned(type_info.values.size()) ?
            enum_as_string(type_info.values[get(value)].name) :
           string()));
    alia_if (ddl.do_list())
    {
        alia_for (size_t i = 0; i != type_info.values.size(); ++i)
        {
            auto const& value = type_info.values[i];
            ddl_item<unsigned> item(ddl, unsigned(i));
            do_text(ctx, in(enum_as_string(value.name)));
        }
        alia_end
    }
    alia_end

    enum_drop_down_result result;
    result.changed = ddl.changed();
    return result;
}

// COLOR CONTROL

size_t static const n_standard_colors = 12;
rgb8 static const
standard_colors[n_standard_colors] = {
    rgb8(0xff, 0x80, 0x01),
    rgb8(0xff, 0xff, 0x01),
    rgb8(0x80, 0xff, 0x01),
    rgb8(0x01, 0xff, 0x01),
    rgb8(0x01, 0xff, 0x80),
    rgb8(0x01, 0xff, 0xff),
    rgb8(0x01, 0x80, 0xff),
    rgb8(0x01, 0x01, 0xff),
    rgb8(0x80, 0x01, 0xff),
    rgb8(0xff, 0x01, 0xff),
    rgb8(0xff, 0x01, 0x80),
    rgb8(0xff, 0x01, 0x01) };

/// \fn control_result do_unsafe_color_drop_down(ui_context& ctx, accessor<rgb8> const& color, layout const& layout_spec)
/// \ingroup gui
/// Color selection from a drop down list
control_result
do_unsafe_color_drop_down(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec)
{
    drop_down_list<rgb8> ddl(ctx, color,
        add_default_size(layout_spec, width(4, CHARS)));
    do_color(ctx, color);
    alia_if (ddl.do_list())
    {
        alia_for (size_t i = 0; i != n_standard_colors; ++i)
        {
            auto const& value = standard_colors[i];
            ddl_item<rgb8> item(ddl, value);
            do_color(ctx, in(value));
        }
        alia_end
    }
    alia_end

    enum_drop_down_result result;
    result.changed = ddl.changed();
    return result;
}

void static
do_selectable_color(ui_context& ctx, accessor<rgb8> const& selected_color,
    rgb8 const& this_color, enum_drop_down_result& result)
{
    auto is_selected = is_equal(selected_color, this_color);
    clickable_panel panel(
        ctx, text("color-chooser"), default_layout,
        is_selected ? PANEL_SELECTED : NO_FLAGS);
    if (panel.clicked())
    {
        selected_color.set(this_color);
        result.changed = true;
    }
    do_color(ctx, in(this_color));
}

control_result
do_unsafe_color_control(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec)
{
    flow_layout flow(ctx);

    enum_drop_down_result result;
    result.changed = false;
    scoped_substyle style(ctx, text("color-control"));
    for (size_t i = 0; i != n_standard_colors; ++i)
        do_selectable_color(ctx, color, standard_colors[i], result);

    return result;
}

std::vector<rgb8>
get_selectable_color_list()
{
    std::vector<rgb8> colors;
    colors.reserve(n_standard_colors);
    for (size_t i = 0; i != n_standard_colors; ++i)
        colors.push_back(standard_colors[i]);
    return colors;
}

// TRISTATE EXPANDER

struct tristate_expander_renderer : simple_control_renderer<tristate_expansion>
{};

struct default_tristate_expander_renderer : tristate_expander_renderer
{
    leaf_layout_requirements get_layout(ui_context& ctx) const
    {
        return get_box_control_layout(ctx, "node-expander");
    }
    void draw(
        ui_context& ctx, layout_box const& region,
        accessor<tristate_expansion> const& value, widget_state state) const
    {
        double raw_angle = 0;
        if (is_gettable(value))
        {
            switch (get(value))
            {
             case tristate_expansion::CLOSED:
                raw_angle = 0.;
                break;
             case tristate_expansion::OPEN:
                raw_angle = 90.;
                break;
             case tristate_expansion::HALFWAY:
                raw_angle = 45.;
                break;
            }
        }
        double angle = smooth_raw_value(ctx, raw_angle,
            animated_transition(linear_curve, 200));

        if (!is_render_pass(ctx))
            return;

        caching_renderer cache;
        initialize_caching_control_renderer(ctx, cache, region,
            combine_ids(make_id(angle), make_id(state)));
        if (cache.needs_rendering())
        {
            box_control_renderer renderer(ctx, cache, "node-expander", state);

            renderer.canvas().translate(
                renderer.content_region().size[0] / SkIntToScalar(2),
                renderer.content_region().size[1] / SkIntToScalar(2));
            renderer.canvas().rotate(float(angle));

            {
                SkPaint paint;
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                set_color(paint, renderer.style().fg_color);
                paint.setStyle(SkPaint::kFill_Style);
                SkScalar a =
                    renderer.content_region().size[0] / SkIntToScalar(2);
                SkPath path;
                path.incReserve(4);
                SkPoint p0;
                p0.fX = SkScalarMul(a, SkDoubleToScalar(-0.34));
                p0.fY = SkScalarMul(a, SkDoubleToScalar(-0.5));
                path.moveTo(p0);
                SkPoint p1;
                p1.fX = p0.fX;
                p1.fY = SkScalarMul(a, SkDoubleToScalar(0.5));
                path.lineTo(p1);
                SkPoint p2;
                p2.fX = p0.fX + SkScalarMul(a, SkDoubleToScalar(0.866));
                p2.fY = 0;
                path.lineTo(p2);
                path.lineTo(p0);
                renderer.canvas().drawPath(path, paint);
            }

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }
};

tristate_expander_result
do_unsafe_tristate_expander(
    ui_context& ctx,
    accessor<tristate_expansion> const& value,
    layout const& layout_spec,
    widget_id id)
{
    node_expander_result result;
    if (do_simple_control<tristate_expander_renderer,
            default_tristate_expander_renderer>(ctx, value, layout_spec,
                NO_FLAGS, id))
    {
        result.changed = true;
        set(value,
            is_gettable(value) && get(value) != tristate_expansion::OPEN ?
            tristate_expansion::OPEN : tristate_expansion::CLOSED);
    }
    else
        result.changed = false;
    return result;
}

// TRISTATE TREE NODE

struct tristate_tree_node_data
{
    tristate_expansion expanded;
};

void tristate_tree_node::begin(
    ui_context& ctx,
    layout const& layout_spec,
    tristate_tree_node_flag_set flags,
    optional_storage<tristate_expansion> const& expanded,
    widget_id expander_id)
{
    ctx_ = &ctx;

    tristate_tree_node_data* data;
    if (get_data(ctx, &data))
    {
        if (flags & TRISTATE_TREE_NODE_INITIALLY_EXPANDED)
            data->expanded = tristate_expansion::OPEN;
        else
            data->expanded = tristate_expansion::CLOSED;
    }

    auto state = resolve_storage(expanded, &data->expanded);

    grid_.begin(ctx, layout_spec);
    row_.begin(grid_);

    is_expanded_ =
        is_gettable(state) && get(state) != tristate_expansion::CLOSED;
    get_widget_id_if_needed(ctx, expander_id);
    expander_result_ =
        do_unsafe_tristate_expander(ctx, state, default_layout, expander_id);

    label_region_.begin(ctx, BASELINE_Y | GROW_X);
    if (!(flags & TRISTATE_TREE_MINIMAL_HIT_TESTING))
        hit_test_box_region(ctx, expander_id, label_region_.region());
}

bool tristate_tree_node::do_children()
{
    ui_context& ctx = *ctx_;
    label_region_.end();
    row_.end();
    content_.begin(ctx, is_expanded_);
    bool do_content = content_.do_content();
    alia_if (do_content)
    {
        row_.begin(grid_, layout(GROW));
        do_spacer(ctx);
        column_.begin(ctx, layout(GROW));
    }
    alia_end
    return do_content;
}

void tristate_tree_node::end()
{
    column_.end();
    row_.end();
    content_.end();
    grid_.end();
}

// animated astroid

struct animated_astroid_style_info
{
    int period, dark_time;
    int n_segments;
    rgba8 color;
    float max_alpha;
    float stroke_width, stroke_length;
};

void static
read_style_info(dataless_ui_context& ctx, animated_astroid_style_info* info,
    style_search_path const* path)
{
    info->stroke_width =
        get_property(path, "stroke-width", UNINHERITED_PROPERTY, 0.4f);
    info->stroke_length =
        get_property(path, "stroke-length", UNINHERITED_PROPERTY, 0.6f);
    info->period =
        get_property(path, "period", UNINHERITED_PROPERTY, 2000);
    info->dark_time =
        get_property(path, "dark-time", UNINHERITED_PROPERTY, 700);
    info->n_segments =
        get_property(path, "segment-count", UNINHERITED_PROPERTY, 16);
    info->color =
        get_color_property(path, "color");
    info->max_alpha =
        get_property(path, "max-alpha", UNINHERITED_PROPERTY, 0.6f);
}

static void
draw_astroid_segment(SkCanvas& canvas, SkPaint& paint, rgba8 const& color,
    float stroke_length, double t)
{
    set_color(paint, color);

    // parametric equation for an astroid
    double c = std::cos(t);
    double s = std::sin(t);
    auto p = make_vector(c * c * c, s * s * s);

    // derivative of the above
    auto d = make_vector(-3 * c * c * s, 3 * s * s * c);

    // At the vertices, the derivative is (0, 0), so this fixes it.
    if (length2(d) < 0.001)
        d = make_vector(c, s);

    d = unit(d) * (stroke_length * 0.5);
    auto p0 = p - d;
    auto p1 = p + d;
    canvas.drawLine(
        SkDoubleToScalar(p0[0]), SkDoubleToScalar(p0[1]),
        SkDoubleToScalar(p1[0]), SkDoubleToScalar(p1[1]),
        paint);
}

void
do_animated_astroid(gui_context& ctx,
    layout const& layout_spec, optional<string> const& tooltip)
{
    auto id = get_widget_id(ctx);
    panel p(ctx, text("transparent"), default_layout, NO_FLAGS, id);
    ALIA_GET_CACHED_DATA(simple_display_data)

    animated_astroid_style_info const* style;
    get_cached_style_info(ctx, &style, text("animated-astroid"));

    ui_time_type ticks = get_animation_tick_count(ctx);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx),
            add_default_size(layout_spec, size(4.f, 4.f, EM)),
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0),
            CENTER | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     case RENDER_CATEGORY:
      {
        layout_box const& region = data.layout_node.assignment().region;
        caching_renderer cache(ctx, data.rendering, make_id(ticks), region);
        if (cache.needs_rendering())
        {
            skia_renderer sr(ctx, cache.image(), region.size);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);
            paint.setStyle(SkPaint::kFill_Style);

            SkScalar scale =
                layout_scalar_as_skia_scalar(
                    (std::min)(region.size[0], region.size[1])) /
                SkFloatToScalar(3.f);
            sr.canvas().translate(
                layout_scalar_as_skia_scalar(region.size[0]) /
                    SkIntToScalar(2),
                layout_scalar_as_skia_scalar(region.size[1]) /
                    SkIntToScalar(2));
            sr.canvas().scale(scale, scale);

            paint.setStrokeCap(SkPaint::kRound_Cap);
            paint.setStrokeWidth(SkFloatToScalar(style->stroke_width));
            for (int i = 0; i != style->n_segments; ++i)
            {
                double alpha =
                    style->max_alpha *
                    (nonnegative_mod<int64_t>(
                        int64_t(i * style->period / style->n_segments) - ticks,
                        style->period) -
                     style->dark_time) /
                    (style->period - style->dark_time);
                // Don't waste time on fully transparent segments.
                if (alpha <= 0)
                    continue;

                double t = (2 * cradle::pi) * i / style->n_segments;

                rgba8 const color = style->color;
                draw_astroid_segment(sr.canvas(), paint,
                    rgba8(color.r, color.g, color.b,
                        uint8_t(0xff * alpha + 0.5)),
                    style->stroke_length, t);
            }

            // Draw the leading segment.
            draw_astroid_segment(sr.canvas(), paint, style->color,
                style->stroke_length,
                (2 * cradle::pi) * ticks / style->period);

            sr.cache();
            cache.mark_valid();
        }
        cache.draw();
        break;
      }
    }
    alia_if (tooltip)
    {
        set_tooltip_message(ctx, id, in(get(tooltip)));
    }
    alia_end
}

// BLENDED BACKGROUND PANEL

struct cell_style_info
{
    panel_style_info panel_info;
    substyle_data substyle;
};

void static
get_cell_style_info(dataless_ui_context& ctx, cell_style_info* info,
    style_search_path const* path, string const& name, widget_state state)
{
    update_substyle_data(ctx, info->substyle, path, name,
        state, ADD_SUBSTYLE_IFF_EXISTS);
    info->panel_info = get_panel_style_info(ctx, info->substyle.state.path);
}

void static
refresh_cell_style_info(
    dataless_ui_context& ctx,
    keyed_data<cell_style_info>& style_data,
    accessor<string> const& style,
    widget_state state,
    accessor<rgba8> const& background_color,
    double blend_factor)
{
    if (is_refresh_pass(ctx))
    {
        refresh_keyed_data(style_data,
            combine_ids(
                combine_ids(ref(ctx.style.id),
                    combine_ids(ref(&style.id()), make_id(state))),
                ref(&background_color.id())));
    }
    if (!is_valid(style_data) && is_gettable(style) &&
        is_gettable(background_color))
    {
        get_cell_style_info(ctx, &style_data.value, ctx.style.path,
            get(style), state);
        style_data.value.panel_info.background_color =
            style_data.value.substyle.properties.background_color =
            interpolate(style_data.value.panel_info.background_color,
                get(background_color), blend_factor);
        mark_valid(style_data);
    }
}

struct blended_background_panel_data
{
    custom_panel_data panel;
    keyed_data<cell_style_info> style;
};

void blended_background_panel::begin(
    ui_context& ctx,
    accessor<string> const& style,
    accessor<rgba8> const& background_color,
    double blend_factor,
    layout const& layout_spec,
    panel_flag_set flags,
    widget_id id,
    widget_state state)
{
    blended_background_panel_data* data_;
    get_data(ctx, &data_);
    refresh_cell_style_info(ctx, data_->style, style, state,
        add_fallback_value(ref(&background_color), in(rgba8(0, 0, 0, 0))),
        blend_factor);
    panel_.begin(ctx, data_->panel,
        make_custom_getter(&get(data_->style).panel_info,
            ref(get(data_->style).substyle.state.id)),
        layout_spec, flags, id, state);
    substyle_.begin(ctx,
        get(data_->style).substyle.state,
        &get(data_->style).substyle.style_info);
}
void blended_background_panel::end()
{
    substyle_.end();
    panel_.end();
}

indirect_accessor<string>
as_text(gui_context& ctx, accessor<cradle::date> const& date)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            static_cast<string(*)(cradle::date const& d)>(&to_string),
            date));
}

void do_date(gui_context& ctx, accessor<cradle::date> const& date)
{
    do_text(ctx,
        gui_apply(ctx,
            static_cast<string(*)(cradle::date const& d)>(&to_string),
            date));
}

string static
optional_date_to_string(optional<cradle::date> const& date)
{
    return date ? to_string(get(date)) : "none";
}

indirect_accessor<string>
as_text(gui_context& ctx, accessor<optional<cradle::date> > const& date)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            optional_date_to_string,
            date));
}

void do_date(gui_context& ctx, accessor<optional<cradle::date> > const& date)
{
    do_text(ctx,
        gui_apply(ctx,
            optional_date_to_string,
            date));
}

indirect_accessor<string>
as_text(gui_context& ctx, accessor<cradle::time> const& time)
{
    return make_indirect(ctx,
        gui_apply(ctx, to_local_string, time));
}

void do_time(gui_context& ctx, accessor<cradle::time> const& time)
{
    do_text(ctx, gui_apply(ctx, to_local_string, time));
}

string static
optional_time_to_local_string(optional<cradle::time> const& date)
{
    return date ? to_local_string(get(date)) : "none";
}

indirect_accessor<string>
as_text(gui_context& ctx, accessor<optional<cradle::time> > const& time)
{
    return make_indirect(ctx,
        gui_apply(ctx, optional_time_to_local_string, time));
}

void do_time(gui_context& ctx, accessor<optional<cradle::time> > const& time)
{
    do_text(ctx, gui_apply(ctx, optional_time_to_local_string, time));
}

// SVG graphic

struct nsvg_image_raii
{
    nsvg_image_raii() : image(0) {}
    nsvg_image_raii(NSVGimage* image) : image(image) {}
    ~nsvg_image_raii() { if (image) nsvgDelete(image); }
    NSVGimage* image;
};

enum class svg_scale_mode
{
    FIT,
    FILL
};

enum class svg_alignment
{
    LEFT,
    RIGHT,
    CENTER
};

struct svg_colors
{
    rgba8 top, bottom;
};

struct svg_graphic_style_info
{
    optional<svg_colors> colors;
    layout_vector size;
    svg_scale_mode scaling;
    svg_alignment alignment;
};

struct svg_graphic_data
{
    layout_leaf layout_node;
    keyed_data<layout_vector> size;
    caching_renderer_data rendering;
};

void static
read_style_info(dataless_ui_context& ctx, svg_graphic_style_info* info,
    style_search_path const* path)
{
    svg_colors colors;
    colors.top =
        get_property(path, "color", UNINHERITED_PROPERTY, rgba8(0, 0, 0, 0));
    colors.bottom =
        get_property(path, "gradient", UNINHERITED_PROPERTY, colors.top);
    info->colors = colors.top != rgba8(0, 0, 0, 0) ? some(colors) : none;
    auto width =
        get_property(path, "width", UNINHERITED_PROPERTY,
            absolute_length(0, EM));
    auto height =
        get_property(path, "height", UNINHERITED_PROPERTY,
            absolute_length(0, EM));
    info->size =
        as_layout_size(
            resolve_absolute_size(get_layout_traversal(ctx),
                get_property(path, "size", UNINHERITED_PROPERTY,
                    make_vector(width, height))));
    auto scale_mode =
        get_property(path, "scale", UNINHERITED_PROPERTY, string("fit"));
    if (scale_mode == "fill")
        info->scaling = svg_scale_mode::FILL;
    else
        info->scaling = svg_scale_mode::FIT;
    auto alignment =
        get_property(path, "alignment", UNINHERITED_PROPERTY, string("center"));
    if (alignment == "center")
        info->alignment = svg_alignment::CENTER;
    else if (alignment == "left")
        info->alignment = svg_alignment::LEFT;
    else
        info->alignment = svg_alignment::RIGHT;
}

rgba8 static
rgba8_from_nsvg_color(int color)
{
    return
        rgba8(
            color & 0xff,
            (color >> 8) & 0xff,
            (color >> 16) & 0xff,
            (color >> 24) & 0xff);
}

void static
apply_nsvg_paint(SkPaint& skia_paint, NSVGpaint const& nsvg_paint)
{
    switch (nsvg_paint.type)
    {
     case NSVG_PAINT_COLOR:
        skia_paint.setColor(as_skia_color(rgba8_from_nsvg_color(nsvg_paint.color)));
        break;
     default:
        skia_paint.setColor(SK_ColorWHITE);
    }
}

void static
render_svg_graphic(dataless_ui_context& ctx, caching_renderer& cache,
    svg_graphic_style_info const& style, layout_box const& region,
    string const& svg)
{
    // Load the SVG.
    auto svg_copy = svg; // nsvgParse changes the string
    NSVGimage* image = nsvgParse(&svg_copy[0], "px", 96);
    nsvg_image_raii image_raii(image);

    skia_renderer sr(ctx, cache.image(), region.size);

    if (image)
    {
        SkPaint paint;
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        set_color(paint, rgba8(0xff, 0xff, 0xff, 0xff));

        // Apply scaling.
        double scale_factor;
        if (style.scaling == svg_scale_mode::FILL)
        {
            scale_factor =
                (std::max)(
                    double(region.size[0]) / image->width,
                    double(region.size[1]) / image->height);
        }
        else
        {
            scale_factor =
                (std::min)(
                    double(region.size[0]) / image->width,
                    double(region.size[1]) / image->height);
        }
        sr.canvas().scale(
            SkDoubleToScalar(scale_factor),
            SkDoubleToScalar(scale_factor));

        // Apply alignment.
        double extra_width = region.size[0] / scale_factor - image->width;
        double x_offset;
        switch (style.alignment)
        {
         case svg_alignment::LEFT:
            x_offset = 0;
            break;
         case svg_alignment::CENTER:
            x_offset = extra_width / 2;
            break;
         case svg_alignment::RIGHT:
         default:
            x_offset = extra_width;
            break;
        }
        // There's no Y alignment parameter. The graphic is always centered.
        double extra_height = region.size[1] / scale_factor - image->height;
        double y_offset = extra_height / 2;
        sr.canvas().translate(
            SkDoubleToScalar(x_offset),
            SkDoubleToScalar(y_offset));

        // If the style specifies colors, set up a shader for doing a vertical gradient
        // with those colors.
        if (style.colors)
        {
            SkPoint gradient_points[] =
                {
                    {
                        SkDoubleToScalar(0.),
                        SkDoubleToScalar(0.)
                    },
                    {
                        SkDoubleToScalar(0.),
                        SkDoubleToScalar(image->height)
                    }
                };
            SkColor gradient_colors[] =
                {
                    as_skia_color(get(style.colors).top),
                    as_skia_color(get(style.colors).bottom),
                };
            paint.setShader(
                SkGradientShader::MakeLinear(
                    gradient_points, gradient_colors, NULL, 2,
                    SkShader::kClamp_TileMode));
        }

        // Render the image paths.
        for (auto const* shape = image->shapes; shape; shape = shape->next)
        {
            SkPath sk_path;
            for (auto const* path = shape->paths; path; path = path->next)
            {
                float* p = &path->pts[0];
                sk_path.moveTo(p[0], p[1]);
                for (int i = 0; i < path->npts - 1; i += 3)
                {
                    float* p = &path->pts[i*2];
                    sk_path.cubicTo(p[2], p[3], p[4], p[5], p[6], p[7]);
                }
            }
            // Some very simple logic to handle simple stroking or filling.
            if (shape->fill.type != NSVG_PAINT_NONE)
            {
                paint.setStyle(SkPaint::kFill_Style);
                if (!style.colors)
                {
                    apply_nsvg_paint(paint, shape->fill);
                }
                sr.canvas().drawPath(sk_path, paint);
            }
            if (shape->stroke.type != NSVG_PAINT_NONE)
            {
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setStrokeWidth(shape->strokeWidth);
                if (!style.colors)
                {
                    apply_nsvg_paint(paint, shape->stroke);
                }
                sr.canvas().drawPath(sk_path, paint);
            }
        }
    }

    sr.cache();
    cache.mark_valid();
}

layout_vector static
get_svg_size(svg_graphic_style_info const& style, string const& svg)
{
    // Load the SVG to get its size.
    auto svg_copy = svg; // nsvgParse changes the string
    NSVGimage* image = nsvgParse(&svg_copy[0], "px", 96);
    nsvg_image_raii image_raii(image);
    if (image)
    {
        // If only the width or the height is specified, set the
        // other using the original aspect ratio of the image.
        auto aspect_ratio = image->width / image->height;
        if (style.size[0] == 0 && style.size[1] != 0)
        {
            return
                make_vector(
                    as_layout_size(style.size[1] * aspect_ratio),
                    style.size[1]);
        }
        else if (style.size[0] != 0 && style.size[1] == 0)
        {
            return
                make_vector(
                    style.size[0],
                    as_layout_size(style.size[0] / aspect_ratio));
        }
        else
            return style.size;
    }
    else
    {
        return make_layout_vector(0, 0);
    }
}

void do_svg_graphic(gui_context& ctx, accessor<string> const& style_name,
    accessor<string> const& svg, layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(svg_graphic_data)

    svg_graphic_style_info const* style;
    get_cached_style_info(ctx, &style, style_name);

    alia_untracked_if (is_gettable(svg))
    {
        auto graphic_id =
            combine_ids(
                combine_ids(ref(&svg.id()), ref(&style_name.id())),
                ref(ctx.style.id));

        switch (ctx.event->category)
        {
         case REFRESH_CATEGORY:
          {
            refresh_keyed_data(data.size, graphic_id);
            if (!is_valid(data.size))
                set(data.size, get_svg_size(*style, get(svg)));
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                // Note that we're assuming here that the entire graphic should
                // go above the baseline. This just happens to be true for the
                // Astroid logo.
                leaf_layout_requirements(get(data.size), get(data.size)[1], 0),
                FILL | PADDED);
            add_layout_node(get_layout_traversal(ctx), &data.layout_node);
            break;
          }

         case RENDER_CATEGORY:
          {
            layout_box const& region = data.layout_node.assignment().region;
            caching_renderer cache(ctx, data.rendering, graphic_id, region);
            if (cache.needs_rendering())
                render_svg_graphic(ctx, cache, *style, region, get(svg));
            cache.draw();
            break;
          }
        }
    }
    alia_end
}

void do_app_logo(gui_context& ctx, accessor<string> const& svg, layout const& layout_spec)
{
    do_svg_graphic(ctx, text("logo"), svg, layout_spec);
}

}
