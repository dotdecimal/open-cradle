#ifndef ANALYSIS_UI_STATE_COMMON_STATE_HPP
#define ANALYSIS_UI_STATE_COMMON_STATE_HPP

#include <analysis/common.hpp>

namespace analysis {

api(struct)
struct validation_state
{
    bool show_cancel_warning;

    optional<string> message;
};

}

#endif
