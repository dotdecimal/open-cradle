#ifndef CRADLE_GUI_APP_INTERNALS_HPP
#define CRADLE_GUI_APP_INTERNALS_HPP

#include <boost/program_options.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <alia/ui/backends/interface.hpp>

#include <cradle/breakpad.hpp>
#include <cradle/gui/app/interface.hpp>
#include <cradle/gui/task_interface.hpp>
#include <cradle/io/services/core_services.hpp>
#include <cradle/io/file.hpp>

// This file defines various data types and utilities that are shared amongst
// app-level code but not intended to be seen by other code.

namespace cradle {

string
read_app_config_file(char const* path, string prop_name);

// APP WINDOW STATE

regular_app_window_state
from_alia(app_window_state const& state);

app_window_state
to_alia(regular_app_window_state const& regular);

// This is called as part of the UI to keep the app window state in sync with
// the actual OS window.
void sync_window_state(ui_context& ctx, app_window& window,
    accessor<regular_app_window_state> const& state);

// APP CONFIG

// Read the app config from the OS.
app_config read_app_config(string const& app_id);

// Write the app config to the OS.
void write_app_config(string const& app_id, app_config const& config);

// This is called as part of the UI to keep the OS's config data in sync with
// the application's.
// It takes care not to write changes back to the OS too frequently.
void sync_app_config(ui_context& ctx, string const& app_id,
    accessor<app_config> const& config);

// SHARED APP CONFIG - config shared by all astroid apps on the workstation

api(struct internal)
struct shared_app_config
{
    optional<file_path> cache_dir;
    // in GB
    int cache_size;

    optional<file_path> crash_dir;
};

// Read the shared app config from the OS.
shared_app_config read_shared_app_config();

// Write the app config to the OS.
void write_shared_app_config(shared_app_config const& config);

// APP CONTROLLER

struct app_controller_interface : noncopyable
{
    virtual ~app_controller_interface() {}

    // Get the general information about this application.
    virtual app_info get_app_info() = 0;

    // Register all the tasks defined by this app.
    virtual void register_tasks() = 0;

    // Get the controller for the root task group.
    // (The caller will assume ownership of the returned object.)
    virtual task_group_controller* get_root_task_group_controller() = 0;

    virtual std::map<char*, char*> get_app_command_line_arguments() = 0;

    virtual void
    process_app_command_line_arguments(
        boost::program_options::variables_map const& vm) = 0;
};

// APP INSTANCE

struct gui_task_event_queue
{
    // The queue only needs to hold one event at a time.
    optional<subtask_event> event;
};

api(enum internal)
enum class app_level_page
{
    APP_CONTENTS,
    APP_INFO,
    SETTINGS,
    NOTIFICATIONS,
    DEV_CONSOLE
};

struct app_instance
{
    alia__shared_ptr<app_controller_interface> controller;

    app_level_page selected_page;

    app_info info;

    string api_url;

    alia__shared_ptr<cradle::gui_system> gui_system;

    state<app_config> config;

    shared_app_config shared_config;

    state<string> username, realm_id;

    state<optional<string> > token;

    string style_file_path;

    crash_reporting_context crash_reporter;

    boost::uuids::random_generator uuid_generator;

    // task stack data

    // data block used for refreshing the task stack, so we can explicitly
    // clear it when we want to
    alia::data_block task_stack_ui_block;
    // list of task groups
    std::vector<task_group_ptr> task_groups;
    // list of groups that are hanging around for animation purposes
    std::vector<task_group_ptr> phantom_task_groups;
    // queue of events that are being passed between tasks
    gui_task_event_queue task_events;

    // If this is set, someone has requested that the UI write back any
    // buffered state it has.
    // (This is set via request_state_write_back().)
    bool state_write_back_requested;

    int return_code;
};

// Initialize an app context that refers to this app instance.
void initialize_app_context(app_context& app_ctx, app_instance& instance);

// Initiate the sign-in process.
void start_sign_in(app_instance& instance,
    string const& user, string const& password);

void sign_in_wih_token(app_instance& instance,
    string const& token);

// Select the given realm for this app instance and initiate a request for
// the associated context.
void select_realm(app_instance& instance, string const& realm_id);

// Set the shared app config.
void set_shared_app_config(app_instance& instance,
    shared_app_config const& config);

// Generate a UUID.
string generate_unique_id(app_instance& instance);

// Reset the task groups for this app instance so that it only contains the
// root group for the app. (It will contain a NEW instance of that group.)
void reset_task_groups(app_instance& instance);

// Go through the stack of task groups and resolve all their app contexts.
void resolve_gui_app_contexts(gui_context& ctx, app_context& root_context);

api(struct internal)
struct ui_performance_metrics
{
    // the number of frames rendered in the last second
    int fps;
    // time of the average refresh over the last second, in microseconds
    int refresh_duration;
};

// Request that the UI refresh as quickly as possible and compute various
// performance metrics.
indirect_accessor<ui_performance_metrics>
compute_performance(ui_context& ctx);

}

#endif
