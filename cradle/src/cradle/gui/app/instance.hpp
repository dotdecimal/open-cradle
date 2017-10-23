#ifndef CRADLE_GUI_APP_INSTANCE_HPP
#define CRADLE_GUI_APP_INSTANCE_HPP

#include <wx/glcanvas.h>

#include <alia/ui/backends/wx.hpp>

#include <cradle/gui/app/internals.hpp>

// This file is used in GUI applications to instantiate the top-level
// application objects: UI window, thread manager, thinknode connection, etc.

// To use this, an application should provide a class implementing the
// app_controller_interface and call CRADLE_IMPLEMENT_APP with the name of that
// class.

namespace cradle {

struct untyped_application : public wxGLApp
{
    untyped_application(app_controller_interface* controller);
    ~untyped_application();
    bool OnInit();
    int OnRun();
    int OnExit();
 private:
    app_instance* instance;
};

template<class Controller>
struct typed_application : untyped_application
{
    typed_application() : untyped_application(new Controller) {}
};

#define CRADLE_IMPLEMENT_APP(app_controller_type) \
    IMPLEMENT_APP(typed_application<app_controller_type>)

}

#endif
