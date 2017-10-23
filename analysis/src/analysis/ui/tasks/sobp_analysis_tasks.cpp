//#include <analysis/ui/tasks/sobp_analysis_tasks.hpp>
//#include <analysis/ui/app_context.hpp>
//
//namespace analysis {
//
//// DOUBLE SCATTERING MACHINE ANALYSIS
//
//CRADLE_DEFINE_UI_TASK(double_scattering_machine_analysis_task, app_context)
//
//indirect_accessor<string>
//double_scattering_machine_analysis_task::get_title(
//    gui_context& ctx, app_context& app_ctx, ui_task_context const& task)
//{
//    return ref(erase_type(ctx, text("PBS Machine Analysis")));
//}
//
//void double_scattering_machine_analysis_task::do_control_ui(
//    gui_context& ctx, app_context& app_ctx, ui_task_context const& task)
//{
//    auto state =
//        apply_value_type<double_scattering_machine_analysis_state>(ctx, task.state);
//
//    do_paragraph(ctx, text("Enter the UID of the PBS machine that you'd like to analyze."));
//
//    auto uid = _field(state, uid);
//    do_text_control(ctx, uid, FILL);
//}
//
//void double_scattering_machine_analysis_task::do_display_ui(
//    gui_context& ctx, app_context& app_ctx, ui_task_context const& task)
//{
//}
//
//void push_double_scattering_machine_analysis_task(
//    app_context& app_ctx, string const& parent_id)
//{
//    push_task(app_ctx, parent_id, "double_scattering_machine_analysis_task",
//        to_value(double_scattering_machine_analysis_state()));
//}
//
//// REGISTRATION
//
//void register_sobp_analysis_tasks()
//{
//    register_analysis_task("double_scattering_machine_analysis_task",
//        new double_scattering_machine_analysis_task);
//}
//
//}
