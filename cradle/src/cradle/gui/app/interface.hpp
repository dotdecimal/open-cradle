#ifndef CRADLE_GUI_APP_INTERFACE_HPP
#define CRADLE_GUI_APP_INTERFACE_HPP

#include <cradle/gui/common.hpp>

// This file defines the interface that the app provides to lower-level code.

namespace cradle {

struct app_instance;

api(struct internal)
struct gui_task_state
{
    string type;

    // task-specific state
    value state;

    optional<string> active_subtask;

    // statistics about the history of subtask activity within this task
    unsigned completed_subtask_count, canceled_subtask_count,
        open_subtask_count;
};

typedef std::map<string,gui_task_state> gui_task_state_map;

api(struct internal)
struct gui_task_group_state
{
    gui_task_state_map tasks;
    string root_id;
};

struct task_group_controller : noncopyable
{
    virtual ~task_group_controller() {}

    // Get the type ID of the root task in the group.
    virtual string get_root_task_type_id() = 0;

    // Get the app context for this task group and its descendants.
    // The app context passed in is the internal one for the parent task group.
    virtual app_context&
    get_internal_app_context(gui_context& ctx, app_context& app_ctx) = 0;

    // The app context passed to the following functions is the internal
    // context for this group.

    // Get an accessor to the state for this task group.
    virtual indirect_accessor<gui_task_group_state>
    get_state_accessor(gui_context& ctx, app_context& app_ctx) = 0;

    // Do the header label for task group.
    // This is used in the main UI header for returning to that task group.
    virtual void
    do_header_label(gui_context& ctx, app_context& app_ctx) = 0;

    // Do the content that should always be at the top of the task stack.
    virtual void
    do_task_header_content(gui_context& ctx, app_context& app_ctx) {}
};

typedef alia__shared_ptr<task_group_controller> task_group_controller_ptr;

struct task_group
{
    string id;
    task_group_controller_ptr controller;
    // This is used to store the internal app context for this group.
    // (It's regenerated every pass, but it's used at different places within
    // each pass, so rather than regenerate it in each of those places, we
    // generate it once and remember it here.)
    app_context* app_ctx;
    // The same applies to the state accessor.
    indirect_accessor<gui_task_group_state> state;
};

gui_task_group_state
make_initial_task_group_state(app_instance& instance,
    string const& root_task_type_id);

// Push a new task group.
// The new task group structure will assume ownership of the controller
// pointer.
void push_task_group(app_instance& instance, task_group_controller* controller);

struct app_context
{
    virtual ~app_context() {}

    app_instance* instance;
};

typedef alia__shared_ptr<task_group> task_group_ptr;

std::vector<task_group_ptr>& get_task_groups(app_context& app_ctx);

// Reset the task groups for this app instance so that it only contains the
// root group for the app. (It will contain a NEW instance of that group.)
void reset_task_groups(app_context& app_ctx);

// Generate a UUID.
string generate_unique_id(app_context& app_ctx);

// Request that the UI write back any locally buffered state to its proper
// location.
void request_state_write_back(app_context& app_ctx);

// This event signals that the UI should immediately write back any buffered
// state to its proper location.
struct state_write_back_event : ui_event
{
    state_write_back_event()
      : ui_event(NO_CATEGORY, CUSTOM_EVENT)
    {}
};

// Given an accessor to some remote_state, this yields an accessor to a local copy that
// will only be written out on a timer tick (every 15 seconds) or when an explicit
// writeback event occurs.
template<class StateType>
indirect_accessor<StateType>
buffer_state_writebacks(
    gui_context& ctx,
    accessor<StateType> const& remote_state)
{
    // local copy
    auto local = get_state(ctx, optional<StateType>());

    if (is_refresh_pass(ctx) && !get(local) && is_gettable(remote_state))
    {
        set(local, some(get(remote_state)));
    }

    // Periodically write back changes to the remote_state accessor.
    timer t(ctx);
    if (!t.is_active())
    {
        t.start(15000);
    }
    // Also write back if we get a state_write_back_event or a
    // shutdown_event.
    if (t.triggered() || detect_event<state_write_back_event>(ctx) ||
        detect_event(ctx, SHUTDOWN_EVENT))
    {
        if (get(local) && is_gettable(remote_state) &&
            get(get(local)) != get(remote_state))
        {
            set(remote_state, get(get(local)));
        }
    }

    return make_indirect(ctx, unwrap_optional(local));
}

api(struct internal)
struct app_info
{
    // the account associated with this app in thinknode
    string thinknode_app_account;

    // the ID of this app in thinknode
    string thinknode_app_id;

    // the version ID of this app in thinknode
    string thinknode_version_id;

    // the name to display for this app
    string app_name;

    // the version number of the local client executable for this app
    string local_version_id;

    // the application bar code id
    string app_barcode_id;

    // the application bar code svg
    string app_barcode;

    // the applications logo svg
    string logo;
};

// Get the general info for this app.
indirect_accessor<app_info>
get_app_info(gui_context& ctx, app_context& app_ctx);

// This is a version of alia's app_window_state structure that has a regular
// CRADLE interface.
api(struct internal)
struct regular_app_window_state
{
    optional<vector<2,int> > position;
    vector<2,int> size;
    bool maximized, full_screen;
};

api(struct internal)
struct app_config
{
    optional<string> username;

    optional<string> realm_id;

    regular_app_window_state window_state;

    float control_panel_width, display_controls_width;
    layout_vector scroll_position;

    float ui_magnification_factor;
};

// Get the config for this app.
indirect_accessor<app_config>
get_app_config(gui_context& ctx, app_context& app_ctx);

}

#endif
