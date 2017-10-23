#include <cradle/gui/app/internals.hpp>

#include <boost/uuid/uuid_io.hpp>

#include <wx/config.h>
#include <json\json.h>

#include <alia/ui/system.hpp>
#include <alia/ui/utilities.hpp>

#include <cradle/background/system.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/io/generic_io.hpp>

namespace cradle {

string
read_app_config_file(char const* path, string prop_name)
{
    std::ifstream f(path);
    Json::Reader reader;
    Json::Value root;
    // TODO: This conditional is broken, but I didn't bother to fix it. It
    // seems this whole function should just use the normal ways of parsing
    // structures from JSON values.
    // (Or it could even use the existing config system.)
    if (/*f != NULL &&*/ reader.parse(f, root))
    {
        if (!root.isMember(prop_name))
        {
            throw parse_error("Unable to get " + prop_name + " property from " + to_string(path));
        }
        const Json::Value apiurl = root.get(prop_name.c_str(), "UTF-8");
        return apiurl.asString();
    }
    else
    {
        throw open_file_error("unable to open file: " + string(path));
    }
}

// APP WINDOW STATE

regular_app_window_state
from_alia(app_window_state const& state)
{
    return regular_app_window_state(
        state.position, state.size,
        state.flags & APP_WINDOW_MAXIMIZED,
        state.flags & APP_WINDOW_FULL_SCREEN);
}

app_window_state
to_alia(regular_app_window_state const& regular)
{
    return app_window_state(
        regular.position, regular.size,
        (regular.maximized ? APP_WINDOW_MAXIMIZED : NO_FLAGS) |
        (regular.full_screen ? APP_WINDOW_FULL_SCREEN : NO_FLAGS));
}

void sync_window_state(ui_context& ctx, app_window& window,
    accessor<regular_app_window_state> const& state)
{
    if (is_refresh_pass(ctx))
    {
        auto current_state = from_alia(window.state());
        if (current_state != get(state))
            set(state, current_state);
    }
}

// GENERIC CONFIG I/O

value static
read_generic_app_config(string const& app_name)
{
    alia__shared_ptr<wxConfig> wx_config(
        new wxConfig(wxString(app_name.c_str(), wxConvUTF8)));
    value result;
    wxString contents;
    if (wx_config->Read("config", &contents))
        parse_json_value(&result, std::string(contents.mb_str()));
    return result;
}

void static
write_generic_app_config(string const& app_name, value const& config)
{
    alia__shared_ptr<wxConfig> wx_config(
        new wxConfig(wxString(app_name.c_str(), wxConvUTF8)));
    string contents;
    value_to_json(&contents, config);
    wx_config->Write("config", wxString(contents.c_str(), wxConvUTF8));
}

// APP CONFIG

app_config read_app_config(string const& app_id)
{
    return from_value<app_config>(read_generic_app_config(app_id));
}

void write_app_config(string const& app_id, app_config const& config)
{
    write_generic_app_config(app_id, to_value(config));
}

struct app_config_sync_data
{
    owned_id value_id;
    // writes are delayed to avoid lagging the UI
    bool dirty;
    ui_time_type write_time;
};

void sync_app_config(ui_context& ctx, string const& app_id,
    accessor<app_config> const& config)
{
    app_config_sync_data* data;
    get_data(ctx, &data);

    alia_untracked_if (is_gettable(config))
    {
        if (is_refresh_pass(ctx) && !data->value_id.matches(config.id()))
        {
            data->value_id.store(config.id());
            data->dirty = true;
            data->write_time = get_animation_tick_count(ctx) + 500;
        }
        if (data->dirty && !get_animation_ticks_left(ctx, data->write_time))
        {
            write_app_config(app_id, get(config));
            data->dirty = false;
        }
    }
    alia_end
}

// SHARED APP CONFIG

shared_app_config read_shared_app_config()
{
    return from_value<shared_app_config>(read_generic_app_config("Astroid2"));
}

void write_shared_app_config(shared_app_config const& config)
{
    write_generic_app_config("Astroid2", to_value(config));
}

// APP INSTANCE

void initialize_app_context(app_context& app_ctx, app_instance& instance)
{
    app_ctx.instance = &instance;
}

void start_sign_in(app_instance& instance,
    string const& user, string const& password)
{
    set_authentication_info(instance.gui_system->bg, instance.api_url,
        web_authentication_credentials(user, password));
}

void sign_in_wih_token(app_instance& instance,
    string const& token)
{
    set_authentication_token(instance.gui_system->bg, token);
}

void select_realm(app_instance& instance, string const& realm_id)
{
    set_context_request_parameters(instance.gui_system->bg,
        framework_usage_info(instance.api_url, realm_id),
        context_request_parameters(
            instance.info.thinknode_app_account,
            instance.info.thinknode_app_id,
            instance.info.thinknode_version_id));
}

void set_shared_app_config(app_instance& instance,
    shared_app_config const& config)
{
    instance.shared_config = config;
    write_shared_app_config(config);
    reset(*get_disk_cache(*instance.gui_system->bg),
        config.cache_dir ?
            get(config.cache_dir) :
            get_default_cache_dir("Astroid2"),
        "",
        config.cache_size * int64_t(0x40000000));
}

string generate_unique_id(app_instance& instance)
{
    return to_string(instance.uuid_generator());
}

void resolve_gui_app_contexts(gui_context& ctx, app_context& root_context)
{
    app_context* app_ctx = &root_context;
    naming_context nc(ctx);
    for (auto const& group_ptr : get_task_groups(root_context))
    {
        auto& group = *group_ptr;
        named_block data_block(nc, make_id_by_reference(group.id));
        app_ctx = &group.controller->get_internal_app_context(ctx, *app_ctx);
        group.app_ctx = app_ctx;
        group.state = group.controller->get_state_accessor(ctx, *app_ctx);
    }
    // Also include the phantom task groups so that phantom task get their
    // contexts. (Sigh...)
    for (auto const& group_ptr : root_context.instance->phantom_task_groups)
    {
        auto& group = *group_ptr;
        named_block data_block(nc, make_id_by_reference(group.id));
        app_ctx = &group.controller->get_internal_app_context(ctx, *app_ctx);
        group.app_ctx = app_ctx;
        group.state = group.controller->get_state_accessor(ctx, *app_ctx);
    }
}

struct performance_reporting_data
{
    optional<ui_performance_metrics> reported;
    // used for tracking performance within each second
    int render_count;
    int refresh_count;
    int total_refresh_duration;
};

void static
reset_tracking(performance_reporting_data& data)
{
    data.refresh_count = 0;
    data.total_refresh_duration = 0;
    data.render_count = 0;
}

indirect_accessor<ui_performance_metrics>
compute_performance(ui_context& ctx)
{
    widget_id id = get_widget_id(ctx);
    performance_reporting_data* data;
    if (get_cached_data(ctx, &data))
    {
        start_timer(ctx, id, 1000);
        reset_tracking(*data);
    }

    if (ctx.event->type == REFRESH_EVENT)
    {
        ++data->refresh_count;
        data->total_refresh_duration += get_last_refresh_duration(*ctx.system);
        request_refresh(ctx, 0);
        record_content_change(ctx);
    }

    if (ctx.event->type == RENDER_EVENT)
        ++data->render_count;

    if (detect_timer_event(ctx, id))
    {
        // Compute new metrics to report and reset tracking.
        ui_performance_metrics reported;
        reported.fps = data->render_count;
        reported.refresh_duration =
            data->total_refresh_duration / data->refresh_count;
        data->reported = reported;
        reset_tracking(*data);
        restart_timer(ctx, id, 1000);
        end_pass(ctx);
    }

    return make_indirect(ctx, optional_in(data->reported));
}

}
