#ifndef CRADLE_GUI_WIDGETS_HPP
#define CRADLE_GUI_WIDGETS_HPP



#include <cradle/gui/common.hpp>
#include <cradle/date_time.hpp>

// This file builds on the alia library of widgets by defining various widgets
// that are used throughout astroid.

namespace cradle {

// panel expander button

typedef control_result panel_expander_result;

void do_top_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void do_bottom_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void do_left_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void do_right_panel_expander(
    ui_context& ctx,
    accessor<bool> const& expanded,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// enum

string
enum_as_string(string const& name);

typedef control_result enum_drop_down_result;

enum_drop_down_result
do_unsafe_enum_drop_down(ui_context& ctx, raw_enum_info const& type_info,
    accessor<unsigned> const& value,
    layout const& layout_spec = default_layout,
    ddl_flag_set flags = NO_FLAGS);

void static inline
do_enum_drop_down(ui_context& ctx, raw_enum_info const& type_info,
    accessor<unsigned> const& value,
    layout const& layout_spec = default_layout,
    ddl_flag_set flags = NO_FLAGS)
{
    if (do_unsafe_enum_drop_down(ctx, type_info, value, layout_spec, flags))
        end_pass(ctx);
}

template<class Enum>
enum_drop_down_result
do_unsafe_enum_drop_down(ui_context& ctx, accessor<Enum> const& value,
    layout const& layout_spec = default_layout,
    ddl_flag_set flags = NO_FLAGS)
{
    // This line should trigger a compile-time error if Enum is not actually
    // an enum type.
    get_value_count(Enum());
    auto type_info = get_proper_type_info(Enum()).info;
    raw_enum_info const* enum_info = any_cast<raw_enum_info>(type_info);
    assert(enum_info);
    return do_unsafe_enum_drop_down(ctx, *enum_info,
        accessor_cast<unsigned>(ref(&value)), layout_spec, flags);
}

template<class Enum>
void
do_enum_drop_down(ui_context& ctx, accessor<Enum> const& value,
    layout const& layout_spec = default_layout,
    ddl_flag_set flags = NO_FLAGS)
{
    if (do_unsafe_enum_drop_down(ctx, value, layout_spec, flags))
        end_pass(ctx);
}

// color control

control_result
do_unsafe_color_control(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec = default_layout);

void static inline
do_color_control(ui_context& ctx, accessor<rgb8> const& color,
    layout const& layout_spec = default_layout)
{
    if (do_unsafe_color_control(ctx, color, layout_spec))
        end_pass(ctx);
}

// This gets the list of colors provided for selection by the color control.
std::vector<rgb8>
get_selectable_color_list();

// tristate tree node

api(enum internal)
enum class tristate_expansion
{
    CLOSED,
    HALFWAY,
    OPEN,
};

typedef control_result tristate_expander_result;
tristate_expander_result
do_unsafe_tristate_expander(
    ui_context& ctx,
    accessor<tristate_expansion> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void static inline
do_tristate_expander(
    ui_context& ctx,
    accessor<tristate_expansion> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    if (do_unsafe_tristate_expander(ctx, value, layout_spec, id))
        end_pass(ctx);
}

ALIA_DEFINE_FLAG_TYPE(tristate_tree_node)
ALIA_DEFINE_FLAG(tristate_tree_node, 0x1,
    TRISTATE_TREE_NODE_INITIALLY_EXPANDED)
ALIA_DEFINE_FLAG(tristate_tree_node, 0x2,
    TRISTATE_TREE_MINIMAL_HIT_TESTING)

struct tristate_tree_node : noncopyable
{
    tristate_tree_node() {}
    ~tristate_tree_node() { end(); }

    tristate_tree_node(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        tristate_tree_node_flag_set flags = NO_FLAGS,
        optional_storage<tristate_expansion> const& expanded =
            optional_storage<tristate_expansion>(none),
        widget_id expander_id = auto_id)
    { begin(ctx, layout_spec, flags, expanded, expander_id); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        tristate_tree_node_flag_set flags = NO_FLAGS,
        optional_storage<tristate_expansion> const& expanded =
            optional_storage<tristate_expansion>(none),
        widget_id expander_id = auto_id);

    bool do_children();

    node_expander_result const& expander_result() const
    { return expander_result_; }

    void end();

 private:
    ui_context* ctx_;

    grid_layout grid_;
    row_layout label_region_;
    collapsible_content content_;
    grid_row row_;
    column_layout column_;

    bool is_expanded_;
    node_expander_result expander_result_;
};

void
do_animated_astroid(gui_context& ctx,
    layout const& layout_spec = default_layout, optional<string> const& tooltip = none);

// blended_background_panel accepts a style name, a custom background color,
// and a blend factor and blends the custom color with the style's native
// background color.
struct blended_background_panel : noncopyable
{
 public:
    blended_background_panel() {}
    blended_background_panel(
        ui_context& ctx,
        accessor<string> const& style,
        accessor<rgba8> const& background_color, // must be gettable!
        double blend_factor,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    {
        begin(ctx, style, background_color, blend_factor, layout_spec,
            flags, id, state);
    }
    ~blended_background_panel() { end(); }
    void begin(
        ui_context& ctx, accessor<string> const& style,
        accessor<rgba8> const& background_color, // must be gettable!
        double blend_factor,
        layout const& layout_spec = default_layout,
        panel_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    // inner_region() is the region inside the panel's border
    layout_box inner_region() const { return panel_.inner_region(); }
    // outer_region() includes the border
    layout_box outer_region() const { return panel_.outer_region(); }
    // padded_region() includes the padding
    layout_box padded_region() const { return panel_.padded_region(); }
 private:
    custom_panel panel_;
    scoped_style substyle_;
};

// date and time displays

indirect_accessor<string>
as_text(gui_context& ctx, accessor<cradle::date> const& date);

void do_date(gui_context& ctx, accessor<optional<cradle::date> > const& date);

indirect_accessor<string>
as_text(gui_context& ctx, accessor<optional<cradle::date> > const& date);

void do_date(gui_context& ctx, accessor<cradle::date> const& date);

indirect_accessor<string>
as_text(gui_context& ctx, accessor<cradle::time> const& time);

void do_time(gui_context& ctx, accessor<cradle::time> const& time);

indirect_accessor<string>
as_text(gui_context& ctx, accessor<optional<cradle::time> > const& time);

void do_time(gui_context& ctx, accessor<optional<cradle::time> > const& time);

// SVG graphics

// Do an SVG graphic.
// 'svg' is an accessor to the SVG code.
// 'style_name' is a style with the following parameters.
// * top_color, bottom_color - two colors that specify a linear gradient over
//   the graphic. (Currently, color and graphic info within the SVG itself is
//   ignored)
// * width, height (or combined into 'size') - size of the graphic in the UI
//   If one of these is omitted, it is calculated automatically from the other
//   based on the SVG's native aspect ratio.
// * scale - Either "fit" or "fill" to specify how the SVG graphic is scaled
//   to the allotted UI region.
// * alignment - Either "left", "right", or "center" to specify how the graphic
//   is alignmed within its allotted UI region. (Currently graphics are always
//   center vertically.)
//
void do_svg_graphic(gui_context& ctx, accessor<string> const& style_name,
    accessor<string> const& svg, layout const& layout_spec = default_layout);

// Do the stylized "astroid" logo.
void do_app_logo(gui_context& ctx, accessor<string> const& svg,
    layout const& layout_spec = default_layout);

}

#endif
