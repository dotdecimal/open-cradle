#ifndef CRADLE_GUI_APP_TASK_INTERFACE_HPP
#define CRADLE_GUI_APP_TASK_INTERFACE_HPP

#include <cradle/gui/common.hpp>
#include <cradle/gui/app/interface.hpp>

// This file contains functionality useful when implementing UI tasks.

namespace cradle {

template<class State>
struct gui_task_context
{
    string id;
    indirect_accessor<optional<string> > active_subtask;
    indirect_accessor<State> state;
};

template<class State>
gui_task_context<State>
cast_task_context(
    gui_context& ctx,
    gui_task_context<value> const& untyped_ctx,
    any& conversion_data)
{
    gui_task_context<State> typed_ctx;
    typed_ctx.id = untyped_ctx.id;
    typed_ctx.active_subtask = untyped_ctx.active_subtask;
    typedef value_accessor_type_applier_data<State> conversion_data_type;
    conversion_data_type* typed_conversion_data =
        any_cast<conversion_data_type>(conversion_data);
    if (!typed_conversion_data)
    {
        conversion_data = conversion_data_type();
        typed_conversion_data =
            any_cast<conversion_data_type>(conversion_data);
    }
    typed_ctx.state = make_indirect(ctx,
        apply_value_type<State>(ctx, typed_conversion_data,
            untyped_ctx.state));
    return typed_ctx;
}

// All UI tasks must implement this interface.
struct gui_task_interface
{
    virtual ~gui_task_interface() {}

    // Show the title of this task.
    virtual void
    untyped_do_title(gui_context& ctx, app_context& app_ctx,
            gui_task_context<value> const& task, any& state_conversion_data) = 0;

    // Do the control UI for this task.
    // This is the UI that appears in Astroid's left pane.
    virtual void
    untyped_do_control_ui(gui_context& ctx, app_context& app_ctx,
            gui_task_context<value> const& task, any& state_conversion_data) = 0;

    // Do the display UI for this task.
    // This is the UI that appears in Astroid's main pane.
    virtual void
    untyped_do_display_ui(gui_context& ctx, app_context& app_ctx,
            gui_task_context<value> const& task, any& state_conversion_data) = 0;
};

// Do a simple task title.
void do_task_title(gui_context& ctx, accessor<string> const& title);

// Do a task title with both a level and the label of that level.
// An example of this is "PATIENT - Jones" where the level is "PATIENT" and
// the label is "Jones".
void do_task_title(gui_context& ctx, accessor<string> const& level,
    accessor<string> const& label);

// Do the header label for a task group.
void do_task_group_label(gui_context& ctx, accessor<string> const& label);

#define CRADLE_COMMON_UI_TASK_INTERFACE_DECLARATIONS(AppContext, StateType) \
    void \
    do_title(gui_context& ctx, AppContext& app_ctx, \
        gui_task_context<StateType> const& task); \
    void \
    untyped_do_title(gui_context& ctx, cradle::app_context& app_ctx, \
        gui_task_context<value> const& task, any& state_conversion_data) \
    { this->do_title(ctx, dynamic_cast<AppContext&>(app_ctx), \
        cast_task_context<StateType>(ctx, task, state_conversion_data)); } \
    void \
    do_control_ui(gui_context& ctx, AppContext& app_ctx, \
        gui_task_context<StateType> const& task); \
    void \
    untyped_do_control_ui(gui_context& ctx, cradle::app_context& app_ctx, \
        gui_task_context<value> const& task, any& state_conversion_data) \
    { this->do_control_ui(ctx, dynamic_cast<AppContext&>(app_ctx), \
        cast_task_context<StateType>(ctx, task, state_conversion_data)); } \
    void \
    do_display_ui(gui_context& ctx, AppContext& app_ctx, \
        gui_task_context<StateType> const& task); \
    void \
    untyped_do_display_ui(gui_context& ctx, cradle::app_context& app_ctx, \
        gui_task_context<value> const& task, any& state_conversion_data) \
    { this->do_display_ui(ctx, dynamic_cast<AppContext&>(app_ctx), \
        cast_task_context<StateType>(ctx, task, state_conversion_data)); }

#define CRADLE_SIMPLE_UI_TASK_INTERFACE_DECLARATIONS(AppContext, StateType) \
    CRADLE_COMMON_UI_TASK_INTERFACE_DECLARATIONS(AppContext, StateType)

#define CRADLE_DEFINE_SIMPLE_UI_TASK(task_name, AppContext, StateType) \
    struct task_name : gui_task_interface \
    { \
        CRADLE_SIMPLE_UI_TASK_INTERFACE_DECLARATIONS( \
            AppContext, StateType) \
    };

// Push a new task group.
// The new task group structure will assume ownership of the controller
// pointer.
void push_task_group(app_context& app_ctx, task_group_controller* controller);

// Get the raw state of the specified task.
gui_task_state
get_raw_task_state(app_context& app_ctx, string const& task_id);

// Push a singleton task onto the stack.
// A singleton task is one that only has one instance per app instance
// (e.g., an overview task).
// The ID of a singleton task is its task_type.
void push_singleton_task(
    app_context& app_ctx,
    string const& parent_task_id,
    string const& task_type,
    value const& initial_state);

// This will pop a singleton task off the stack.
// When popping a singleton task, the state associated with the task is NOT
// cleaned up. It's left around for the next time that task is invoked.
void pop_singleton_task(app_context& app_ctx, string const& task_id);

// Push a new task onto the stack.
// The return value is the ID of the new task.
string
push_task(
    app_context& app_ctx,
    string const& parent_task_id,
    string const& task_type,
    value const& initial_state);

// Uncover a task in the stack.
// This will pop all tasks below the specified one.
void uncover_task(app_context& app_ctx, string const& task_id);

// Uncover a task group in the stack.
// This will pop all groups below the specified one.
void uncover_task_group(app_context& app_ctx, size_t group_index);

// This is called by a producer task when it has completed and wants to return
// the value it's produced back to its parent.
// This will inform the parent of the new value and also pop the task off the
// stack.
void produce_value(app_context& app_ctx,
    string const& producer_task_id, value const& value);

// Activate an existing task. (Push it back onto the stack.)
void activate_task(app_context& app_ctx, string const& parent_task_id,
    string const& task_id);

// Cancel the given task and pop it off the stack.
void cancel_task(app_context& app_ctx, string const& task_id);

// This is called by a non-producer task when it has completed.
void complete_task(app_context& app_ctx, string const& task_id);

api(enum internal)
enum class subtask_event_type
{
    TASK_COMPLETED,
    VALUE_PRODUCED,
    TASK_CANCELED,
};

api(struct internal)
struct subtask_event
{
    subtask_event_type type;
    string task_id;
    // only valid for VALUE_PRODUCED events
    cradle::value value;
};

optional<subtask_event>
get_subtask_event(app_context& app_ctx, string const& subtask_id);

// These are for tasks that don't require a special context.

void register_app_task(string const& id, gui_task_interface* implementation);

struct gui_task_implementation_table;

gui_task_implementation_table&
get_task_implementation_table(app_context& app_ctx);

}

#endif
