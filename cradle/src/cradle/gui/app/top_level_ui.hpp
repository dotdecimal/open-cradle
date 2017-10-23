#ifndef CRADLE_GUI_APP_TOP_LEVEL_UI_HPP
#define CRADLE_GUI_APP_TOP_LEVEL_UI_HPP

#include <alia/ui/backends/interface.hpp>

namespace cradle {

struct app_instance;

struct app_window_controller : alia::app_window_controller
{
    app_instance* instance;

    void do_ui(alia::ui_context& ctx);
};

}

#endif
