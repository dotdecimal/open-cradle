#include <cradle/gui/app/instance.hpp>

#include <cradle/gui/widgets.hpp>

#include <analysis/ui/app_context.hpp>
#include <analysis/ui/registry.hpp>

using namespace analysis;

struct root_task_group_controller : task_group_controller
{
    string get_root_task_type_id() { return "root_task"; }

    virtual cradle::app_context&
    get_internal_app_context(gui_context& ctx, cradle::app_context& app_ctx)
    {
        return get_app_context(ctx, app_ctx);
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
        do_styled_text(ctx, text("title"), text("analysis"), UNPADDED);
    }
};

struct app_controller : app_controller_interface
{
    app_info get_app_info()
    {
        // Leaving the app version empty results in the app version
        // installed to the realm being used.
        return app_info(
            "mgh",
            "analysis",
            "",
            "analysis",
            "1.0.0-a11",
            "",
            "");
    }

    void register_tasks() { analysis::register_tasks(); }

    task_group_controller* get_root_task_group_controller()
    { return new root_task_group_controller; }
};

CRADLE_IMPLEMENT_APP(app_controller)
