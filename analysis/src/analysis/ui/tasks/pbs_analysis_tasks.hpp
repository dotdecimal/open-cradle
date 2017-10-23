#ifndef ANALYSIS_UI_TASKS_PBS_ANALYSIS_TASKS_HPP
#define ANALYSIS_UI_TASKS_PBS_ANALYSIS_TASKS_HPP

#include <analysis/ui/common.hpp>
#include <analysis/ui/state/common_state.hpp>

namespace analysis {

void register_pbs_analysis_tasks();

// PBS MACHINE ANALYSIS

api(struct)
struct pbs_machine_analysis
{
    string label;
    optional<string> thinknode_id;
};

//api(struct)
//struct pbs_machine_analysis_task_state
//{
//    size_t analysis_index;
//};
//
//void push_pbs_machine_analysis_task(
//    app_context& app_ctx, string const& parent_id,
//    size_t analysis_index);

void do_pbs_machine_analysis_summary(
    gui_context& ctx, app_context& app_ctx,
    accessor<pbs_machine_analysis> const& analysis);

void do_pbs_machine_analysis_display(
    gui_context& ctx, app_context& app_ctx,
    accessor<pbs_machine_analysis> const& analysis);

}

#endif
