#include <analysis/ui/tasks/pbs_analysis_tasks.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/requests.hpp>
#include <cradle/gui/task_interface.hpp>

#include <analysis/ui/app_context.hpp>
#include <analysis/ui/controls/common_controls.hpp>
#include <analysis/ui/displays/pbs_machine_display.hpp>

namespace analysis {

// PBS MACHINE ANALYSIS

// controls

void do_pbs_machine_analysis_summary(
    gui_context& ctx, app_context& app_ctx,
    accessor<pbs_machine_analysis> const& analysis)
{
}

// display

optional<request<pbs_machine_spec> > static
compose_pbs_machine_request(pbs_machine_analysis const& analysis)
{
    return
        analysis.thinknode_id
          ? some(
                rq_immutable(
                    immutable_reference<pbs_machine_spec>(
                        get(analysis.thinknode_id))))
          : none;
}

void do_pbs_machine_analysis_display(
    gui_context& ctx, app_context& app_ctx,
    accessor<pbs_machine_analysis> const& analysis)
{
    auto request =
        unwrap_optional(gui_apply(ctx, compose_pbs_machine_request, analysis));
    alia_if (is_gettable(request))
    {
        auto machine = gui_request(ctx, request);
        auto display_state = get_state(ctx, pbs_machine_display_state());
        do_pbs_machine_display(ctx, machine, display_state);
    }
    alia_else
    {
        do_empty_display_panel(ctx, GROW);
    }
    alia_end
}

// task definition

//CRADLE_DEFINE_SIMPLE_UI_TASK(pbs_machine_analysis_task, app_context,
//    pbs_machine_analysis_task_state)
//
//auto static
//get_analysis_state(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<pbs_machine_analysis_task_state> const& task)
// -> decltype(
//        _union_member(
//            select_index_via_accessor(
//                _field(get_session_state(app_ctx), analyses),
//                _field(task.state, analysis_index)),
//            pbs_machine))
//{
//    return
//        _union_member(
//            select_index_via_accessor(
//                _field(get_session_state(app_ctx), analyses),
//                _field(task.state, analysis_index)),
//            pbs_machine);
//}
//
//void
//pbs_machine_analysis_task::do_title(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<pbs_machine_analysis_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//
//    do_task_title(ctx, text("PBS Machine Analysis")); //_field(state, label));
//}
//
//void pbs_machine_analysis_task::do_control_ui(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<pbs_machine_analysis_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//
//    do_paragraph(ctx, text("Enter the UID of the PBS machine that you'd like to analyze."));
//
//    do_text_control(ctx, unwrap_optional(_field(state, thinknode_id)), FILL);
//
//    do_done_button_row(ctx, app_ctx, task.id);
//}
//
//void pbs_machine_analysis_task::do_display_ui(
//    gui_context& ctx, app_context& app_ctx,
//    gui_task_context<pbs_machine_analysis_task_state> const& task)
//{
//    auto state = get_analysis_state(ctx, app_ctx, task);
//    do_pbs_machine_analysis_display(ctx, app_ctx, state);
//}
//
//void register_pbs_analysis_tasks()
//{
//    register_app_task("pbs_machine_analysis_task",
//        new pbs_machine_analysis_task);
//}
//
//void push_pbs_machine_analysis_task(
//    app_context& app_ctx, string const& parent_id,
//    size_t analysis_index)
//{
//    push_task(app_ctx, parent_id, "pbs_machine_analysis_task",
//        to_value(pbs_machine_analysis_task_state(analysis_index)));
//}

}
