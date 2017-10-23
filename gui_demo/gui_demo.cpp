#include <cradle/gui/widgets.hpp>
#include <cradle/gui/app/instance.hpp>
#include <cradle/gui/app/interface.hpp>

#include <gui_demo/registry.hpp>

using namespace gui_demo;

struct root_task_group_controller : task_group_controller
{
    string get_root_task_type_id() { return "demo_task"; }

    virtual cradle::app_context&
    get_internal_app_context(gui_context& ctx, cradle::app_context& app_ctx)
    {
        return app_ctx;
    }

    indirect_accessor<gui_task_group_state>
    get_state_accessor(gui_context& ctx, cradle::app_context& app_ctx)
    {
        // Use local state for this task group.
        state_accessor<gui_task_group_state> state;
        if (get_state(ctx, &state))
        {
            set(state,
                make_initial_task_group_state(*app_ctx.instance,
                    this->get_root_task_type_id()));
        }
        return make_indirect(ctx, state);
    }

    void do_header_label(gui_context& ctx, cradle::app_context& app_ctx)
    {
        row_layout row(ctx, BASELINE_Y);
        do_app_logo(ctx, _field(get_app_info(ctx, app_ctx), logo), BASELINE_Y);
        do_styled_text(ctx, text("title"), text("GUI Demo"), UNPADDED);
    }
};

struct app_controller : app_controller_interface
{
    app_info get_app_info()
    {
        // Leaving the app version empty results in the app version
        // installed to the realm being used.
        return app_info(
            "decimal",
            "planning",
            "",
            "Astroid GUI Demo",
            "1.0.0",
            "",
            "");
    }

    void register_tasks() { gui_demo::register_tasks(); }

    task_group_controller* get_root_task_group_controller()
    { return new root_task_group_controller; }
};

CRADLE_IMPLEMENT_APP(app_controller)
