#include <cradle/gui/app/interface.hpp>

#include <cradle/gui/app/internals.hpp>

// This file implements the interface that the app provides to lower-level
// code.

namespace cradle {

gui_task_group_state
make_initial_task_group_state(app_instance& instance,
    string const& root_task_type_id)
{
    gui_task_group_state state;

    auto root_task_id = generate_unique_id(instance);

    state.root_id = root_task_id;

    state.tasks[root_task_id] =
        gui_task_state(
            root_task_type_id,
            value(),
            none,
            0, 0, 0);

    return state;
}

void push_task_group(app_instance& instance, task_group_controller* controller)
{
    // Only allow new task groups if all existing task groups have readable state and are
    // thus reflected in the existing task stack.
    for (auto const& group : instance.task_groups)
    {
        if (!group->state.is_gettable())
            return;
    }

    task_group_ptr group(new task_group);
    group->controller.reset(controller);
    group->id = generate_unique_id(instance);
    // These shouldn't matter since they're set every pass, but it might be
    // useful to detect if they're accidentally used before they're set.
    group->app_ctx = 0;
    group->state = 0;
    instance.task_groups.push_back(group);
}

string generate_unique_id(app_context& app_ctx)
{
    return generate_unique_id(*app_ctx.instance);
}

std::vector<task_group_ptr>& get_task_groups(app_context& app_ctx)
{
    return app_ctx.instance->task_groups;
}

void reset_task_groups(app_context& app_ctx)
{
    reset_task_groups(*app_ctx.instance);
}

void request_state_write_back(app_context& app_ctx)
{
    app_ctx.instance->state_write_back_requested = true;
}

indirect_accessor<app_info>
get_app_info(gui_context& ctx, app_context& app_ctx)
{
    return make_indirect(ctx, in_ptr(&app_ctx.instance->info));
}

indirect_accessor<app_config>
get_app_config(gui_context& ctx, app_context& app_ctx)
{
    return make_indirect(ctx, make_accessor(app_ctx.instance->config));
}

}
