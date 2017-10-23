#ifndef CRADLE_GUI_GENERIC_TASKS_HPP
#define CRADLE_GUI_GENERIC_TASKS_HPP

#include <cradle/gui/common.hpp>
#include <alia/ui/utilities/timing.hpp>
#include <alia/ui/library/controls.hpp>
#include <cradle/gui/widgets.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

// This file provides an implementation of Astroid's UI task stack concept
// using a generic definition of tasks.
//
// Tasks must provide the following interface.
//
// Task::id_type is a copyable type that can be compared for equality.
// Also, its default constructor must produce a value that's distinguishable
// from other (explicitly constructed) values.
//
// get_id(Task task) yields an ID of type Task::id_type that uniquely
// identifies the given task.
//
// do_title(gui_context& ctx, Task task) shows the title of the task.
//
// do_task_control_ui(gui_context& ctx, Task task) does the given task's
// control UI (i.e., the UI in Astroid's left pane).
//
// do_task_display_ui(gui_context& ctx, Task task) does the given task's
// display UI (i.e., the UI in Astroid's main pane).
//
// Task::group_id_type is a copyable type that can be compared for equality.
//
// get_group_id(Task task) yields an ID of type Task::group_id_type that
// uniquely identifies the given task.

namespace cradle {

template<class Task>
struct gui_task_stack_animation_state
{
    // the first task that is transitioning OFF OF the stack
    // If this is Task::id_type(), then there is no such task.
    typename Task::id_type first_popping_task;
    // the first task that is transitioning ON TO the stack
    // If this is Task::id_type(), then there is no such task.
    // Note that if there are tasks transitioning in both directions, the popping tasks
    // will always be before the pushing tasks in the list.
    typename Task::id_type first_pushing_task;
    float y;
    value_smoother<float> y_smoother;
};

template<class Task>
struct generic_task_storage
{
    Task task;
    data_block title_ui_block, control_ui_block, display_ui_block;
};

template<class Task>
struct generic_gui_task_stack
{
    bool headers_expanded = false;
    gui_task_stack_animation_state<Task> animation;
    boost::ptr_vector<generic_task_storage<Task> > tasks;
};

template<class Task>
bool
is_empty(generic_gui_task_stack<Task>& stack)
{
    return stack.tasks.empty();
}

template<class Task>
typename boost::ptr_vector<generic_task_storage<Task> >::iterator
get_first_popping_task(generic_gui_task_stack<Task>& stack)
{
    for (auto i = stack.tasks.begin(); i != stack.tasks.end(); ++i)
    {
        if (get_id(i->task) == stack.animation.first_popping_task)
        {
            return i;
        }
    }
    return stack.tasks.end();
}

template<class Task>
typename boost::ptr_vector<generic_task_storage<Task> >::iterator
get_first_pushing_task(generic_gui_task_stack<Task>& stack)
{
    for (auto i = stack.tasks.begin(); i != stack.tasks.end(); ++i)
    {
        if (get_id(i->task) == stack.animation.first_pushing_task)
        {
            return i;
        }
    }
    return stack.tasks.end();
}

template<class Task>
typename boost::ptr_vector<generic_task_storage<Task> >::iterator
get_first_transitioning_task(generic_gui_task_stack<Task>& stack)
{
    for (auto i = stack.tasks.begin(); i != stack.tasks.end(); ++i)
    {
        if (get_id(i->task) == stack.animation.first_popping_task ||
            get_id(i->task) == stack.animation.first_pushing_task)
        {
            return i;
        }
    }
    return stack.tasks.end();
}

// Get the last task in the stack that's not currently animating.
// Note that this is different from get_first_transitioning_task because when there are
// tasks that are transitioning in both directions at the same time, the popping tasks
// aren't actually animated.
template<class Task>
typename boost::ptr_vector<generic_task_storage<Task> >::iterator
get_first_animated_task(generic_gui_task_stack<Task>& stack)
{
    if (is_pushing(stack))
    {
        return get_first_pushing_task(stack);
    }
    else if (is_popping(stack))
    {
        return get_first_popping_task(stack);
    }
    else
    {
        return stack.tasks.end();
    }
}

template<class Task>
void reset_animation(generic_gui_task_stack<Task>& stack)
{
    if (is_popping(stack))
    {
        stack.tasks.erase(get_first_popping_task(stack), get_first_pushing_task(stack));
    }
    stack.animation.first_popping_task = Task::id_type();
    stack.animation.first_pushing_task = Task::id_type();
}

template<class Task>
bool is_pushing(generic_gui_task_stack<Task>& stack)
{
    return stack.animation.first_pushing_task != Task::id_type();
}

template<class Task>
bool is_popping(generic_gui_task_stack<Task>& stack)
{
    return stack.animation.first_popping_task != Task::id_type();
}

template<class Task>
bool is_animating(generic_gui_task_stack<Task>& stack)
{
    return is_pushing(stack) || is_popping(stack);
}

// Get the last task in the stack that's neither pushing nor popping.
template<class Task>
generic_task_storage<Task>&
get_last_fixed_task(generic_gui_task_stack<Task>& stack)
{
    assert(
        !stack.tasks.empty() &&
        stack.animation.first_pushing_task !=
            get_id(stack.tasks.front().task) &&
        stack.animation.first_popping_task !=
            get_id(stack.tasks.front().task));
    auto i = get_first_transitioning_task(stack);
    --i;
    return *i;
}

// Get the last task in the stack that's not currently animating.
template<class Task>
generic_task_storage<Task>&
get_last_unanimated_task(generic_gui_task_stack<Task>& stack)
{
    assert(!stack.tasks.empty());
    auto i = get_first_animated_task(stack);
    assert(i != stack.tasks.begin());
    --i;
    return *i;
}

// Get the "foreground" task. The foreground task is defined (perhaps a bit subtly) as the
// task that will be in front of all others AFTER the current transitions are completed.
template<class Task>
generic_task_storage<Task>&
get_foreground_task(generic_gui_task_stack<Task>& stack)
{
    if (is_pushing(stack))
    {
        return stack.tasks.back();
    }
    else
    {
        return get_last_fixed_task(stack);
    }
}

// Is the given task in the foreground? (See above for "foreground" definition.)
template<class Task, class Id>
bool
is_task_in_foreground(generic_gui_task_stack<Task>& stack, Id const& task_id)
{
    return get_id(get_foreground_task(stack).task) == task_id;
}

// Clear cached data from the UI data blocks associated with any tasks that
// aren't currently active.
template<class Task>
void clear_inactive_task_data_block_caches(generic_gui_task_stack<Task>& stack)
{
    if (!stack.tasks.empty())
    {
        auto const* back_task = &stack.tasks.back();
        auto const* last_unanimated_task =
            is_animating(stack) ? &get_last_unanimated_task(stack) : 0;
        for (auto& task : stack.tasks)
        {
            if (&task != back_task && &task != last_unanimated_task)
            {
                clear_cached_data(task.control_ui_block);
                clear_cached_data(task.display_ui_block);
            }
        }
    }
}

template<class Task>
void update_gui_task_stack(gui_context& ctx, generic_gui_task_stack<Task>& stack)
{
    gui_task_stack_animation_state<Task>& animation = stack.animation;
    if (is_animating(stack))
    {
        if (is_pushing(stack))
        {
            animation.y =
                smooth_raw_value(ctx, animation.y_smoother, 0.f,
                    animated_transition(ease_out_curve, 300));
            if (animation.y == 0.f)
            {
                reset_animation(stack);
            }
        }
        else
        {
            animation.y =
                smooth_raw_value(ctx, animation.y_smoother, 1.f,
                    animated_transition(ease_in_curve, 300));
            if (animation.y == 1.f)
            {
                reset_animation(stack);
            }
        }
    }
    clear_inactive_task_data_block_caches(stack);
};

ALIA_DEFINE_FLAG_TYPE(push_gui_task)
ALIA_DEFINE_FLAG(push_gui_task, 0x1, PUSH_UI_TASK_NO_ANIMATION)

template<class Task>
void push_task(generic_gui_task_stack<Task>& stack, Task const& task,
    push_gui_task_flag_set flags = NO_FLAGS)
{
    auto* storage = new generic_task_storage<Task>;
    storage->task = task;
    stack.tasks.push_back(storage);
    if (!(flags & PUSH_UI_TASK_NO_ANIMATION))
    {
        stack.animation.y = 1.f;
        reset_smoothing(stack.animation.y_smoother, stack.animation.y);
        if (!is_pushing(stack))
        {
            stack.animation.first_pushing_task = get_id(task);
        }
    }
}

template<class Task>
void initiate_pop(generic_gui_task_stack<Task>& stack)
{
    reset_animation(stack);
    stack.animation.y = 0.f;
    reset_smoothing(stack.animation.y_smoother, stack.animation.y);
}

template<class Task>
void pop(generic_gui_task_stack<Task>& stack)
{
    stack.animation.first_popping_task = get_id(get_last_fixed_task(stack).task);
}

// Pop a single task off the stack.
template<class Task>
void pop_task(generic_gui_task_stack<Task>& stack)
{
    initiate_pop(stack);
    pop(stack);
}

// Given a stack and a task within that stack, this will pop tasks off the
// stack until the specified task is in the foreground.
template<class Task, class Id>
void uncover_task(generic_gui_task_stack<Task>& stack, Id const& task_id)
{
    initiate_pop(stack);
    while (!is_task_in_foreground(stack, task_id))
        pop(stack);
}

template<class Task>
size_t get_active_task_count(generic_gui_task_stack<Task>& stack)
{
    size_t n_active_tasks = stack.tasks.size();
    // Tasks that are transitioning out don't count as active, so subtract those.
    if (is_popping(stack))
    {
        if (is_pushing(stack))
        {
            n_active_tasks -=
                std::distance(
                    get_first_popping_task(stack),
                    get_first_pushing_task(stack));
        }
        else
        {
            n_active_tasks -=
                std::distance(
                    get_first_popping_task(stack),
                    stack.tasks.end());
        }
    }
    return n_active_tasks;
}

enum class gui_task_stack_event
{
    NONE,
    UNCOVER_TASK
};

template<class Task>
struct gui_task_stack_result
{
    gui_task_stack_event event;
    typename Task::id_type id;
};

template<class Task>
void do_task_header(gui_context& ctx, generic_gui_task_stack<Task>& stack,
    generic_task_storage<Task>& storage, bool uncoverable,
    gui_task_stack_result<Task>* result)
{
    auto& task = storage.task;
    bool is_foreground = is_task_in_foreground(stack, get_id(task));
    clickable_panel header(ctx,
        select_accessor(in(is_foreground),
            text("foreground-task-header"),
            text("background-task-header")), UNPADDED);
    if (uncoverable && !is_foreground && header.clicked())
    {
        result->event = gui_task_stack_event::UNCOVER_TASK;
        result->id = get_id(task);
    }
    {
        scoped_data_block data_block(ctx, storage.title_ui_block);
        do_title(ctx, task);
    }
}

template<class Task>
void
do_animated_task_headers(gui_context& ctx, generic_gui_task_stack<Task>& stack,
    gui_task_stack_result<Task>* result)
{
    alia_for (auto i = get_first_animated_task(stack);  i != stack.tasks.end(); ++i)
    {
        // Add a background panel so that you can't see the controls of the
        // task behind this one as this one is animating.
        panel background(ctx, text("background"), UNPADDED, PANEL_NO_INTERNAL_PADDING);
        do_task_header(ctx, stack, *i, false, result);
    }
    alia_end
}

template<class Task>
size_t count_unanimated_tasks(generic_gui_task_stack<Task>& stack)
{
    return std::distance(stack.tasks.begin(), get_first_animated_task(stack));
}

// Count the number of tasks in the bottommost group.
template<class Task>
size_t count_tasks_in_bottom_group(generic_gui_task_stack<Task>& stack)
{
    // Starting at the last active task, iterate backwards through the list of tasks and
    // count how many have the same group ID as that one.
    size_t count = 0;
    typename Task::group_id_type bottom_group_id;
    // First process the pushing tasks.
    {
        auto i = stack.tasks.end();
        auto first_pushing = get_first_pushing_task(stack);
        while (i != first_pushing)
        {
            --i;
            if (count == 0)
            {
                bottom_group_id = get_group_id(i->task);
                ++count;
            }
            else if (get_group_id(i->task) == bottom_group_id)
            {
                ++count;
            }
        }
    }
    // Now process the tasks that aren't transitioning at all.
    {
        auto i = get_first_transitioning_task(stack);
        while (i != stack.tasks.begin())
        {
            --i;
            if (count == 0)
            {
                bottom_group_id = get_group_id(i->task);
                ++count;
            }
            else if (get_group_id(i->task) == bottom_group_id)
            {
                ++count;
            }
        }
    }
    return count;
}

// Get the number of tasks whose headers are always visible. This is now simply the number
// of tasks in the bottommost group minus one. (The top task in the bottommost group
// doesn't give any context that's not already in the header.)
template<class Task>
size_t
count_footer_tasks(generic_gui_task_stack<Task>& stack)
{
    return count_tasks_in_bottom_group(stack) - 1;
}

template<class Task>
size_t
count_hidden_tasks(generic_gui_task_stack<Task>& stack)
{
    return
        stack.headers_expanded ?
            0 :
            get_active_task_count(stack) - count_footer_tasks(stack);
}

template<class Task>
void
do_fixed_task_headers(
    gui_context& ctx, generic_gui_task_stack<Task>& stack,
    gui_task_stack_result<Task>* result)
{
    size_t n_active_tasks = get_active_task_count(stack);

    size_t n_footer_tasks = count_footer_tasks(stack);

    float n_hidden_tasks = smooth_raw_value(ctx, float(count_hidden_tasks(stack)));

    do_top_panel_expander(ctx,
        inout(&stack.headers_expanded),
        FILL | UNPADDED,
        auto_id);

    {
        collapsible_content container(ctx,
            1.f - n_hidden_tasks / float(count_unanimated_tasks(stack)));
        alia_if (container.do_content())
        {
            alia_for (auto i = stack.tasks.begin();
                i != get_first_transitioning_task(stack); ++i)
            {
                do_task_header(ctx, stack, *i, true, result);
            }
            alia_end
        }
        alia_end
    }
}

template<class Task>
void
do_task_controls(gui_context& ctx, generic_task_storage<Task>& storage)
{
    scoped_data_block data_block(ctx, storage.control_ui_block);
    panel p(ctx, text("content"), UNPADDED | GROW, PANEL_NO_INTERNAL_PADDING);
    do_task_control_ui(ctx, storage.task);
}

template<class Task>
void
do_animated_panel_stack(gui_context& ctx, generic_gui_task_stack<Task>& stack,
    gui_task_stack_result<Task>* result)
{
    alia_if (is_animating(stack))
    {
        column_layout column(ctx, GROW);
        auto content_region = column.region();

        scoped_clip_region clipper;
        scoped_transformation transform;
        alia_untracked_if (!is_refresh_pass(ctx))
        {
            clipper.begin(*get_layout_traversal(ctx).geometry);
            clipper.set(box<2,double>(content_region));

            transform.begin(*get_layout_traversal(ctx).geometry);
            transform.set(translation_matrix(make_vector<double>(0,
                round_to_layout_scalar(stack.animation.y *
                    content_region.size[1]))));
        }
        alia_end

        do_animated_task_headers(ctx, stack, result);
        do_task_controls(ctx, stack.tasks.back());
    }
    alia_end
}

template<class Task>
gui_task_stack_result<Task>
do_task_stack_controls(gui_context& ctx, generic_gui_task_stack<Task>& stack)
{
    gui_task_stack_result<Task> result;
    result.event = gui_task_stack_event::NONE;
    alia_if (!is_empty(stack))
    {
        column_layout column(ctx, GROW);
        do_fixed_task_headers(ctx, stack, &result);
        {
            layered_layout layering(ctx, GROW);
            {
                column_layout column(ctx, GROW);
                // If we're popping and pushing tasks at the same time, the headers for
                // any popping tasks should be part of the background (if they're
                // visible). Note that this visibility logic is only correct for a single
                // popping task, but that's fine for now.
                alia_if (is_pushing(stack) && is_popping(stack) &&
                    count_unanimated_tasks(stack) > count_hidden_tasks(stack))
                {
                    alia_for (auto i = get_first_popping_task(stack);
                        i != get_first_pushing_task(stack); ++i)
                    {
                        do_task_header(ctx, stack, *i, false, &result);
                    }
                    alia_end
                }
                alia_end
                // If we're in an animated transition, the last fixed task is actually in
                // the background, so we don't want the user interacting with it. Thus, we
                // need to filter out input events here (except for focus loss events).
                if (!is_animating(stack) || ctx.event->category != INPUT_CATEGORY ||
                    ctx.event->type == FOCUS_LOSS_EVENT)
                {
                    do_task_controls(ctx, get_last_unanimated_task(stack));
                }
            }
            do_animated_panel_stack(ctx, stack, &result);
        }
    }
    alia_else
    {
        panel background(ctx, text("background"), UNPADDED | GROW);
    }
    alia_end
    return result;
}

template<class Task>
void
do_task_display(dataless_ui_context& ctx, generic_task_storage<Task>& storage)
{
    gui_context& gui_ctx = static_cast<gui_context&>(ctx);
    scoped_data_block data_block(gui_ctx, storage.display_ui_block);
    do_task_display_ui(gui_ctx, storage.task);
}

template<class Task>
void
do_task_stack_display(gui_context& ctx, generic_gui_task_stack<Task>& stack)
{
    alia_if (!is_empty(stack))
    {
        alia_if (is_animating(stack))
        {
            do_task_display(ctx, get_last_unanimated_task(stack));
            {
                scoped_surface_opacity opacity(ctx, 1.f - stack.animation.y);
                do_task_display(ctx, stack.tasks.back());
            }
        }
        alia_else
        {
            do_task_display(ctx, stack.tasks.back());
        }
        alia_end
    }
    alia_else
    {
        do_empty_display_panel(ctx, GROW);
    }
    alia_end
}

}

#endif
