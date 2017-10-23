#include <analysis/ui/tasks/spatial_analysis_3d_task.hpp>

#include <alia/ui/utilities.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/display.hpp>
#include <cradle/gui/displays/image_utilities.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/gui/displays/sliced_image.hpp>
#include <cradle/gui/requests.hpp>
#include <cradle/gui/task_interface.hpp>
#include <cradle/gui/widgets.hpp>

#include <visualization/ui/inspection.hpp>
#include <visualization/ui/rendering/geometry_rendering.hpp>
#include <visualization/ui/rendering/image_rendering.hpp>
#include <visualization/ui/views/spatial_3d_views.hpp>
#include <visualization/ui/views/statistical_dose_views.hpp>

#include <analysis/ui/app_context.hpp>
#include <analysis/ui/controls/common_controls.hpp>

using namespace visualization;

namespace analysis {

// common utilities

optional<request<image3> > static
compose_image_request(spatial_analysis_3d const& analysis)
{
    return
        analysis.image && !get(analysis.image).thinknode_id.empty()
          ? some(
                rq_object(
                    object_reference<image3>(
                        get(analysis.image).thinknode_id)))
          : none;
}

optional<request<image3> > static
compose_dose_request(dose_analysis const& analysis)
{
    return
        !analysis.thinknode_id.empty()
      ? some(
            rq_object(
                object_reference<image3>(
                    analysis.thinknode_id)))
      : none;
}

// Compose the scene geometry request for the given analysis.
optional<request<sliced_scene_geometry<3> > > static
compose_scene_geometry_request(spatial_analysis_3d const& analysis)
{
    auto image_request = compose_image_request(analysis);
    if (image_request)
    {
        return
            compose_sliced_scene_geometry_request(
                rq_compute_regular_image_geometry(
                    get(image_request),
                    rq_value(optional<out_of_plane_information>())));
    }
    for (auto const& dose : analysis.doses)
    {
        auto dose_request = compose_dose_request(dose);
        if (dose_request)
        {
            return
                compose_sliced_scene_geometry_request(
                    rq_compute_regular_image_geometry(
                        get(dose_request),
                        rq_value(optional<out_of_plane_information>())));
        }
    }
    return none;
}

// controls

void do_spatial_analysis_3d_controls(
    gui_context& ctx, app_context& app_ctx,
    accessor<spatial_analysis_3d> const& analysis)
{
    grid_layout grid(ctx);

    alia_if (has_value(_field(analysis, image)))
    {
        do_heading(ctx, text("subsection-heading"), text("Image"));
        {
            grid_row row(grid);
            do_bullet(ctx);
            {
                column_layout column(ctx, BASELINE_Y);
                auto image = unwrap_optional(_field(analysis, image));
                do_flow_text(ctx, _field(image, label), FILL_X);
                do_text(ctx, _field(image, thinknode_id));
            }
        }
    }
    alia_end

    alia_if (!is_empty(_field(analysis, doses)))
    {
        do_heading(ctx, text("subsection-heading"), text("Doses"));
        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<dose_analysis> const& dose)
            {
                grid_row row(grid);
                do_bullet(ctx);
                {
                    column_layout column(ctx, BASELINE_Y);
                    do_flow_text(ctx, _field(dose, label), FILL_X);
                    do_text(ctx, _field(dose, thinknode_id));
                    {
                        grid_layout level_grid(ctx);
                        for_each(ctx,
                            [&](gui_context& ctx, size_t index,
                                auto const& level)
                            {
                                grid_row level_row(level_grid);
                                do_color(ctx, _field(level, color));
                                do_text(ctx, _field(level, value));
                            },
                            _field(_field(dose, rendering), levels));
                    }
                }
            },
            _field(analysis, doses));
    }
    alia_end

    alia_if (!is_empty(_field(analysis, structures)))
    {
        do_heading(ctx, text("subsection-heading"), text("Structures"));
        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<structure_analysis> const& structure)
            {
                grid_row row(grid);
                do_color(ctx, _field(structure, color));
                {
                    column_layout column(ctx, BASELINE_Y);
                    do_flow_text(ctx, _field(structure, label), FILL_X);
                    do_text(ctx, _field(structure, thinknode_id));
                }
            },
            _field(analysis, structures));
    }
    alia_end
}

// display

