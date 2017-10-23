#ifndef ANALYSIS_UI_DISPLAYS_PBS_MACHINE_DISPLAY_HPP
#define ANALYSIS_UI_DISPLAYS_PBS_MACHINE_DISPLAY_HPP

#include <analysis/ui/common.hpp>
#include <dosimetry/proton/pbs/machine.hpp>

namespace analysis {

api(enum)
enum class pbs_machine_display_section
{
    NO_SECTION_SELECTION,
    IMAGING_OVERVIEW,
    SCANNING_OVERVIEW,
    DELIVERABLE_ENERGY_LIST,
    MODELED_ENERGY_LIST,
    PRISTINE_PEAKS,
    SIGMA_GRAPH
};

api(struct)
struct pbs_machine_display_state
{
    pbs_machine_display_section selected_section;
};

void do_pbs_machine_display(
    gui_context& ctx, accessor<pbs_machine_spec> const& machine,
    accessor<pbs_machine_display_state> const& state);

}

#endif
