#include <analysis/ui/registry.hpp>
#include <analysis/ui/tasks/pbs_analysis_tasks.hpp>
#include <analysis/ui/tasks/root_task.hpp>
#include <analysis/ui/tasks/spatial_analysis_3d_task.hpp>

namespace analysis {

void register_tasks()
{
    //register_pbs_analysis_tasks();
    register_root_task();
    //register_spatial_analysis_3d_task();
}

}