void static
add_dose(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<dose_analysis> const& analysis)
{
    auto dose_request =
        unwrap_optional(
            gui_apply(ctx, compose_dose_request, analysis));

    auto& gui_dose =
        make_image_interface(ctx,
            dose_request,
            rq_in(optional<out_of_plane_information>()),
            get_default_value_range(ctx, dose_request));

    auto dose_levels =
        gui_apply(ctx,
            [ ](image_level_list const& levels)
            {
                return
                    map([ ](auto const& level)
                        {
                            return
                                visualization::image_level(
                                    level.value,
                                    level.color,
                                    level.lower_color);
                        },
                        levels);
            },
            _field(_field(analysis, rendering), levels));

    auto color_wash_parameters =
        gui_apply(ctx,
            [ ](auto const& rendering)
            {
                return
                    rendering
                  ? some(
                        visualization::color_wash_rendering_parameters(
                            get(rendering).opacity))
                  : none;
            },
            _field(_field(analysis, rendering), color_wash));
    alia_if (has_value(color_wash_parameters))
    {
        add_image_color_wash(ctx,
            scene_graph,
            &gui_dose,
            dose_levels,
            unwrap_optional(color_wash_parameters),
            FILLED_OVERLAY_CANVAS_LAYER);
    }
    alia_end

    auto isoband_parameters =
        gui_apply(ctx,
            [ ](auto const& rendering)
            {
                return
                    rendering
                  ? some(
                        visualization::isoband_rendering_parameters(
                            get(rendering).opacity))
                  : none;
            },
            _field(_field(analysis, rendering), isobands));
    alia_if (has_value(isoband_parameters))
    {
        add_image_isobands(ctx,
            scene_graph,
            &gui_dose,
            dose_levels,
            unwrap_optional(isoband_parameters),
            FILLED_OVERLAY_CANVAS_LAYER);
    }
    alia_end

    auto isoline_parameters =
        gui_apply(ctx,
            [ ](auto const& rendering)
            {
                return
                    rendering
                  ? some(
                        visualization::isoline_rendering_parameters(
                            get(rendering).type,
                            get(rendering).width,
                            get(rendering).opacity))
                  : none;
            },
            _field(_field(analysis, rendering), isolines));
    alia_if (has_value(isoline_parameters))
    {
        add_image_isolines(ctx,
            scene_graph,
            &gui_dose,
            dose_levels,
            unwrap_optional(isoline_parameters),
            LINE_OVERLAY_CANVAS_LAYER);
    }
    alia_end

    add_inspectable_image(ctx,
        scene_graph,
        &gui_dose,
        gui_apply(ctx,
            make_unstyled_text,
            _field(analysis, label)),
        text("%.1f"),
        text("Gy(RBE)"));
}

void static
add_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<structure_analysis> const& analysis)
{
    auto structure =
        gui_apply(ctx,
            [ ](auto const& analysis)
            {
                cradle::gui_structure structure;
                structure.color = analysis.color;
                structure.label = make_unstyled_text(analysis.label);
                structure.geometry =
                    rq_object(
                        object_reference<structure_geometry>(
                            analysis.thinknode_id));
                return structure;
            },
            analysis);

    // Add the filled version.
    auto fill_parameters =
        gui_apply(ctx,
            [ ](auto const& rendering)
            {
                return
                    rendering
                  ? some(float(get(rendering).opacity))
                  : none;
            },
            _field(_field(analysis, rendering), fill));
    alia_if (has_value(fill_parameters))
    {
        add_sliced_filled_structure(ctx,
            scene_graph,
            structure,
            unwrap_optional(fill_parameters));
    }
    alia_end

    // Add the outlined version.
    auto outline_parameters =
        gui_apply(ctx,
            [ ](auto const& rendering)
            {
                return
                    rendering
                  ? some(
                        visualization::spatial_region_outline_parameters(
                            get(rendering).type,
                            get(rendering).width,
                            get(rendering).opacity))
                  : none;
            },
            _field(_field(analysis, rendering), outline));
    alia_if (has_value(outline_parameters))
    {
        add_sliced_outlined_structure(ctx,
            scene_graph,
            structure,
            unwrap_optional(outline_parameters));
    }
    alia_end
}

