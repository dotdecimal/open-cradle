#include <analysis/ui/tasks/root_task.hpp>
#include <analysis/ui/app_context.hpp>

#include <analysis/ui/tasks/spatial_analysis_3d_task.hpp>
#include <analysis/ui/tasks/pbs_analysis_tasks.hpp>
#include <analysis/ui/tasks/sobp_analysis_tasks.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/gui/collections.hpp>

#include <cradle/gui/task_interface.hpp>
#include <cradle/gui/services.hpp>
#include <cradle/gui/displays/compositions/image_displays.hpp>
#include <analysis/ui/displays/pbs_machine_display.hpp>
#include <cradle/gui/displays/regular_image.hpp>

namespace analysis {

CRADLE_DEFINE_SIMPLE_UI_TASK(root_task, app_context, root_task_state)

void root_task::do_title(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<root_task_state> const& task)
{
    do_task_title(ctx, text("APP"), text("analysis"));
}

//void static
//do_analysis_summary(gui_context& ctx, app_context& app_ctx,
//    accessor<analysis_info> const& analysis)
//{
//    _switch_accessor (_field(ref(&analysis), type))
//    {
//        alia_case (analysis_info_type::SPATIAL3):
//        {
//            do_spatial_analysis_3d_summary(ctx, app_ctx,
//                _union_member(ref(&analysis), spatial3));
//            break;
//        }
//        alia_case (analysis_info_type::PBS_MACHINE):
//        {
//            do_pbs_machine_analysis_summary(ctx, app_ctx,
//                _union_member(ref(&analysis), pbs_machine));
//            break;
//        }
//    }
//    _end_switch
//}

void root_task::do_control_ui(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<root_task_state> const& task)
{
    auto state = task.state;

    scrollable_panel
        scrolling(ctx, text("scrollable-content"), GROW,
            PANEL_NO_HORIZONTAL_SCROLLING);

    do_heading(ctx, text("subsection-heading"), text("Analysis"));

    do_text(ctx, text("Enter the RKS entry ID of your analysis below."));

    auto analysis_entry_id =
        _field(get_session_state(app_ctx), analysis_entry_id);

    do_text_control(ctx, analysis_entry_id, FILL_X);

    watch_rks_entry(ctx, app_ctx, analysis_entry_id);

    auto analysis =
        gui_rks_entry_value<spatial_analysis_3d>(
            ctx, app_ctx,
            analysis_entry_id);

    alia_if (is_gettable(analysis))
    {
        do_separator(ctx);
        do_spatial_analysis_3d_controls(ctx, app_ctx, analysis);
    }
    alia_end

    //do_heading(ctx, text("subsection-heading"), text("Analyses"));

    //for_each(ctx,
    //    [&](gui_context& ctx, size_t index,
    //        accessor<analysis_info> const& analysis)
    //    {
    //        auto selected =
    //            make_radio_accessor(
    //                unwrap_optional(_field(state, selected_analysis)),
    //                in(index));
    //        auto widget_id = get_widget_id(ctx);
    //        panel p(ctx, text("analysis-panel"), default_layout, NO_FLAGS,
    //            widget_id,
    //            get_widget_state(ctx, widget_id,
    //                is_gettable(selected) && get(selected) ?
    //                WIDGET_SELECTED : NO_FLAGS));
    //        alia_if (is_gettable(analysis))
    //        {
    //            alia_untracked_if (detect_click(ctx, widget_id, LEFT_BUTTON))
    //            {
    //                switch (get(analysis).type)
    //                {
    //                 case analysis_info_type::PBS_MACHINE:
    //                    push_pbs_machine_analysis_task(app_ctx, task.id, index);
    //                    break;
    //                 case analysis_info_type::SPATIAL3:
    //                    push_spatial_analysis_3d_task(app_ctx, task.id, index);
    //                    break;
    //                }
    //                end_pass(ctx);
    //            }
    //            alia_end
    //        }
    //        alia_end
    //        do_analysis_summary(ctx, app_ctx, analysis);
    //    },
    //    _field(get_session_state(app_ctx), analyses));

    //do_heading(ctx, text("subsection-heading"), text("New Analysis"));

    //auto analyses = _field(get_session_state(app_ctx), analyses);
    //alia_if (is_gettable(analyses))
    //{
    //    clickable_panel panel(ctx, text("subtask-panel"));
    //    if (panel.clicked())
    //    {
    //        auto new_index = get(analyses).size();
    //        push_back_to_accessor(
    //            analyses,
    //            make_analysis_info_with_spatial3(spatial_analysis_3d()));
    //        push_spatial_analysis_3d_task(app_ctx, task.id, new_index);
    //        end_pass(ctx);
    //    }
    //    do_heading(ctx, text("heading"), text("3D Scene"));
    //    do_paragraph(ctx, text("Analyze spatial 3D data (images, dose, etc.)"));
    //}
    //{
    //    clickable_panel panel(ctx, text("subtask-panel"));
    //    if (panel.clicked())
    //    {
    //        auto new_index = get(analyses).size();
    //        push_back_to_accessor(
    //            analyses,
    //            make_analysis_info_with_pbs_machine(pbs_machine_analysis()));
    //        push_pbs_machine_analysis_task(app_ctx, task.id, new_index);
    //        end_pass(ctx);
    //    }
    //    do_heading(ctx, text("heading"), text("PBS Machine Analysis"));
    //    do_paragraph(ctx, text("Analyze a PBS machine model"));
    //}
    //alia_end
}

void root_task::do_display_ui(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<root_task_state> const& task)
{
    auto analysis =
        gui_rks_entry_value<spatial_analysis_3d>(
            ctx, app_ctx,
            _field(get_session_state(app_ctx), analysis_entry_id));
    alia_if (is_gettable(analysis))
    {
        do_spatial_analysis_3d_display(ctx, app_ctx, analysis);
    }
    alia_else
    {
        do_empty_display_panel(ctx);
    }
    alia_end
}

void register_root_task()
{
    register_app_task("root_task", new root_task);
}

}
