#ifndef ANALYSIS_UI_TASKS_ROOT_TASK_HPP
#define ANALYSIS_UI_TASKS_ROOT_TASK_HPP

#include <analysis/ui/common.hpp>

namespace analysis {

api(struct)
struct root_task_state
{
    optional<size_t> selected_analysis;
};

void register_root_task();

}

#endif