struct spatial_analysis_3d_view_controller : spatial_3d_view_controller
{
    void
    generate_scene(gui_context& ctx,
        spatial_3d_scene_graph& scene_graph)
    {
        auto& app_ctx = *app_ctx_;
        auto analysis = analysis_;

        set_scene_geometry(ctx,
            scene_graph,
            gui_request(ctx,
                unwrap_optional(
                    gui_apply(ctx,
                        compose_scene_geometry_request,
                        analysis))),
            in(patient_position_type::HFS));

        auto image_request =
            unwrap_optional(
                gui_apply(ctx, compose_image_request, analysis));
        alia_if (is_gettable(image_request))
        {
            auto& gui_image =
                make_image_interface(ctx,
                    image_request,
                    rq_in(optional<out_of_plane_information>()),
                    get_default_value_range(ctx, image_request));
            add_gray_image(ctx,
                scene_graph,
                &gui_image,
                gui_apply(ctx,
                    [ ](auto const& rendering)
                    {
                        return
                            visualization::gray_image_rendering_parameters(
                                rendering.level,
                                rendering.window);
                    },
                    _field(unwrap_optional(_field(analysis, image)), rendering)),
                BACKGROUND_CANVAS_LAYER);
            add_inspectable_image(ctx,
                scene_graph,
                &gui_image,
                gui_apply(ctx,
                    make_unstyled_text,
                    _field(unwrap_optional(_field(analysis, image)), label)),
                text("%.1f"),
                text(""));
        }
        alia_end

        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<dose_analysis> const& dose)
            {
                add_dose(ctx, scene_graph, dose);
            },
            _field(analysis, doses));

        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<structure_analysis> const& structure)
            {
                add_structure(ctx, scene_graph, structure);
            },
            _field(analysis, structures));

        //for_each(ctx,
        //    [&](gui_context& ctx, accessor<string> const& id,
        //        accessor<gui_structure> const& structure) -> void
        //    {
        //        auto visible =
        //            local_calc<bool>(ctx,
        //                c_fn(&is_structure_visible),
        //                display_ctx->structure_visibility, id);
        //        alia_if (is_gettable(visible) && get(visible))
        //        {
        //            draw_structure(ctx, c3d, image_grid,
        //                structure, display_ctx->spatial_region_options);
        //        }
        //        alia_end
        //    },
        //    display_ctx->structures);
    }

    app_context* app_ctx_;
    indirect_accessor<spatial_analysis_3d> analysis_;
};

void do_spatial_analysis_3d_display(
    gui_context& ctx, app_context& app_ctx,
    accessor<spatial_analysis_3d> const& analysis)
{
    null_display_context null_ctx;
    display_view_provider<null_display_context> provider(&null_ctx);

    auto sliced_view_state = get_state(ctx, sliced_3d_view_state());

    // Add spatial views.
    spatial_analysis_3d_view_controller spatial_view_controller;
    spatial_view_controller.app_ctx_ = &app_ctx;
    spatial_view_controller.analysis_ = alia::ref(&analysis);
    spatial_3d_views spatial_views;
    add_spatial_3d_views(ctx,
        provider,
        spatial_views,
        &spatial_view_controller,
        alia::ref(&sliced_view_state));

    auto display_state = get_state(ctx, cradle::display_state());
    do_display(ctx, provider,
        gui_apply(ctx,
            [ ]()
            {
                return
                    display_view_composition_list{
                        make_default_spatial_3d_view_composition()};
            }),
        display_state,
        _field(get_app_config(ctx, app_ctx), display_controls_width),
        [&](gui_context& ctx,
            accessor<cradle::display_state> const& state,
            accordion& accordion)
        {
        });
}

// task definition

//CRADLE_DEFINE_SIMPLE_UI_TASK(spatial_analysis_3d_task, app_context,
//    spatial_analysis_3d_task_state)
//
//auto static
//get_analysis_state(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<spatial_analysis_3d_task_state> const& task)
// -> decltype(
//        _union_member(
//            select_index_via_accessor(
//                _field(get_session_state(app_ctx), analyses),
//                _field(task.state, analysis_index)),
//            spatial3))
//{
//    return
//        _union_member(
//            select_index_via_accessor(
//                _field(get_session_state(app_ctx), analyses),
//                _field(task.state, analysis_index)),
//            spatial3);
//}
//
//void
//spatial_analysis_3d_task::do_title(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<spatial_analysis_3d_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//    do_task_title(ctx, text("Spatial 3D Analysis"));
//}
//
//void spatial_analysis_3d_task::do_control_ui(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<spatial_analysis_3d_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//    do_spatial_analysis_3d_controls(ctx, app_ctx, task.id, state);
//    do_done_button_row(ctx, app_ctx, task_id);
//}
//
//void spatial_analysis_3d_task::do_display_ui(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<spatial_analysis_3d_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//    do_spatial_analysis_3d_display(ctx, app_ctx, state);
//}
//
//void register_spatial_analysis_3d_task()
//{
//    register_app_task("spatial_analysis_3d_task", new spatial_analysis_3d_task);
//}
//
//void push_spatial_analysis_3d_task(
//    app_context& app_ctx, string const& parent_id,
//    size_t analysis_index)
//{
//    push_task(app_ctx, parent_id, "spatial_analysis_3d_task",
//        to_value(spatial_analysis_3d_task_state(analysis_index)));
//}

}
