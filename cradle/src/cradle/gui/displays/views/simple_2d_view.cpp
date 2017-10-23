#include <cradle/gui/displays/views/simple_2d_view.hpp>
#include <cradle/gui/widgets.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/graphing.hpp>
#include <cradle/gui/displays/image_interface.hpp>
#include <alia/ui/utilities.hpp>

namespace cradle {

void static
do_vertical_profile_panel(
    gui_context& ctx, embedded_canvas& canvas,
    simple_2d_view_controller const& controller,
    accessor<std::vector<line_profile> > const& profiles)
{
    auto active = get_state(ctx, false);

    row_layout row(ctx);

    do_left_panel_expander(ctx, active, FILL_Y);

    {
        horizontal_collapsible_content collapsible(ctx, get(active),
            default_transition, 1.);
        alia_if (collapsible.do_content())
        {
            row_layout row(ctx, GROW);
            auto value_range =
                unwrap_optional(controller.get_profile_value_range(ctx));
            alia_if (is_gettable(value_range))
            {
                auto value_window =
                    get(value_range).max - get(value_range).min;
                auto padding = 0.05;
                auto scene_box =
                    make_box(
                        make_vector(
                            get(value_range).min - value_window * padding,
                            canvas.scene_box().corner[1]),
                        make_vector(
                            (1 + padding * 2) * value_window,
                            canvas.scene_box().size[0]));

                auto camera = make_default_camera(scene_box);
                camera.position[1] = canvas.camera().position[1];

                static data_reporting_parameters
                    spatial_y_parameters("Y", "mm", 1);
                static data_reporting_parameters
                    unknown_parameters("", "", 1);

                line_graph graph(ctx, scene_box,
                    in_ptr(&spatial_y_parameters),
                    in_ptr(&unknown_parameters),
                    text("line-profile-graph"),
                    layout(width(200, PIXELS), FILL),
                    storage(in(camera)));
                graph.canvas().force_scale_factor(1,
                    canvas.get_scale_factor()[1]);
                graph.canvas().set_scene_coordinates();
                for_each(ctx,
                    [&](gui_context& ctx, size_t index,
                        accessor<line_profile> const& profile)
                    {
                        alia_if (get(profile).axis == 0)
                        {
                            controller.do_profile_content(ctx, graph, profile);
                        }
                        alia_end
                    },
                    profiles);
                graph.do_highlight(ctx);
            }
            alia_end
            do_separator(ctx);
        }
        alia_end
    }

    do_spacer(ctx, GROW);
}

void static
do_horizontal_profile_panel(
    gui_context& ctx, embedded_canvas& canvas,
    simple_2d_view_controller const& controller,
    accessor<std::vector<line_profile> > const& profiles)
{
    auto active = get_state(ctx, false);

    {
        collapsible_content collapsible(ctx, get(active),
            default_transition, 0.);
        alia_if (collapsible.do_content())
        {
            do_separator(ctx);
            auto value_range =
                unwrap_optional(controller.get_profile_value_range(ctx));
            alia_if (is_gettable(value_range))
            {
                auto value_window =
                    get(value_range).max - get(value_range).min;
                auto padding = 0.05;
                auto scene_box =
                    make_box(
                        make_vector(
                            canvas.scene_box().corner[0],
                            get(value_range).min - value_window * padding),
                        make_vector(
                            canvas.scene_box().size[0],
                            (1 + padding * 2) * value_window));

                auto camera = make_default_camera(scene_box);
                camera.position[0] = canvas.camera().position[0];

                static data_reporting_parameters
                    spatial_x_parameters("X", "mm", 1);
                static data_reporting_parameters
                    unknown_parameters("", "", 1);

                line_graph graph(ctx, scene_box,
                    in_ptr(&spatial_x_parameters),
                    in_ptr(&unknown_parameters),
                    text("line-profile-graph"),
                    layout(height(200, PIXELS), FILL),
                    storage(in(camera)));
                graph.canvas().force_scale_factor(0,
                    canvas.get_scale_factor()[0]);
                graph.canvas().set_scene_coordinates();
                for_each(ctx,
                    [&](gui_context& ctx, size_t index,
                        accessor<line_profile> const& profile)
                    {
                        alia_if (get(profile).axis == 1)
                        {
                            controller.do_profile_content(ctx, graph, profile);
                        }
                        alia_end
                    },
                    profiles);
                graph.do_highlight(ctx);
            }
            alia_end
        }
        alia_end
    }

    do_bottom_panel_expander(ctx, active, FILL_X);
}

std::vector<rgb8> static
get_color_list(std::vector<line_profile> const& profiles)
{
    return
        map([](line_profile const& p) -> rgb8 { return p.color; }, profiles);
}

void static
do_profile_list_ui_for_axis(
    gui_context& ctx,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<std::vector<line_profile> > const& profiles,
    unsigned axis)
{
    grid_layout grid(ctx);

    for_each(ctx,
        [&](gui_context& ctx, size_t index,
            accessor<line_profile> const& profile)
        {
            alia_if (get(profile).axis == axis)
            {
                grid_row row(grid);
                do_color_control(ctx, _field(ref(&profile), color));
                do_text_control(ctx, _field(ref(&profile), position));
                do_text(ctx, text("mm"));
                if (do_icon_button(ctx, icon_type::REMOVE_ICON))
                {
                    remove_item_from_accessor(profiles, index);
                    end_pass(ctx);
                }
            }
            alia_end
        },
        profiles);

    alia_untracked_if (do_link(ctx, text("add profile")))
    {
        if (is_gettable(scene_geometry) && is_gettable(profiles))
        {
            auto scene_box = get_bounding_box(get(scene_geometry));
            push_back_to_accessor(profiles,
                line_profile(axis,
                    round_slice_position(get(scene_geometry).slicing[axis],
                        get_center(scene_box)[axis]),
                    choose_new_color(get_color_list(get(profiles)),
                        get_selectable_color_list())));
        }
        end_pass(ctx);
    }
    alia_end
}

void static
do_profile_list_ui(
    gui_context& ctx,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<std::vector<line_profile> > const& profiles)
{
    do_heading(ctx, text("subsection-heading"), text("Horizontal Profiles"));
    do_profile_list_ui_for_axis(ctx, scene_geometry, profiles, 1);

    do_heading(ctx, text("subsection-heading"), text("Vertical Profiles"));
    do_profile_list_ui_for_axis(ctx, scene_geometry, profiles, 0);
}

void static
do_point_sample_list_ui(
    gui_context& ctx,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<std::vector<point_sample_2d> > const& samples)
{
}

void static
do_line_profile_overlay_tool(
    gui_context& ctx, embedded_canvas& canvas,
    sliced_scene_geometry<2> const& scene_geometry,
    accessor<line_profile> const& profile)
{
    auto tool_id = get_widget_id(ctx);
    alia_untracked_if (is_gettable(profile))
    {
        double delta =
            apply_line_tool(
                canvas, get(profile).color, line_style(1, solid_line),
                get(profile).axis, get(profile).position,
                tool_id, LEFT_BUTTON);
        if (delta != 0)
        {
            set(_field(ref(&profile), position),
                round_slice_position(
                    scene_geometry.slicing[get(profile).axis],
                    get(profile).position + delta));
            end_pass(ctx);
        }
    }
    alia_end
}

void
do_simple_2d_view(
    gui_context& ctx,
    simple_2d_view_controller const& controller,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<simple_2d_view_measurement_state> const& state,
    layout const& layout_spec)
{
    alia_if (is_gettable(scene_geometry))
    {
        auto scene_box = get_bounding_box(get(scene_geometry));

        scoped_substyle style(ctx, text("display"));

        canvas c2;
        c2.initialize(ctx, scene_box, base_zoom_type::FIT_SCENE, none);

        {
            side_rulers rulers(ctx, c2, BOTTOM_RULER | LEFT_RULER, GROW);

            layered_layout layering(ctx, GROW);

            c2.begin(layout(size(30, 30, EM), GROW));

            clear_canvas(c2, rgb8(0x00, 0x00, 0x00));

            controller.do_content(ctx, c2);

            apply_panning_tool(c2, MIDDLE_BUTTON);
            apply_double_click_reset_tool(c2, LEFT_BUTTON);
            apply_zoom_drag_tool(ctx, c2, RIGHT_BUTTON);

            for_each(ctx,
                [&](gui_context& ctx, size_t index,
                    accessor<line_profile> const& profile)
                {
                    do_line_profile_overlay_tool(ctx, c2,
                        get(scene_geometry), profile);
                },
                _field(ref(&state), profiles));

            c2.end();

            controller.do_overlays(ctx);

            {
                row_layout row(ctx, GROW);
                do_vertical_profile_panel(ctx, c2, controller,
                    _field(ref(&state), profiles));
                {
                    column_layout column(ctx, GROW);
                    do_spacer(ctx, GROW);
                    do_horizontal_profile_panel(ctx, c2, controller,
                        _field(ref(&state), profiles));
                }
            }
        }
    }
    alia_else
    {
        do_empty_display_panel(ctx, layout_spec);
    }
    alia_end
}

void
do_simple_2d_view_controls(
    gui_context& ctx,
    simple_2d_view_controller const& controller,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<simple_2d_view_measurement_state> const& measurement)
{
    do_profile_list_ui(ctx, scene_geometry,
        _field(ref(&measurement), profiles));

    do_point_sample_list_ui(ctx, scene_geometry,
        _field(ref(&measurement), point_samples));
}

}
