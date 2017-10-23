#ifndef CRADLE_GUI_APP_GUI_TASKS_HPP
#define CRADLE_GUI_APP_GUI_TASKS_HPP

#include <cradle/gui/generic_tasks.hpp>
#include <cradle/gui/task_interface.hpp>

namespace cradle {

// Note that this whole task implementation is a mess, as exemplified by the
// gui_task_with_context structure. It works, but it suffers from being
// structured according to a much earlier and different UI design and then
// being patched and contorted multiple times to accomodate design changes.
// Once we have time to revisit the plan overview UI design and the way that
// tasks and breadcrumbs work, it'd be worth also revisiting the code design.

struct gui_task_implementation_table
{
    std::map<string,alia__shared_ptr<gui_task_interface> > implementations;
};

void register_task(gui_task_implementation_table& table, string const& id,
    alia__shared_ptr<gui_task_interface> const& implementation);

struct unimplemented_task : exception
{
    unimplemented_task(string const& task_id)
      : exception("unimplemented task: " + task_id)
      , task_id_(new string(task_id))
    {}
    ~unimplemented_task() throw() {}

    string const& task_id() const
    { return *task_id_; }

 private:
    alia__shared_ptr<string> task_id_;
};

gui_task_interface*
find_task_implementation(
    gui_task_implementation_table const& table, string const& task_id);

struct gui_task_with_context
{
    typedef string id_type;
    typedef size_t group_id_type;
    app_context** context_pointer;
    task_group_ptr group;
    size_t group_index;
    gui_task_implementation_table const* table;
    bool is_phantom;
    string id, type;
    state<gui_task_state> phantom;
    any state_conversion_data;
    id_change_minimization_data<gui_task_state> id_change_minimization;
};

gui_task_interface&
get_task_implementation(gui_context& ctx, gui_task_with_context const& task);

gui_task_context<value>
make_task_context(gui_context& ctx, gui_task_with_context& task);

static inline string const&
get_id(gui_task_with_context const& task)
{
    return task.id;
}

static inline size_t
get_group_id(gui_task_with_context const& task)
{
    return task.group_index;
}

void do_title(gui_context& ctx, gui_task_with_context& task);

void do_task_control_ui(gui_context& ctx, gui_task_with_context& task);

void do_task_display_ui(gui_context& ctx, gui_task_with_context& task);

api(struct internal)
struct abbreviated_task_info
{
    string id;
    string type;
    size_t group_index;
};

struct gui_task_stack_cache
{
    std::vector<abbreviated_task_info> stack;
    std::vector<keyed_data<gui_task_group_state> > groups;
};

struct gui_task_stack_data
{
    gui_task_stack_cache cache;
    generic_gui_task_stack<gui_task_with_context> stack;
};

generic_gui_task_stack<gui_task_with_context>&
get_gui_task_stack(gui_context& ctx, app_context& app_ctx);

}

#endif
