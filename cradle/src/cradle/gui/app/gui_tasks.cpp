#include <cradle/gui/app/gui_tasks.hpp>

#include <algorithm>

#include <alia/ui/system.hpp>

#include <cradle/gui/app/internals.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/diff.hpp>

namespace cradle {

void register_task(gui_task_implementation_table& table, string const& id,
    alia__shared_ptr<gui_task_interface> const& implementation)
{
    table.implementations[id] = implementation;
}

gui_task_interface*
find_task_implementation(
    gui_task_implementation_table const& table, string const& task_id)
{
    auto i = table.implementations.find(task_id);
    if (i == table.implementations.end())
        throw unimplemented_task(task_id);
    return i->second.get();
}

gui_task_interface&
get_task_implementation(gui_context& ctx, gui_task_with_context const& task)
{
    keyed_data<gui_task_interface*>* implementation;
    get_data(ctx, &implementation);
    if (is_refresh_pass(ctx))
        refresh_keyed_data(*implementation, make_id(task.type));
    if (!is_valid(*implementation))
        set(*implementation, find_task_implementation(*task.table, task.type));
    return *get(*implementation);
}

static inline app_context&
get_app_context(gui_task_with_context& task)
{ return *task.group->app_ctx; }

gui_task_context<value>
make_task_context(gui_context& ctx, gui_task_with_context& task)
{
    gui_task_context<value> task_ctx;
    task_ctx.id = task.id;
    alia_if (task.is_phantom)
    {
        auto task_accessor =
            make_readonly(make_accessor(task.phantom));
        task_ctx.state = make_indirect(ctx, _field(task_accessor, state));
        task_ctx.active_subtask =
            make_indirect(ctx, _field(task_accessor, active_subtask));
    }
    alia_else
    {
        auto group_state = task.group->state;
        auto task_accessor =
            minimize_id_changes(ctx,
                &task.id_change_minimization,
                select_map_index(_field(group_state, tasks), in(task.id)));
        task_ctx.state = make_indirect(ctx, _field(task_accessor, state));
        task_ctx.active_subtask =
            make_indirect(ctx, _field(task_accessor, active_subtask));
    }
    alia_end
    return task_ctx;
}

void do_title(gui_context& ctx, gui_task_with_context& task)
{
    auto& impl = get_task_implementation(ctx, task);
    impl.untyped_do_title(ctx, get_app_context(task),
        make_task_context(ctx, task), task.state_conversion_data);
}

void do_task_control_ui(gui_context& ctx, gui_task_with_context& task)
{
    auto& impl = get_task_implementation(ctx, task);
    impl.untyped_do_control_ui(ctx, get_app_context(task),
        make_task_context(ctx, task), task.state_conversion_data);
}

void do_task_display_ui(gui_context& ctx, gui_task_with_context& task)
{
    auto& impl = get_task_implementation(ctx, task);
    impl.untyped_do_display_ui(ctx, get_app_context(task),
        make_task_context(ctx, task), task.state_conversion_data);
}

void static
push_task_stack_for_group(std::vector<abbreviated_task_info>& stack,
    gui_task_group_state const& group_state, size_t group_index)
{
    // Starting at the root task, follow the active_subtask references within
    // the tasks and record the path traversed.
    string task_id = group_state.root_id;
    while (1)
    {
        auto task = group_state.tasks.find(task_id);
        if (task == group_state.tasks.end())
            break;
        {
            abbreviated_task_info info;
            info.id = task_id;
            info.type = task->second.type;
            info.group_index = group_index;
            stack.push_back(info);
        }
        auto subtask_id = task->second.active_subtask;
        if (!subtask_id)
            break;
        task_id = get(subtask_id);
    }
}

void static
convert_task_to_phantom(gui_task_stack_data& data, size_t index)
{
    auto& popped_task = data.stack.tasks[index].task;
    popped_task.is_phantom = true;
    auto const& group_state =
        data.cache.groups[popped_task.group_index].value;
    auto task_state =
        group_state.tasks.find(popped_task.id);
    if (task_state != group_state.tasks.end())
        popped_task.phantom.set(task_state->second);
}

void static
push_task_to_stack(
    app_context& app_ctx,
    gui_task_stack_data& data,
    abbreviated_task_info const& task_info,
    push_gui_task_flag_set flags = NO_FLAGS)
{
    gui_task_with_context task;
    task.group_index = task_info.group_index;
    task.group = app_ctx.instance->task_groups[task.group_index];
    task.table = &get_task_implementation_table(app_ctx);
    task.id = task_info.id;
    task.type = task_info.type;
    task.is_phantom = false;
    push_task(data.stack, task, flags);
}

generic_gui_task_stack<gui_task_with_context>&
get_gui_task_stack(gui_context& ctx, app_context& app_ctx)
{
    scoped_data_block data_block(ctx, app_ctx.instance->task_stack_ui_block);
    gui_task_stack_data* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        auto& groups = get_task_groups(app_ctx);
        size_t n_groups = groups.size();
        // Check if the cache is still valid.
        bool cache_valid = true;
        if (data->cache.groups.size() != groups.size())
            cache_valid = false;
        if (cache_valid)
        {
            for (size_t i = 0; i != n_groups; ++i)
            {
                refresh_keyed_data(data->cache.groups[i], groups[i]->state.id());
                if (!is_valid(data->cache.groups[i]))
                    cache_valid = false;
            }
        }
        // If the cache is no longer valid, update the task stack to reflect
        // the changes in the group states.
        if (!cache_valid)
        {
            auto& old_stack = data->cache.stack;
            // If the old list is empty, we're initializing the task stack,
            // so we don't want animation.
            auto flags = old_stack.empty() ? PUSH_UI_TASK_NO_ANIMATION : NO_FLAGS;

            // Construct the stack for the current state.
            std::vector<abbreviated_task_info> new_stack;
            size_t group_index = 0;
            for (auto& group : app_ctx.instance->task_groups)
            {
                if (is_gettable(groups[group_index]->state))
                {
                    push_task_stack_for_group(new_stack,
                        get(groups[group_index]->state), group_index);
                }
                ++group_index;
            }

            // Diff the old stack with the new one and analyze the changes.
            auto diff = compute_value_diff(to_value(old_stack), to_value(new_stack));
            // If the diff contains UPDATE operations, this must be a case where a task
            // was popped and then another was pushed as part of the same operation.
            // (We only support the case where it's one of each.)
            if (std::any_of(
                    diff.begin(), diff.end(),
                    [ ](auto const& item)
                    {
                        return item.op == value_diff_op::UPDATE;
                    }))
            {
                assert(old_stack.size() == new_stack.size());
                // Pop the old task.
                initiate_pop(data->stack);
                convert_task_to_phantom(*data, old_stack.size() - 1);
                request_state_write_back(app_ctx);
                pop(data->stack);
                // Push the new one.
                push_task_to_stack(app_ctx, *data, new_stack.back());
            }
            else
            {
                unsigned n_pops = 0;
                bool animation_reset = false;
                for (auto const& i : diff)
                {
                    // All changes should either be insertions of new tasks or
                    // deletions of old tasks at the end of the stack.
                    assert(i.path.size() > 0);
                    if (i.path.size() == 1)
                    {
                        size_t index = cradle::from_value<size_t>(i.path.front());
                        switch (i.op)
                        {
                         case value_diff_op::DELETE:
                          {
                            if (data->stack.tasks.size() > index)
                            {
                                convert_task_to_phantom(*data, index);
                                ++n_pops;
                            }
                            break;
                          }
                         case value_diff_op::INSERT:
                          {
                            if (!animation_reset)
                            {
                                reset_animation(data->stack);
                                animation_reset = true;
                            }
                            assert(index == data->stack.tasks.size());
                            push_task_to_stack(app_ctx, *data, new_stack[index], flags);
                            break;
                          }
                         case value_diff_op::UPDATE:
                            assert(0);
                            break;
                        }
                    }
                }
                if (n_pops != 0)
                {
                    // If a task is getting popped off, request a state write back
                    // to make sure its state is saved.
                    request_state_write_back(app_ctx);

                    initiate_pop(data->stack);
                    for (unsigned i = 0; i != n_pops; ++i)
                        pop(data->stack);
                }
            }

            // Update the cache.
            data->cache.groups.resize(n_groups);
            for (size_t i = 0; i != n_groups; ++i)
            {
                if (!is_valid(data->cache.groups[i]) &&
                    is_gettable(groups[i]->state))
                {
                    set(data->cache.groups[i], get(groups[i]->state));
                }
            }
            data->cache.stack = new_stack;
        }

        update_gui_task_stack(ctx, data->stack);

        // If there are no phantom tasks in the stack, then it should be safe to clear
        // out the phantom task groups.
        if (!std::any_of(
                data->stack.tasks.begin(), data->stack.tasks.end(),
                [ ](auto const& task_storage)
                {
                    return task_storage.task.is_phantom;
                }))
        {
            app_ctx.instance->phantom_task_groups.clear();
        }
    }
    return data->stack;
}

}
