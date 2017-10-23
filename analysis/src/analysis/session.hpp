#ifndef ANALYSIS_SESSION_HPP
#define ANALYSIS_SESSION_HPP

#include <analysis/common.hpp>

#include <analysis/ui/tasks/spatial_analysis_3d_task.hpp>
#include <analysis/ui/tasks/pbs_analysis_tasks.hpp>

namespace analysis {

api(union)
union analysis_info
{
    spatial_analysis_3d spatial3;
    pbs_machine_analysis pbs_machine;
};

api(struct)
struct session_info
{
    string analysis_entry_id;
};

}

#endif
