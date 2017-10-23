#include <cradle/gui/task_interface.hpp>
#include <cradle/gui/app/gui_tasks.hpp>
#include <cradle/gui/app/internals.hpp>
#include <set>

namespace cradle {

void do_task_title(gui_context& ctx, accessor<string> const& title)
{
    do_paragraph(ctx, add_fallback_value(ref(&title), text(" ")));
}

void do_task_title(gui_context& ctx, accessor<string> const& level,
    accessor<string> const& label)
{
    flow_layout paragraph(ctx);
    do_styled_text(ctx, text("level"), level);
    do_text(ctx, text(" - "));
    do_text(ctx, label);
}

void do_task_group_label(gui_context& ctx, accessor<string> const& label)
{
    do_text(ctx, label);
}

// TASK STACK GC

void static
add_gc_task_reference(gui_task_state_map& task_map,
    std::set<string>& referenced_tasks, string const& task_id);

void static
scan_for_task_references(gui_task_state_map& task_map,
    std::set<string>& referenced_tasks, value const& v)
{
    switch (v.type())
    {
     case value_type::STRING:
        add_gc_task_reference(task_map, referenced_tasks, cast<string>(v));
        break;
     case value_type::LIST:
        for (auto const& i : cast<value_list>(v))
            scan_for_task_references(task_map, referenced_tasks, i);
        break;
     case value_type::MAP:
        for (auto const& i : cast<value_map>(v))
            scan_for_task_references(task_map, referenced_tasks, i.second);
        break;
    }
}

void static
add_gc_task_reference(gui_task_state_map& task_map,
    std::set<string>& referenced_tasks, string const& task_id)
{
    auto i = task_map.find(task_id);
    if (i == task_map.end())
        return; // This isn't an actual reference to a task.

    if (referenced_tasks.find(task_id) != referenced_tasks.end())
        return; // This task has already been marked and scanned.

    // Mark it.
    referenced_tasks.insert(task_id);

    // Scan its contents.
    {
        gui_task_state const& state = i->second;
        if (state.active_subtask)
        {
            add_gc_task_reference(task_map, referenced_tasks,
                get(state.active_subtask));
        }
        scan_for_task_references(task_map, referenced_tasks, state.state);
    }
}

// Scan the task state map and remove any tasks that aren't referenced by
// the root. This scans each individual task's state as a dynamic value,
// and since task references are currently stored as simple strings, this
// could erroneously keep tasks around past when they're needed, but that's
// not a big problem (and the odds of it happening are small).
void static
gc_task_state_map(gui_task_group_state& state)
{
    // Scan through and record all the referenced tasks.
    std::set<string> referenced_tasks;
    auto& task_map = state.tasks;
    add_gc_task_reference(task_map, referenced_tasks, state.root_id);

    // Delete any that aren't referenced.
    for (auto i = task_map.begin(); i != task_map.end(); )
    {
        auto next = i;
        ++next;
        if (referenced_tasks.find(i->first) == referenced_tasks.end())
            task_map.erase(i);
        i = next;
    }
}

// TASK STACK MANIPULATION

static inline gui_task_group_state const&
get_group_state(task_group const& group)
{
    return get(group.state);
}

static inline void
set_group_state(task_group& group, gui_task_group_state const& state)
{
    set(group.state, state);
}

size_t static
find_task_group(app_context& app_ctx, string const& task_id)
{
    auto const& groups = get_task_groups(app_ctx);
    size_t n_groups = groups.size();
    for (size_t i = 0; i != n_groups; ++i)
    {
        auto const& state = get_group_state(*groups[i]);
        if (state.tasks.find(task_id) != state.tasks.end())
            return i;
    }
    throw exception("internal error: couldn't find task");
}

void push_task_group(app_context& app_ctx, task_group_controller* controller)
{
    auto& instance = *app_ctx.instance;
    push_task_group(*app_ctx.instance, controller);
}

void pop_task_group(app_context& app_ctx)
{
    auto& instance = *app_ctx.instance;
    // Add it to the front of the list of phantom task groups.
    instance.phantom_task_groups.insert(instance.phantom_task_groups.begin(),
        instance.task_groups.back());
    instance.task_groups.pop_back();
}

gui_task_state
get_raw_task_state(app_context& app_ctx, string const& task_id)
{
    auto const& group =
        *get_task_groups(app_ctx)[find_task_group(app_ctx, task_id)];
    auto state = get_group_state(group);
    auto task = state.tasks.find(task_id);
    assert(task != state.tasks.end());
    return task->second;
}

static inline task_group&
get_bottom_task_group(app_context& app_ctx)
{
    return *get_task_groups(app_ctx).back();
}

void static
activate_task(
    gui_task_group_state& state,
    string const& parent_task_id,
    string const& task_id)
{
    state.tasks[parent_task_id].active_subtask = task_id;
}

string static
push_new_task(
    app_context& app_ctx,
    gui_task_group_state& state,
    string const& parent_task_id,
    string const& task_type,
    value const& initial_state)
{
    gui_task_state new_task;
    new_task.type = task_type;
    new_task.state = initial_state;
    auto task_id = generate_unique_id(app_ctx);
    state.tasks[task_id] = new_task;
    activate_task(state, parent_task_id, task_id);
    ++state.tasks[parent_task_id].open_subtask_count;
    return task_id;
}

void static
pop_task_by_parent_id(gui_task_group_state& state, string const& parent_id,
    bool canceled)
{
    auto& parent_state = state.tasks[parent_id];
    parent_state.active_subtask = none;
    --parent_state.open_subtask_count;
    if (canceled)
        ++parent_state.canceled_subtask_count;
    else
        ++parent_state.completed_subtask_count;
}

void static
pop_task_by_id(gui_task_group_state& state, string const& task_id,
    bool canceled)
{
    // Search for the parent task.
    // Since the task we're looking for is on the stack, we just have to start
    // at the root and follow through the active_subtask references until we
    // find it.
    string parent_id = state.root_id;
    while (1)
    {
        auto task = state.tasks.find(parent_id);
        assert(task != state.tasks.end());
        if (task == state.tasks.end())
            return;
        auto subtask_id = task->second.active_subtask;
        if (!subtask_id)
            return;
        if (get(subtask_id) == task_id)
            break;
        parent_id = get(subtask_id);
    }
    pop_task_by_parent_id(state, parent_id, canceled);
}

void static
delete_task(gui_task_group_state& state, string const& task_id)
{
    state.tasks.erase(task_id);
    gc_task_state_map(state);
}

optional<string> static
find_task_by_type(task_group const& group, string const& type)
{
    auto const& state = get_group_state(group);
    for (auto const& i : state.tasks)
    {
        if (i.second.type == type)
            return some(i.first);
    }
    return none;
}

void push_singleton_task(
    app_context& app_ctx,
    string const& parent_task_id,
    string const& task_type,
    value const& initial_state)
{
    auto& group = get_bottom_task_group(app_ctx);
    auto state = get_group_state(group);
    // Only create a new task if there isn't already one of this type.
    auto task_id = find_task_by_type(group, task_type);
    if (!task_id)
    {
        push_new_task(app_ctx, state, parent_task_id, task_type,
            initial_state);
    }
    else
    {
        activate_task(state, parent_task_id, get(task_id));
    }
    set_group_state(group, state);
}

void pop_singleton_task(app_context& app_ctx, string const& task_id)
{
    auto& group = get_bottom_task_group(app_ctx);
    auto state = get_group_state(group);
    // If we're popping the root task, just delete the group instead.
    if (state.root_id == task_id)
    {
        pop_task_group(app_ctx);
    }
    else
    {
        pop_task_by_id(state, task_id, false);
        set_group_state(group, state);
    }
}

string
push_task(
    app_context& app_ctx,
    string const& parent_task_id,
    string const& task_type,
    value const& initial_state)
{
    auto& group = get_bottom_task_group(app_ctx);
    auto state = get_group_state(group);
    auto task_id =
        push_new_task(app_ctx, state, parent_task_id, task_type,
            initial_state);
    set_group_state(group, state);
    return task_id;
}

void activate_task(app_context& app_ctx, string const& parent_task_id,
    string const& task_id)
{
    auto& group = get_bottom_task_group(app_ctx);
    auto state = get_group_state(group);
    activate_task(state, parent_task_id, task_id);
    set_group_state(group, state);
}

void static
push_task_event(app_context& app_ctx, subtask_event const& event)
{
    app_ctx.instance->task_events.event = event;
}

void static
pop_and_delete_task(app_context& app_ctx, string const& task_id, bool canceled)
{
    auto& group = get_bottom_task_group(app_ctx);
    auto state = get_group_state(group);
    // If we're popping the root task, just delete the group instead.
    if (state.root_id == task_id)
    {
        pop_task_group(app_ctx);
    }
    else
    {
        pop_task_by_id(state, task_id, canceled);
        delete_task(state, task_id);
        set_group_state(group, state);
    }
}

void uncover_task(app_context& app_ctx, string const& task_id)
{
    auto group_index = find_task_group(app_ctx, task_id);
    // Uncover the task's group.
    uncover_task_group(app_ctx, group_index);
    // Set the active subtask for the specified task to none.
    auto& group = *get_task_groups(app_ctx)[group_index];
    auto state = get_group_state(group);
    state.tasks[task_id].active_subtask = none;
    set_group_state(group, state);
}

void uncover_task_group(app_context& app_ctx, size_t group_index)
{
    while (get_task_groups(app_ctx).size() > group_index + 1)
        pop_task_group(app_ctx);
}

void produce_value(app_context& app_ctx,
    string const& producer_task_id, value const& value)
{
    push_task_event(app_ctx,
        subtask_event(subtask_event_type::VALUE_PRODUCED, producer_task_id,
            value));
    pop_and_delete_task(app_ctx, producer_task_id, false);
}

void cancel_task(app_context& app_ctx, string const& task_id)
{
    push_task_event(app_ctx,
        subtask_event(subtask_event_type::TASK_CANCELED, task_id, value()));
    pop_and_delete_task(app_ctx, task_id, true);
}

void complete_task(app_context& app_ctx, string const& task_id)
{
    push_task_event(app_ctx,
        subtask_event(subtask_event_type::TASK_COMPLETED, task_id, value()));
    pop_and_delete_task(app_ctx, task_id, false);
}

optional<subtask_event>
get_subtask_event(app_context& app_ctx, string const& subtask_id)
{
    auto& event = app_ctx.instance->task_events.event;
    if (event && get(event).task_id == subtask_id)
    {
        auto copy = event;
        // Clear the event from the "queue".
        event = none;
        return copy;
    }
    return none;
}

static gui_task_implementation_table the_app_task_table;

gui_task_implementation_table&
get_task_implementation_table(app_context& app_ctx)
{
    return the_app_task_table;
}

void register_app_task(string const& id, gui_task_interface* implementation)
{
    alia__shared_ptr<gui_task_interface> ptr(implementation);
    register_task(the_app_task_table, id, ptr);
}

}
