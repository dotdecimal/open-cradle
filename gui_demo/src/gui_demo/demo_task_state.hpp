#ifndef GUI_DEMO_UI_TASKS_STATE_ROOT_TASK_STATE_HPP
#define GUI_DEMO_UI_TASKS_STATE_ROOT_TASK_STATE_HPP

#include <gui_demo/forward.hpp>

namespace gui_demo {

api(enum internal)
enum class demo_id
{
    TUTORIAL,
    WIDGETS,
    LAYOUT,
    CONTAINERS,
    TIMING,
};

api(struct internal)
struct demo_task_state
{
    optional<demo_id> selected_demo;
};

}

#endif
