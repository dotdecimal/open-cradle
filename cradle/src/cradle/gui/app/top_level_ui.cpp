#include <cradle/gui/app/top_level_ui.hpp>

#include <alia/ui/system.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>

#include <cradle/background/system.hpp>
#include <cradle/gui/app/internals.hpp>
#include <cradle/gui/app/gui_tasks.hpp>
#include <cradle/gui/background.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/services.hpp>
#include <cradle/gui/web_requests.hpp>
#include <cradle/io/services/core_services.hpp>

#include <cradle/io/generic_io.hpp>

namespace cradle {

float get_magnification_factor(app_context& app_ctx)
{
    return app_ctx.instance->config.nonconst_get().ui_magnification_factor;
}

void set_magnification_factor(ui_system& system, app_context& app_ctx, float magnification)
{
    if (system.style.magnification != magnification)
    {
        on_ui_style_change(system);
        auto& config = app_ctx.instance->config.nonconst_get();
        config.ui_magnification_factor = magnification;
        system.style.magnification = config.ui_magnification_factor;
    }
}

void static
reset_ui(gui_context& ctx, app_context& app_ctx)
{
    reset_task_groups(app_ctx);

    // Clear any cached data.
    clear_memory_cache(get_background_system(ctx));
    clear_mutable_data_cache(get_background_system(ctx));

    // Why is this here?
    set_magnification_factor(*ctx.system, app_ctx,
        get_magnification_factor(app_ctx));
}

void static
leave_realm(gui_context& ctx, app_context& app_ctx)
{
    clear_framework_context(*ctx.gui_system->bg);
    reset_ui(ctx, app_ctx);
}

void static
log_out(gui_context& ctx, app_context& app_ctx)
{
    app_ctx.instance->username.set("");
    clear_authentication_info(*ctx.gui_system->bg);
    // Clear realm selection
    clear_framework_context(*ctx.gui_system->bg);
    reset_ui(ctx, app_ctx);
}

void static
toggle_full_screen(gui_context& ctx, app_window& window)
{
    bool is_full_screen = window.is_full_screen();
    window.set_full_screen(!is_full_screen);
}

void static
do_right_side_header_controls(
    gui_context& ctx, app_context& app_ctx, app_window& window)
{
    row_layout right_side(ctx, CENTER_Y);

    //do_state_session_ui(ctx, app_ctx);

    auto show_performance = get_state(ctx, false);
    alia_if (get(show_performance))
    {
        auto performance = compute_performance(ctx);
        alia_if (is_gettable(performance))
        {
            do_text(ctx,
                printf(ctx, "%i \xC2\xB5s refresh; %i FPS",
                    _field(performance, refresh_duration),
                    _field(performance, fps)));
        }
        alia_else
        {
            do_text(ctx, text("measuring"));
        }
        alia_end
        if (do_link(ctx, text("hide")))
            set(show_performance, false);

        do_separator(ctx);
    }
    alia_end
    if (detect_key_press(ctx, KEY_F9))
    {
        set(show_performance, !get(show_performance));
    }

    auto& notifications = ctx.gui_system->notifications;

    auto unseen_notifications =
        count_if(
            ctx.gui_system->notifications.history.begin(),
            ctx.gui_system->notifications.history.end(),
            [](notification_content const& notification)
            {
                return !notification.seen;
            });

    alia_if(unseen_notifications > 0)
    {
        scoped_substyle style(ctx, text("notification-flag"));
        alia_if(do_button(ctx, in(to_string(unseen_notifications)), layout(width(2.5, EM), RIGHT)))
        {
            app_ctx.instance->selected_page =
                app_level_page::NOTIFICATIONS;
            end_pass(ctx);
        }
        alia_end
    }
    alia_end

    auto user =
        get_user_info(ctx, app_ctx,
            _field(get_session_info(ctx, app_ctx), username));
    alia_if (is_gettable(user))
    {
        do_styled_text(ctx, text("user"), _field(user, name));
    }
    alia_end

    do_styled_text(ctx, text("realm"),
        printf(ctx, "(%s)", make_accessor(app_ctx.instance->realm_id)));

    // Only add the menu and close icons if we're in full screen mode.
    // (They're available through the normal OS title/menu bar otherwise.)
    alia_if (window.is_full_screen())
    {
        {
            state_proxy<int> selection;
            drop_down_list<int>
                ddl(ctx, make_accessor(selection), layout(width(1, EM)),
                    DDL_COMMAND_LIST);
            alia_if (ddl.do_list())
            {
                int n = 0;
                {
                    column_layout column(ctx, layout(width(9, EM)));
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Switch Realms"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        leave_realm(ctx, app_ctx);
                        end_pass(ctx);
                    }
                }
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Sign Out"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        log_out(ctx, app_ctx);
                        end_pass(ctx);
                    }
                }
                do_separator(ctx);
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Exit Full Screen"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        toggle_full_screen(ctx, window);
                        end_pass(ctx);
                    }
                }
                do_separator(ctx);
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("App Info"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        app_ctx.instance->selected_page =
                            app_level_page::APP_INFO;
                        end_pass(ctx);
                    }
                }
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Notifications"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        app_ctx.instance->selected_page =
                            app_level_page::NOTIFICATIONS;
                        end_pass(ctx);
                    }
                }
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Settings"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        app_ctx.instance->selected_page =
                            app_level_page::SETTINGS;
                        end_pass(ctx);
                    }
                }
                {
                    ddl_item<int> item(ddl, ++n);
                    do_text(ctx, text("Developer Console"));
                    if (selection.was_set() && selection.get() == n)
                    {
                        app_ctx.instance->selected_page =
                            app_level_page::DEV_CONSOLE;
                        end_pass(ctx);
                    }
                }
            }
            alia_end
        }

        if (do_icon_button(ctx, REMOVE_ICON))
        {
            window.close();
            end_pass(ctx);
        }
    }
    alia_end
}

void static
do_header_row(
    gui_context& ctx, app_context& app_ctx, app_window& window)
{
    panel header(ctx, text("header"), UNPADDED, PANEL_HORIZONTAL |
        PANEL_NO_INTERNAL_PADDING);

    {
        row_layout task_row(ctx, CENTER_Y);

        naming_context nc(ctx);
        auto const& task_groups = get_task_groups(app_ctx);
        size_t n_groups = task_groups.size();
        alia_if(n_groups == 1)
        {
            {
                panel logo_panel(ctx, text("task-group-logo"), UNPADDED | BASELINE_Y);
                do_app_logo(ctx, _field(get_app_info(ctx, app_ctx), logo), CENTER_Y);
            }
        }
        alia_else
        {
            {
                clickable_panel logo_panel(ctx, text("task-group-logo"), UNPADDED | BASELINE_Y);
                if (logo_panel.clicked())
                {
                    uncover_task_group(app_ctx, 0);
                    end_pass(ctx);
                }
                do_app_logo(ctx, _field(get_app_info(ctx, app_ctx), logo), CENTER_Y);
            }
        }
        alia_end

        for (size_t i = 0; i != n_groups; ++i)
        {
            auto const& group = *task_groups[i];
            named_block data_block(nc, make_id_by_reference(group.id));
            alia_if (i + 1 != n_groups)
            {
                row_layout row(ctx, BASELINE_Y);
                {
                    clickable_panel p(ctx, text("task-group"), UNPADDED | BASELINE_Y);
                    if (p.clicked())
                    {
                        uncover_task_group(app_ctx, i);
                        end_pass(ctx);
                    }
                    group.controller->do_header_label(ctx, *group.app_ctx);
                }
                do_styled_text(ctx, text("bold"), text("\302\273"), CENTER_Y);
            }
            alia_else
            {
                panel p(ctx, text("active-task-group"), UNPADDED | BASELINE_Y);
                group.controller->do_header_label(ctx, *group.app_ctx);
            }
            alia_end
        }
    }

    do_spacer(ctx, GROW);

    do_right_side_header_controls(ctx, app_ctx, window);
}

void static
do_control_panel(
    gui_context& ctx, app_context& app_ctx,
    generic_gui_task_stack<gui_task_with_context>& task_stack)
{
    // resizable_content works in screen pixels, but we want the UI magnification factor
    // to apply to the pixel count that we're storing in the app config.
    auto width_in_pixels =
        accessor_cast<int>(
            scale(
                _field(get_app_config(ctx, app_ctx), control_panel_width),
                ctx.system->style.magnification));

    resizable_content resizable(ctx, width_in_pixels);

    scrollable_panel
        control_panel(ctx,
            text("control-panel"),
            GROW,
            PANEL_NO_INTERNAL_PADDING | PANEL_NO_VERTICAL_SCROLLING,
            storage(_field(get_app_config(ctx, app_ctx), scroll_position)));

    alia_if (!is_empty(task_stack))
    {
        naming_context nc(ctx);
        auto const& task_groups = get_task_groups(app_ctx);
        for (size_t i = 0; i != task_groups.size(); ++i)
        {
            auto const& group = *task_groups[i];
            named_block data_block(nc, make_id_by_reference(group.id));
            group.controller->do_task_header_content(ctx, *group.app_ctx);
        }
    }
    alia_end

    auto result = do_task_stack_controls(ctx, task_stack);
    switch (result.event)
    {
     case gui_task_stack_event::UNCOVER_TASK:
        uncover_task(app_ctx, result.id);
        end_pass(ctx);
        break;
    }
}

void static
do_app_content_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    row_layout main_row(ctx, GROW);

    auto controls_expanded = get_state(ctx, true);

    if (detect_key_press(ctx, key_code('[')))
    {
        set(controls_expanded, !get(controls_expanded));
        end_pass(ctx);
    }

    do_left_panel_expander(ctx, controls_expanded,
        FILL_Y | UNPADDED);

    auto& task_stack = get_gui_task_stack(ctx, app_ctx);

    {
        horizontal_collapsible_content collapsible(ctx,
            get(controls_expanded), default_transition, 0.9);
        alia_if (collapsible.do_content())
        {
            do_control_panel(ctx, app_ctx, task_stack);
        }
        alia_end
    }

    {
        layered_layout layering(ctx, GROW);

        do_task_stack_display(ctx, task_stack);

        do_background_status_report(ctx);
    }
}

shared_app_config static
sanitize_shared_app_config(shared_app_config const& config)
{
    return
        shared_app_config(
            (config.cache_dir && get(config.cache_dir).empty()) ?
            none : config.cache_dir,
            config.cache_size,
            (config.crash_dir && get(config.crash_dir).empty()) ?
            none : config.crash_dir);
}

void static
do_directory_controls(
    gui_context& ctx, accessor<optional<file_path> > const& dir)
{
    alia_if (has_value(dir))
    {
        do_text_control(ctx, unwrap_optional(ref(&dir)), width(20, EM));
        if (do_link(ctx, text("use default")))
        {
            set(dir, optional<file_path>());
            end_pass(ctx);
        }
    }
    alia_else
    {
        do_text(ctx, text("using default"));
        if (do_link(ctx, text("use custom")))
        {
            set(dir, some(file_path("")));
            end_pass(ctx);
        }
    }
    alia_end
}

void static
do_app_info_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    column_layout top(ctx, GROW);
    scoped_substyle style(ctx, text("top-level-ui"));
    auto clamped_width = width(60, EM);
    {
        clamped_header header(ctx, text("background"), text("header"),
            clamped_width);
        do_heading(ctx, text("heading"), text("App Info"));
    }
    {
        clamped_content background(ctx, text("background"), text("content"),
            clamped_width, GROW);

        form form(ctx);

        do_form_section_heading(form, text("support"));
        {
            form_field field(form, text("User Guide"));
            do_url_link(ctx,
                text("apps.dotdecimal.com"),
                text("http://apps.dotdecimal.com"));
        }
        {
            form_field field(form, text("Report an Issue"));
            do_url_link(ctx,
                text("dotdecimal.freshdesk.com"),
                text("http://dotdecimal.freshdesk.com"));
            do_spacer(ctx, height(8, PIXELS));
        }

        do_form_section_heading(form, text("thinknode"));

        auto app_info = get_app_info(ctx, app_ctx);
        {
            form_field field(form, text("Realm"));
            do_styled_text(ctx, text("realm"),
                printf(ctx, "(%s)", make_accessor(app_ctx.instance->realm_id)));
        }
        {
            form_field field(form, text("App Account"));
            do_styled_text(ctx, text("value"),
                _field(app_info, thinknode_app_account));
        }
        {
            form_field field(form, text("App ID"));
            do_styled_text(ctx, text("value"),
                _field(app_info, thinknode_app_id));
        }
        {
            form_field field(form, text("App Version"));
            do_styled_text(ctx, text("value"),
                _field(app_info, thinknode_version_id));
        }
        {
            form_field field(form, text("Context ID"));
            {
                row_layout row(ctx);
                auto context_id =
                    _field(get_framework_context(ctx, app_ctx), context_id);
                do_styled_text(ctx, text("value"), context_id);
                alia_if (is_gettable(context_id))
                {
                    alia_untracked_if (do_link(ctx, text("copy")))
                    {
                        ctx.system->os->set_clipboard_text(get(context_id));
                        end_pass(ctx);
                    }
                    alia_end
                }
                alia_end
            }
            do_spacer(ctx, height(8, PIXELS));
        }

        do_form_section_heading(form, text("client"));

        {
            form_field field(form, text("Built"));
            do_styled_text(ctx, text("value"), text(__TIMESTAMP__));
        }
        {
            form_field field(form, text("Version"));
            do_styled_text(ctx, text("value"),
                in_ptr(&app_ctx.instance->info.local_version_id));
        }
        {
            // FDA GUDID Bar code and symbols
            do_spacer(ctx, height(50, PIXELS));
            panel p(ctx, text("gudid-panel"));

            {
                row_layout row(ctx);
                do_spacer(ctx, GROW_X);
                {
                    panel p(ctx, text(""), width(400, PIXELS));

                    row_layout row(ctx);
                    // Read instructions for use
                    do_svg_graphic(ctx, text("gudid-barcode"),
                        text("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><!-- Created with Inkscape (http://www.inkscape.org/) --><svg xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns=\"http://www.w3.org/2000/svg\" height=\"224\" width=\"244\" version=\"1.1\" xmlns:cc=\"http://creativecommons.org/ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\"><metadata><rdf:RDF><cc:Work rdf:about=\"\"><dc:format>image/svg+xml</dc:format><dc:type rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\"/><dc:title/></cc:Work></rdf:RDF></metadata><g transform=\"translate(24.832631,-0.67115309)\"><path fill=\"#d8d8d8\" d=\"M26.5,186c-6.7,0-15.1-2-18.7-4l-3.79-1v-71-71.2l2.25,0.7c14.9,4.8,33,6.34,42.8,3.65,2.48-0.684,11.9-4.8,21-9.15,19.5-9.35,25.9-10.9,35.5-8.4,3.3,0.842,6.98,1.78,8.18,2.08s4.57-0.257,7.5-1.24c11.3-3.8,21.6-1.8,39.3,7.65,4.95,2.64,12.6,6.03,17,7.53,6.94,2.37,9.45,2.72,19,2.62,10.1-0.105,14.9-0.914,28.2-4.8l2.25-0.655v71,71l-6.21,2.1c-8.12,2.74-22.6,4.47-31,3.7-8.27-0.759-18.5-4.36-30.3-10.7-4.95-2.64-11.8-5.74-15.2-6.88-8.06-2.69-19.2-2.77-24.9-0.18-3.8,1.72-4.27,1.74-7,0.228-3-1-6-1-14-1h-10.5l-17.2,8.28c-21.3,10-29.9,12-44.8,10zm18.7-9.81c4.15-0.879,12.6-4.31,22-8.92,17.4-8.53,22.8-9.99,35-9.48h9v-60.2-60.7l-2-1.3c-2-0.8-6-1.3-11-1.3l-8.5-0.0012-15.5,7.73c-8.52,4.25-18.4,8.51-22,9.46-7.2,1.92-25.9,2.2-34.2,0.525l-4.75-0.956v61.7,61.7l5.75,1.13c12.1,2.38,17.8,2.52,26.5,0.678zm168,0,5-2,0-61c0-56.8,0-62.2-2-61.6-3,1.3-21,2.7-26,2.2-10-1.1-17-3.3-33-11.3-18-8.4-24-9.8-33-7.3l-4,1.3v61.1,60.6l9.51-0.319c11.9-0.399,16.8,0.944,33.4,9.25,11.8,5.89,19.2,8.81,25.1,9.93,3.76,0.711,19.3-0.351,24.7-1.69zm-66-34v-5.5h6,6v-19.5-19.5h-6-6v-3.5-3.5h18.5,18.5v23,23h5,5v5.5,5.5h-23.5-23.5v-5.5zm18-57.9c-7-4.6-8-14-2-19.8,3-2.6,5-3.3,8-3.3,8,0,13,4.9,13,13,0,9.02-11,15-18.7,10.1z\"/><path fill=\"#bcbcbc\" d=\"m23.2,186c-3.49-0.489-9.23-1.85-12.8-3.02l-6.4-2v-71c0-67.4,0.09-71.2,1.75-70.5,12.2,4.81,34,6.35,44.8,3.15,3.3-0.981,11-4.28,17-7.34,21.7-11,30.6-12.7,42.6-8.5,3.57,1.27,6.14,1.66,7.13,1.1,0.856-0.479,4.35-1.52,7.76-2.32,9.72-2.27,15.7-0.837,35,8.39,9.08,4.33,18.5,8.44,21,9.14,2.64,0.746,9.26,1.21,16,1.13,10.6-0.133,16.5-1.14,27.8-4.76l2.25-0.723v70.5c0,58.2-0.238,70.7-1.36,71.6-2.29,1.9-15.8,4.9-25.2,5.62-13.4,1.01-20.6-0.87-40.9-10.7-16.6-8.03-17.1-8.22-25.7-8.62-6.66-0.318-9.9,0.003-13.7,1.36-4.63,1.67-5.19,1.67-9.07,0.12-4.77-1.91-16.5-2.2-22.6-0.567-2.2,0.592-9.4,3.8-16,7.13s-15.2,7.14-19,8.47c-7.98,2.76-20.4,3.69-30.3,2.3zm27.8-10.8c3.03-1.05,10.5-4.46,16.5-7.58,15.6-8.03,22.2-9.88,34-9.45h9v-60.6-61.1l-4-1.3c-3-0.7-7.8-1.1-11.3-0.8-5.4,0.3-8.4,1.4-20.7,7.5-22.6,11.1-32.1,13.3-49.2,11.4-5.3-0.6-10.3-1.4-11.2-1.7-1.5-0.6-1.6,5-1.6,61.6v62l5.25,1.04c12.5,2.48,24.3,2.22,32.8-0.716zm162,0.656,5.25-1.15,0-61.9c0-53.9-0.189-61.9-1.46-61.4-4.42,1.7-22.9,2.76-29.7,1.71-10.2-1.56-12.3-2.3-30.2-10.9-17.3-8.27-23.4-9.64-32.3-7.25l-5,0.9v61.2,61.2l8.12-0.369c11.7-0.531,18.2,1.23,35.2,9.61,8.33,4.09,17.9,8.06,21.2,8.83,6.59,1.53,19.5,1.19,28.2-0.747zm-66-33,1-5l5.75-0.3,5.75-0.3v-19.5-19.5h-6-6v-3.5-3.5h18.5,18.5v23,23h5,5v5.5,5.5h-23.6-23.6l0.302-5.25zm19-58.8c-7.29-3.64-8.82-13.1-3.14-19.4,2.23-2.49,3.75-3.18,7.94-3.58,4.98-0.479,5.34-0.343,9.19,3.51,3.49,3.49,4.01,4.59,4.01,8.49,0,5.05-2.39,8.87-7.04,11.3-3.75,1.94-6.68,1.87-11-0.272z\"/><path fill=\"#999\" d=\"M26,186c-4.7-1-11.5-2-15.2-3l-6.8-2v-71c0-67.5,0.09-71.2,1.75-70.5,4.84,2.11,16.5,4.5,24.6,5.04,13.4,0.897,21-1.03,39.1-9.99,8.25-4.07,16.8-7.9,19-8.5,5.98-1.63,15.3-1.27,21.4,0.827,5.28,1.82,5.52,1.82,10.9-0.0326,6.74-2.32,15-2.43,22.2-0.29,2.97,0.883,11.2,4.52,18.3,8.08,14,7,25,10.4,34,10.4,7.73,0,19.6-1.84,25.9-4l5-1.9v70.8,70.8l-3.84,1.53c-9.18,3.67-29.5,5.09-40.3,2.8-3.09-0.659-12.5-4.6-21-8.75-21.3-10.5-30.7-12.2-42.1-7.64-3.36,1.32-4.17,1.29-8.48-0.356-10.6-4.06-23-1.61-40.8,8.1-16.2,8-27.7,10-44.2,9zm19.2-9c2.9-1,12.7-5,21.8-9,18.7-8.94,23.9-10.4,35.7-9.73l8.34,0.438v-61.2-61.2l-6.06-1.7c-8.4-1.7-13-0.6-32,8.6-20.4,9.8-24.8,10.9-42,10.2-7.4-0.3-14.5-0.8-15.8-1.2l-2.2-0.7v62,62l5.25,1.06c2.89,0.584,6.15,1.24,7.25,1.46,4.19,0.825,14.6,0.582,19.7-0.458zm168-1l5.25-1.16v-62-62l-2.25,0.705c-1.24,0.388-8.55,0.938-16.2,1.22-17.8,0.659-22.1-0.391-42.2-10.3-19-9.2-24-10.5-32-8.2l-6,1.7-0.0005,61.2-0.00051,61.2,8.91-0.346c11.5-0.447,16.7,1.05,34.6,9.86,7.7,3.8,16.5,7.6,19.5,8.45,6.18,1.73,20.8,1.5,29.8-0.472zm-66-33v-5h6,6v-20-20h-6-6v-3.5-3.5h18.5,18.5v23.5,23.5h5,5v5,5h-23.5-23.5v-5zm19.3-58.6c-12.3-6.06-6.97-24.4,6.67-23,6.01,0.614,11,5.81,11,11.4-0.005,4.97-3.21,10-7.49,11.8-4.35,1.82-6.22,1.79-10.2-0.184z\"/><path fill=\"#7d7d7d\" d=\"M27,186c-9.2-1-15.9-2-20.2-5l-2.75-1v-71c0-38.3,0.11-70,0.25-70,0.138,0.000278,2.52,0.864,5.3,1.92,5.75,2.2,18,4.1,26.4,4.1,9,0,19.7-3.3,33.7-10.3,20.1-10.1,28.3-11.7,39.4-7.8,5.5,1.89,5.67,1.89,11.7,0,12.5-3.9,20.4-2.3,41.8,8.62,13.3,6.77,22.2,9.42,31.8,9.45,7.42,0.0249,20-1.85,26.1-3.91l5.25-1.76v70.8,70.8l-6.75,2.1c-8.33,2.59-22.8,3.92-31.4,2.88-7.56-0.91-17.5-4.46-27.4-9.8-13-7.01-20.2-9.4-28.4-9.49-5.06-0.0538-9.07,0.523-12.3,1.77-4.47,1.71-5.11,1.73-8.5,0.289-12-5-20.2-3-42,7-20.5,9.79-27.1,11.3-43,9.92zm20.3-9.43c3.13-0.815,11.5-4.38,18.5-7.92,17.7-8.89,24.9-11,36.3-10.4l8.93,0.427v-61.3-61.3l-5.6-1.63c-7.9-2.8-12.4-1.7-31,7.2-21.6,10.3-25.8,11.4-43,10.7-7.4-0.3-14.5-0.8-15.8-1.2l-2.2-0.7v62.1,62.1l2.26,0.568c10.5,2.64,24.1,3.06,32,0.989zm166-1l4.75-1.22v-61.9-61.9l-4.75,0.811c-2,0.2-11,0.6-18,0.6-16,0-18.1-0.587-38.5-10.6-17-8.36-22.9-9.79-30.9-7.46l-5,1.7v61.3,61.3l8.28-0.435c11.3-0.591,18,1.31,35.3,9.94,7.81,3.9,16.6,7.7,19.5,8.45,6.24,1.59,22.8,1.24,30.1-0.621zm-66-33v-5h6,6v-20-20l-5.75-0.3c-5-0.2-5-0.2-5-3.2v-3l18.2-0.273,18.2-0.273v23.5,23.5h5,5v5,5h-23.5-23.5v-5zm21-58.1c-10-3.9-11-16-3-21.7,2.78-1.88,4.4-2.28,7.87-1.9,12.4,1.36,15.1,18.5,3.64,23.3-3.96,1.65-5.06,1.69-8.51,0.316z\"/><path fill=\"#595959\" d=\"m17.5,185c-3.31-0.721-7.7-2.01-9.75-2.87l-3.75-2v-70-70.8l7.75,2.48c10.5,3.36,27.1,4.24,36.2,1.93,3.58-0.904,13.5-4.99,22-9.08,8.52-4.09,17.3-7.9,19.5-8.48,6.03-1.58,12.6-1.28,19.6,0.899,6.25,1.94,6.33,1.94,12.3,0,11.7-3.85,18.9-2.43,39.8,7.88,7.89,3.89,17.1,7.77,20.5,8.61,10.7,2.7,30.8,1.32,40.6-2.78l3.75-1.57v70.9,70.9l-4.75,1.53c-12,3.88-30.3,4.82-40.7,2.1-3.23-0.841-12.7-4.85-21-8.92-8.3-4.06-17.5-7.93-20.5-8.6-6.26-1.41-15.8-0.678-20.1,1.55-2.71,1.4-3.45,1.36-8.94-0.487-11.4-3.84-19.5-2.3-38.8,7.35-17.8,8.91-23.9,10.7-37.1,10.6-5.85-0.0437-13.4-0.669-16.7-1.39zm30.6-8.1c3-1,11.7-5,19.4-9,17.2-8.49,24.1-10.4,35.2-9.68l8.25,0.513v-61.4-61.4l-5.18-1.54c-3-1-6.6-1.7-8.2-1.7-4.4,0-14.3,3.7-24.3,9-4.9,2.6-12.4,6.1-16.5,7.6-6.7,2.5-8.9,2.8-20.5,2.8-7.1,0.1-15.6-0.3-18.7-0.7l-5.8-0.8v61.1c0,48.1,0.272,61.7,1.25,62.3,5.14,3.22,26.1,4.28,34.8,1.76zm166-0.4,3.75-1.12,0-62.1,0-62.1-2.75,0.636c-1.51,0.35-8.38,0.901-15.2,1.22-16.7,0.787-22.1-0.562-43.5-10.9-16.2-7.84-17.3-8.19-24-8.22-4.29-0.0155-8.18,0.558-10,1.47l-3,1.5-0.258,61.2-0.258,61.2,7.39-0.624c11.5-0.976,18.6,0.85,35.9,9.35,8.38,4.11,17.5,8.1,20.2,8.88,5.76,1.63,25.6,1.29,31.8-0.554zm-66-34v-4.95l5.75-0.3,5.75-0.3,0.271-19.8,0.271-19.8h-6.02-6.02v-3.5-3.5h18,18v23.5,23.5h5,5v5,5h-23-23v-4.95zm15-61.4c-2.77-2.77-3.4-4.16-3.4-7.55,0-5.53,2.17-9.43,6.36-11.4,4.53-2.15,7.44-2.06,11.4,0.362,7.02,4.28,8.02,13.6,2.05,19.2-2,2.3-3,2.8-7,2.8-5,0-6-0.4-9-3.4z\"/><path fill=\"#3d3d3d\" d=\"m18,185c-3.6-0.717-8.21-2-10.2-2.86l-3.8-2v-70-70.2l2.25,0.6c1.24,0.353,4.72,1.33,7.75,2.18,3.02,0.844,10.7,1.78,17,2.09,14.8,0.715,19.9-0.589,39.5-10.2,20.8-10.1,26.7-11.3,38.4-7.48l6.19,2.03,6.58-2.03c12.3-3.79,16.6-2.95,38.4,7.51,8.52,4.09,18.4,8.17,22,9.08,9.31,2.35,26.9,1.48,36.8-1.83l7-2.4v70.7,70.7l-7.25,2.25c-5.1,1.58-10.9,2.41-19.4,2.79-14.9,0.657-20.6-0.811-39.6-10.3-7.08-3.55-15.7-7.16-19.1-8.03-7.36-1.89-16.8-1.39-21.7,1.13-3.17,1.64-3.57,1.64-6.89,0.0575-1.94-0.925-6.5-1.93-10.1-2.24-8.79-0.737-15,1.12-32.5,9.76-7.7,3.79-16.7,7.58-20,8.42-7.19,1.84-22.4,1.85-31.6,0.0307zm32.2-8.56c3.36-1.13,11.3-4.7,17.7-7.94,16.9-8.54,19.8-9.39,32.4-9.45l10.8-0.049v-61.1-61.1l-3.25-1.37c-1.79-0.756-6.45-1.34-10.4-1.31-6.83,0.0563-7.78,0.385-22.7,7.8-21,9.7-28.3,11.6-44.4,10.7-6.6-0.3-13.5-0.9-15.3-1.2l-3.2-0.6v62.1,62.1l3.75,0.972c6.17,1.6,8.31,1.83,18.5,1.97,7.5,0.104,11.2-0.344,15.9-1.92zm164,0.0882,3.75-1.13,0-62,0-62-5.75,0.822c-3,0.3-11,0.7-18,0.7-14,0-16-0.7-38-11.1-18-8.5-22-9.6-31-7l-6,1.7v61.4,61.4l8.25-0.578c11.2-0.785,17.7,1.02,36.8,10.1,8.52,4.09,17.1,7.9,19,8.46,4.33,1.26,26.2,0.627,31.2-0.902zm-66-33v-5h6,6v-20-20h-6-6v-3.5-3.5h18,18v23.5,23.5h5,5v5,5h-23-23v-5zm15.4-61.4c-2.77-2.77-3.4-4.16-3.4-7.55,0-5.61,2.18-9.43,6.55-11.5,4.7-2.23,9.81-1.33,13.5,2.38,3.71,3.71,4.61,8.82,2.38,13.5-2,4.4-5,6.6-11,6.6-3,0-5-0.6-8-3.4z\"/><path fill=\"#000\" d=\"m21.5,185c-2.75-0.45-7.59-1.59-10.8-2.54l-5.7-1v-71c0-38.3,0.11-70,0.25-70,0.138,0.0071,3.18,0.958,6.75,2.11,4.59,1.49,10.2,2.25,19.1,2.61,15.6,0.7,19-0.2,39.9-10.3,19.2-9.26,24.9-10.6,34.7-7.94,8.42,2.25,11.2,2.26,18.8,0.0087,9.46-2.79,16.5-1.18,35.1,8,8.32,4.11,18,8.21,21.5,9.1,8.81,2.26,28.7,1.48,37.9-1.48,3.58-1.16,6.61-2.11,6.75-2.11,0.138-0.0071,0.25,31.5,0.25,70,0,65.7-0.106,70-1.76,70.9-3.96,2.12-14.3,4.21-23.8,4.79-14.1,0.866-21.7-1-39.1-9.61-7.62-3.77-16.5-7.63-19.8-8.58-7.13-2.06-18.5-1.63-23.3,0.893-3.04,1.58-3.46,1.58-6.49,0.0104-4.22-2.18-15.4-2.95-21.5-1.47-3.8,1-13,4-21.5,9-8.52,4.1-18.2,8.13-21.5,8.97-6.32,1.61-18.7,1.82-27,0.466zm26.6-8.03c1.9-1,10.4-5,18.9-9,18.4-8.83,27.7-11.2,37.7-9.73l6.3,0.941v-61.8-61.8l-2.75-1.2c-1.51-0.66-6.14-1.21-10.3-1.23-7.37-0.0252-7.86,0.138-23.5,7.72-23.4,12-34.7,14.2-54.4,10.9l-7.8-1.2v62.2,62.2l3.25,1.02c5.07,1.59,7.5,1.85,18.9,2,5.83,0.0749,12.1-0.353,14-0.951zm164,0l5-1.16,0.257-62.6,0.257-62.6-4.26,1.05c-2,0.4-9,1-17,1-16,0.1-19-0.8-40-11.2-15-7.5-16-7.7-23-7.7-4.13,0-8.97,0.617-10.8,1.37l-3.25,1.37v61.7,61.7l4.75-0.936c5.68-1.12,15.6-0.314,22.1,1.79,2.58,0.84,9.63,4.05,15.7,7.12,17.5,8.92,22.1,10.2,35,10.2,6.05-0.0412,13.2-0.597,16-1.24zm-64-34v-5h6,6v-20.5-20.5h-6c-5.73,0-6-0.111-6-2.5v-2.5h18,18v23,23h5,5v5,5h-23-23v-5zm17.8-59.6c-5.97-3.24-7.6-12.1-3.18-17.4,4.59-5.45,12.4-5.92,17.3-1.02,3.52,3.52,4.5,8.02,2.7,12.3-2.86,6.85-10.4,9.54-16.8,6.04z\"/></g></svg>"),
                        layout(height(80, PIXELS), CENTER_Y, 1));

                    // Device manufacturer svg
                    do_svg_graphic(ctx, text("gudid-barcode"),
                        text("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?><!-- Created with Inkscape (http://www.inkscape.org/) --><svg xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns=\"http://www.w3.org/2000/svg\" height=\"93\" width=\"113\" version=\"1.1\" xmlns:cc=\"http://creativecommons.org/ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\"><metadata><rdf:RDF><cc:Work rdf:about=\"\"><dc:format>image/svg+xml</dc:format><dc:type rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\"/><dc:title/></cc:Work></rdf:RDF></metadata><g transform=\"translate(1.1822037,0.39406769)\"><path fill=\"#c1c1c1\" d=\"M12,74.9,12,58.8,18.7,46.9c3.7-6.5,7.1-11.9,7.5-11.9,0.425,0,1.88,2.14,3.24,4.75s3.54,6.49,4.86,8.61l2.4,3.86,4.83-8.61c2.7-4.8,5.1-8.7,5.5-8.7,0.353,0,2.81,3.86,5.45,8.57l4.81,8.57,2.2-3.82c1.3-2.2,3.5-6,5-8.6,1.49-2.61,3.03-4.74,3.42-4.74,0.389,0.0077,2.73,3.72,5.21,8.25l4.5,8.1,0.3-24.7,0.2-24.8h12,12v44.5,44.5h-45-45v-16.1z\"/><path fill=\"#BBB\" d=\"M12,74.9,12,58.7,18.8,46.9c3.71-6.51,7.05-11.8,7.42-11.9,0.368-0.0152,2.75,3.8,5.3,8.47,2.55,4.68,4.79,8.5,4.99,8.5,0.195,0,2.5-3.82,5.13-8.5,2.62-4.68,5.03-8.5,5.34-8.49,0.314,0.0036,2.77,3.88,5.45,8.62l4.88,8.61,4.95-8.62c2.72-4.74,5.25-8.61,5.62-8.6,0.368,0.0077,2.69,3.72,5.17,8.25l4.5,8.3,0.3-24.7,0.2-24.8h12,12v44.5,44.5h-45-45v-16.1z\"/><path fill=\"#a6a6a6\" d=\"m12,75.1,0-15.9,5.61-9.83c9.6-16.8,7.9-16.1,13.7-5.9,5.7,10,5,10.1,11.5-1.7,2.01-3.71,3.9-6.75,4.2-6.74,0.297,0.0046,2.04,2.83,3.87,6.28s4.12,7.32,5.08,8.59l1.74,2.31,4.84-8.63c2.66-4.74,5.19-8.28,5.61-7.84,0.4,0.2,2.8,4.2,5.3,8.6l4.56,8,0.0074-24.8v-24.7h12,12v44,44h-45-45v-15.9z\"/><path fill=\"#9e9e9e\" d=\"m12,75.3,0-15.7,6.14-11.1c3.38-6.1,6.6-11.7,7.17-12.4,0.777-0.986,2.25,0.824,6.02,7.39,5.7,9.93,4.99,10,11.5-1.83,2-3.7,3.9-6.7,4.2-6.7,0.275,0.000981,2.3,3.4,4.5,7.56s4.51,7.99,5.14,8.52c0.805,0.682,2.56-1.56,5.97-7.63,5.5-9.81,4.82-9.92,11.2,1.86l3.7,6.8,0.3-24.6,0.2-24.6h12,12v44,44h-45-45v-15.7z\"/><path fill=\"#959595\" d=\"m12,75.3,0-15.7,3.13-5.58c1.72-3.07,4.92-8.66,7.1-12.4l3.98-6.85,4.48,7.85c2.46,4.32,4.79,8.4,5.17,9.07,0.416,0.737,2.66-2.29,5.66-7.66,2.74-4.88,5.29-8.68,5.67-8.45,0.382,0.236,2.55,3.78,4.81,7.88s4.5,7.69,4.97,7.98c0.47,0.291,2.54-2.51,4.6-6.23,6.3-11.7,5.4-11.6,11.2-1.6l5.1,8.8v-24.7-24.8h12,12v44,44h-45-45v-15.7z\"/><path fill=\"#898989\" d=\"m12,75.2,0-15.8,4.61-7.98c2.54-4.39,5.73-9.94,7.1-12.3l2.49-4.35,4.48,7.85c2.46,4.32,4.81,8.43,5.2,9.13,0.462,0.818,2.44-1.79,5.51-7.25,2.6-4.6,5.1-8.4,5.5-8.4,0.401,0,2.89,3.85,5.53,8.56l4.8,8.56,4.46-7.81c2.45-4.3,4.8-8.42,5.23-9.17,0.529-0.937,2.37,1.41,5.89,7.5l5.2,8.8v-24.7-24.8h12,12v44,44h-45-45v-15.8z\"/><path fill=\"#000\" d=\"M12,75.4,12,59.8,18.8,47.9c3.71-6.53,7.05-11.9,7.41-11.9,0.365-0.0188,2.35,3,4.41,6.72,6.49,11.7,5.61,11.6,10.9,1.83,2.57-4.72,4.98-8.58,5.35-8.58,0.368,0.0018,2.87,3.84,5.55,8.53,2.69,4.69,5,8.4,5.13,8.25,0.138-0.156,2.49-4.21,5.23-9.01l4.98-8.72,4.88,8.71,4.9,8.9,0.3-24.8,0.2-24.7h12,12v44,44h-45-45v-15.6z\"/></g></svg>"),
                        layout(height(80, PIXELS), CENTER_Y, 1));

                    {
                        column_layout col(ctx, CENTER_Y);
                        // Device manufacturer info
                        do_styled_text(ctx, text("gudid-barcode"), text(".decimal, LLC"), UNPADDED);
                        do_styled_text(ctx, text("gudid-barcode"), text("121 Central Park Place"), UNPADDED);
                        do_styled_text(ctx, text("gudid-barcode"), text("Sanford, FL. 32771"), UNPADDED);
                    }
                }
                do_spacer(ctx, GROW_X);
            }
            {
                // App bar code svg
                do_svg_graphic(ctx, text("gudid-barcode"),
                    _field(app_info, app_barcode),
                    layout(height(80, PIXELS), CENTER_Y, 1));
                // App bar code id
                do_styled_text(ctx, text("gudid-barcode"), _field(app_info, app_barcode_id), CENTER_X | UNPADDED);
            }
        }
    }
    {
        clamped_header footer(ctx, text("background"), text("footer"),
            clamped_width);
        {
            row_layout row(ctx);
            do_spacer(ctx, GROW);
            if (do_button(ctx, text("Done")))
            {
                app_ctx.instance->selected_page = app_level_page::APP_CONTENTS;
                end_pass(ctx);
            }
        }
    }
}

void static
do_settings_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    auto& disk_cache = *get_disk_cache(*ctx.gui_system->bg);

    auto state = get_state(ctx, shared_app_config());
    auto cache_info = get_state(ctx, disk_cache_info());
    if (detect_transition_into_here(ctx))
    {
        set(state, app_ctx.instance->shared_config);
        set(cache_info, get_summary_info(disk_cache));
        end_pass(ctx);
    }

    auto clamped_width = width(60, EM);

    column_layout top(ctx, GROW);
    scoped_substyle style(ctx, text("top-level-ui"));
    {
        clamped_header header(ctx, text("background"), text("header"),
            clamped_width);
        do_heading(ctx, text("heading"), text("Settings"));
    }
    {
        clamped_content background(ctx, text("background"), text("content"),
            clamped_width, GROW);

        form form(ctx);

        do_form_section_heading(form, text("Disk Cache"));
        {
            form_field field(form, text("Directory"));
            do_directory_controls(ctx, _field(state, cache_dir));
        }
        {
            form_field field(form, text("Size"));
            {
                row_layout row(ctx);
                do_text_control(ctx, _field(state, cache_size));
                do_text(ctx, text("GiB"));
            }
        }
        {
            form_field field(form, text("Status"));
            {
                row_layout row(ctx);
                do_styled_text(ctx, text("value-style"),
                    as_text(ctx,
                        accessor_cast<int>(_field(cache_info, n_entries))));
                do_text(ctx, text("entries"));
            }
            {
                row_layout row(ctx);
                do_styled_text(ctx, text("value-style"),
                    printf(ctx,
                        "%.3f",
                        scale(
                            accessor_cast<double>(
                                _field(cache_info, total_size)),
                            1. / double(0x40000000))));
                do_text(ctx, text("GiB"));
            }
            if (do_button(ctx, text("Clear")))
            {
                clear(disk_cache);
                set(cache_info, get_summary_info(disk_cache));
                end_pass(ctx);
            }
        }

        do_form_section_heading(form, text("Crash Reporting"));
        {
            form_field field(form, text("Directory"));
            do_directory_controls(ctx, _field(state, crash_dir));
            do_paragraph(ctx, text("Note that changes to the crash reporting directory won't take effect until the application is restarted."));
        }
    }
    {
        clamped_header footer(ctx, text("background"), text("footer"),
            clamped_width);
        {
            row_layout row(ctx);
            do_spacer(ctx, GROW);
            if (do_button(ctx, text("Done")))
            {
                set_shared_app_config(*app_ctx.instance,
                    sanitize_shared_app_config(get(state)));
                app_ctx.instance->selected_page = app_level_page::APP_CONTENTS;
                end_pass(ctx);
            }
            if (do_button(ctx, text("Cancel")))
            {
                app_ctx.instance->selected_page = app_level_page::APP_CONTENTS;
                end_pass(ctx);
            }
        }
    }
}

void static
do_notification_history_ui(
    gui_context& ctx, app_context& app_ctx, app_window& window)
{
    column_layout top(ctx, GROW);
    scoped_substyle style(ctx, text("top-level-ui"));
    auto clamped_width = width(60, EM);
    {
        clamped_content background(ctx, text("background"), text("content"),
            clamped_width, GROW);
        display_notification_history(ctx);
    }
    {
        clamped_header footer(ctx, text("background"), text("footer"),
            clamped_width);
        {
            row_layout row(ctx);
            do_spacer(ctx, GROW);
            if (do_button(ctx, text("Clear")))
            {
                clear_notification_history(ctx);
                end_pass(ctx);
            }
            auto& notifications = ctx.gui_system->notifications;
            if (do_button(ctx, text("Done")))
            {
                for (auto& notification : notifications.history)
                {
                    notification.seen = true;
                }
                app_ctx.instance->selected_page = app_level_page::APP_CONTENTS;
                end_pass(ctx);
            }
        }
    }
}

struct request_report_item
{
    string type;
    string info;
    string body;
};

struct request_report
{
    std::vector<request_report_item> requests;
};

request_report
get_request_report(gui_context& ctx)
{
    request_report rr;
    for (auto const& request : ctx.gui_system->request_list)
    //for (int i = 0; i < ctx.gui_system->request_list.size(); ++i)
    {
        //auto request = ctx.gui_system->request_list[i];
        switch (request.type)
        {
            case request_type::OBJECT:
                {
                    request_report_item itm;
                    itm.type = string("OBJECT: ") + as_object(request);
                    itm.info = as_object(request);
                    rr.requests.push_back(itm);
                    break;
                }
            case request_type::IMMUTABLE:
                {
                    request_report_item itm;
                    itm.type = string("IMMUTABLE") + as_immutable(request);
                    itm.info = as_immutable(request);
                    rr.requests.push_back(itm);
                    break;
                }
            case request_type::REMOTE_CALCULATION:
                {
                    auto r = as_thinknode_request(as_remote_calc(request));

                    request_report_item itm;
                    itm.type = "REMOTE_CALCULATION: " + to_string(r.type);

                    if (r.type == calculation_request_type::FUNCTION)
                    {
                        auto f = as_function(r);
                        itm.info = f.name;
                        itm.body = value_to_json(to_value(f));
                    }
                    else
                    {
                        itm.info = to_string(r.type);
                        itm.body = to_string(r.type);
                    }

                    rr.requests.push_back(itm);
                    break;
                }
            default:
                {
                    request_report_item itm;
                    itm.type = to_string(request.type);
                    itm.info = "none";
                    rr.requests.push_back(itm);
                }
        }
    }

    return rr;
}

std::vector<size_t>
in_order_indices(size_t count)
{
    std::vector<size_t> indices;
    for (size_t i = 0; i != count; ++i)
        indices.push_back(i);
    return indices;
}

template<class Resolver>
std::vector<size_t>
sorted_vector_indices(size_t item_count, Resolver const& resolver)
{
    auto indices = in_order_indices(item_count);
    sort(indices.begin(), indices.end(),
        [&](size_t a, size_t b) { return resolver(a) < resolver(b); });
    return indices;
}

void static
do_request_list(gui_context& ctx, accessor<request_report> const& report)
{
    auto x = sorted_vector_indices(get(report).requests.size(),
    [&](size_t i) { return get(report).requests[i].type; });

    for_each(ctx,
    [&](gui_context& ctx, size_t unused, // index of the index
    accessor<size_t> const& index)
    {
        auto info = select_index_via_accessor(_field(report, requests), index);

        tree_node node(ctx);
        do_text(ctx, _field(info, type));
        alia_if (node.do_children())
        {
            do_text(ctx, _field(info, info));
            tree_node data_node(ctx);
            do_text(ctx, text("data"));
            alia_if (data_node.do_children())
            {
                do_paragraph(ctx, _field(info,body));
            }
            alia_end
        }
        alia_end
    },
    in(x));
}

void static
do_request_list_report(gui_context& ctx)
{
    auto request_info = get_state(ctx, request_report());

    // Update the snapshot when transitioning into this UI.
    if (detect_transition_into_here(ctx))
    {
        set(request_info, get_request_report(ctx));
        end_pass(ctx);
    }

    //// Also periodically update the snapshot.
    //{
    //    timer t(ctx);
    //    if (!t.is_active())
    //        t.start(1000);
    //    if (t.triggered())
    //    {
    //        set(request_info, get_request_report(ctx));
    //        end_pass(ctx);
    //    }
    //}

    alia_if (do_button(ctx, text("Clear request list")))
    {
        ctx.gui_system->request_list.clear();
        set(request_info, get_request_report(ctx));
        end_pass(ctx);
    }
    alia_end

    do_request_list(ctx, request_info);
}

void static
do_dev_console_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    auto clamped_width = width(60, EM);

    column_layout top(ctx, GROW);
    scoped_substyle style(ctx, text("top-level-ui"));
    {
        clamped_header header(ctx, text("background"), text("header"),
            clamped_width);
        do_heading(ctx, text("heading"), text("Dev Console"));
    }
    {
        clamped_content background(ctx, text("background"), text("content"),
            clamped_width, GROW);

        auto dev_page = get_state(ctx, int(0));
        //bool checked = false;
        {
            row_layout r(ctx);
            if (do_button(ctx, text("Memory Cache")))
            {
                set(dev_page, 0);
            }
            if (do_button(ctx, text("Requests")))
            {
                set(dev_page, 1);
            }
        }
        alia_if (is_equal(dev_page, 0))
        {
            do_memory_cache_report(ctx);
        }
        alia_else_if (is_equal(dev_page, 1))
        {
            do_text(ctx, text("Requests"));
            do_request_list_report(ctx);
        }
        alia_else
        {
            do_text(ctx, text("NONE"));
        }
        alia_end
    }
    {
        clamped_header footer(ctx, text("background"), text("footer"),
            clamped_width);
        {
            row_layout row(ctx);
            do_spacer(ctx, GROW);
            if (do_button(ctx, text("Done")))
            {
                app_ctx.instance->selected_page = app_level_page::APP_CONTENTS;
                end_pass(ctx);
            }
        }
    }
}

void static
do_main_window_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    try
    {
        column_layout column(ctx);

        do_header_row(ctx, app_ctx, window);

        {
            layered_layout layering(ctx, GROW);

            alia_switch(app_ctx.instance->selected_page)
            {
             alia_case(app_level_page::APP_INFO):
                do_app_info_ui(ctx, app_ctx, window);
                break;
             alia_case(app_level_page::SETTINGS):
                do_settings_ui(ctx, app_ctx, window);
                break;
             alia_case(app_level_page::NOTIFICATIONS):
                do_notification_history_ui(ctx, app_ctx, window);
                break;
             alia_case(app_level_page::DEV_CONSOLE):
                do_dev_console_ui(ctx, app_ctx, window);
                break;
             alia_default:
                do_app_content_ui(ctx, app_ctx, window);
            }
            alia_end

            display_active_notifications(ctx);
        }
    }
    catch (std::exception& e)
    {
        // If an error occurs on a refresh or render pass, something more
        // serious is wrong, and the error will likely just occur over and
        // over again on subsequent passes.
        // TODO: It's not entirely clear what to do in this case. Also, there
        // are other events (including wrapped refresh/render events) that
        // shouldn't be generating exceptions, so this logic should be refined.
        if (!is_refresh_pass(ctx) && !is_render_pass(ctx))
            post_notification(ctx, new simple_notification(e.what()));
        end_pass(ctx);
    }
}

string static
get_app_version_from_realm_versions(
    std::vector<realm_version> const& versions,
    string const& app_name)
{
    string version = "none";
    for(auto v : versions)
    {
        if (v.app == app_name && v.status == "installed")
        {
            version = v.version;
        }
    }
    return version;
}

string static
make_realm_version_url(string const& base_url, string const& realm_name)
{
    string name = realm_name;
    if (name == "") { name = "developers"; }
    return base_url + to_string("/iam/realms/") + name + to_string("/versions");
}

bool static realms_versions_match(string realm_id, string installed_realm_id)
{
    return realm_id == installed_realm_id;
}

void static
select_realm(app_context& app_ctx, accessor<string> const& realm_id,
    accessor<bool> const& remember_realm, accessor<bool> const& version_id_valid,
    accessor<bool> const& is_development)
{

    select_realm(*app_ctx.instance, get(realm_id));
    auto& config = app_ctx.instance->config.nonconst_get();
    if (get(remember_realm) && (is_true(version_id_valid) || is_true(is_development)))
        config.realm_id = get(realm_id);
    else
        config.realm_id = none;
}

void static
start_sign_in(app_context& app_ctx, accessor<string> const& username,
    accessor<bool> const& remember_username, accessor<string> const& password)
{
    start_sign_in(*app_ctx.instance, get(username), get(password));
    auto& config = app_ctx.instance->config.nonconst_get();
    if (get(remember_username))
        config.username = get(username);
    else
        config.username = none;
}

template<class DoContent>
void static
do_top_level_dialog(
    gui_context& ctx, app_context& app_ctx, app_window& window, bool do_logo,
    DoContent const& do_content)
{
    column_layout column(ctx, GROW);

    do_header_row(ctx, app_ctx, window);

    {
        panel background_panel(ctx, text("top-level-background"), GROW);

        layered_layout layered(ctx, GROW);

        // Do the background graphics.
        {
            row_layout row(ctx);
            auto svg = text("<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" width=\"144\" height=\"160\"><rect id=\"backgroundrect\" width=\"100%\" height=\"100%\" x=\"0\" y=\"0\" fill=\"none\" stroke=\"none\"/>\r\n<g class=\"currentLayer\"><title>Layer 1</title><path fill=\"none\" stroke=\"#888\" stroke-width=\"1\" d=\"M144,80 c-39.8,0 -72,-32.2 -72,-72 c0,39.8 -32.2,72 -72,72 c39.8,0 72,32.2 72,72 C72,112 104,80 144,80 z\" id=\"svg_1\" class=\"selected\"/></g>\r\n</svg>\r\n\r\n");
            do_svg_graphic(ctx, text("left-astroid"), svg, GROW);
            do_svg_graphic(ctx, text("right-astroid"), svg, GROW);
        }

        // Do the logo.
        alia_if (do_logo)
        {
            // Position the logo by adding a larger spacer in the middle that
            // simulates the space taking up by a dialog.
            column_layout column(ctx);
            do_app_logo(ctx,
                _field(get_app_info(ctx, app_ctx), logo),
                layout(height(5, EM), FILL_X | CENTER_Y, 1));
            do_spacer(ctx, height(20, EM));
            do_spacer(ctx,
                layout(height(5, EM), FILL_X | CENTER_Y, 1));
        }
        alia_end

        // Do the actual dialog.
        {
            do_spacer(ctx, GROW);
            {
                panel dialog_panel(ctx, text("top-level-dialog"), CENTER);
                do_content();
            }
            do_spacer(ctx, GROW);
        }
    }
}

void static
do_authentication_ui(
    gui_context& ctx, app_context& app_ctx, app_window& window,
    accessor<background_authentication_status> const& auth_status,
    accessor<string> const& username)
{
    do_top_level_dialog(ctx, app_ctx, window, true,
        [&]() // do_content
        {
            state_accessor<bool> remember_username;
            if (get_state(ctx, &remember_username))
            {
                auto const& config = app_ctx.instance->config.get();
                set(remember_username,
                    config.username && get(config.username) == get(username));
            }

            auto password = get_state(ctx, string());
            // When (re-)entering the authentication UI, clear the password and
            // reset the UI (cause we might have gotten here involuntarily if our
            // session token expires).
            if (detect_transition_into_here(ctx))
            {
                reset_ui(ctx, app_ctx);
                set(password, string(""));
                end_pass(ctx);
            }

            auto auth_state = get(auth_status).state;

            // Is this the initial authentication?
            // (The initial authentication is initiated via the command-line.)
            // If there's no request or we detect a failure, we know it's not.
            auto is_initial_authentication = get_state(ctx, true);
            alia_if (
                auth_state == background_authentication_state::NO_CREDENTIALS ||
                is_failure(auth_state))
            {
                if (detect_transition_into_here(ctx))
                {
                    set(is_initial_authentication, false);
                    end_pass(ctx);
                }
            }
            alia_end

            // If this is the initial authentication, don't show the controls.
            alia_if (!get(is_initial_authentication))
            {
                do_heading(ctx, text("heading"), text("Sign In"), CENTER);

                do_separator(ctx);

                do_spacer(ctx, height(0.25, EM));

                {
                    grid_layout grid(ctx);
                    {
                        grid_row row(grid);
                        do_text(ctx, text("Username"));
                        auto result =
                            do_unsafe_text_control(ctx, username, GROW);
                        if (result.event == TEXT_CONTROL_ENTER_PRESSED)
                        {
                            start_sign_in(app_ctx, username, remember_username,
                                password);
                            end_pass(ctx);
                        }
                    }
                    {
                        grid_row row(grid);
                        do_text(ctx, text("Password"));
                        auto result =
                            do_unsafe_text_control(ctx, password, GROW,
                                TEXT_CONTROL_MASK_CONTENTS);
                        if (result.event == TEXT_CONTROL_ENTER_PRESSED)
                        {
                            start_sign_in(app_ctx, username, remember_username,
                                password);
                            end_pass(ctx);
                        }
                    }
                }

                do_spacer(ctx, height(0.25, EM));

                do_check_box(ctx, remember_username,
                    text("Remember my username on this computer."),
                    text("Remembers the username on this computer for the last user\n"
                        "successfully logged in."));

                do_separator(ctx);

                do_spacer(ctx, height(0.25, EM));
            }
            alia_end

            alia_if (is_failure(auth_state))
            {
                panel p(ctx, text("error-panel"));

                do_paragraph(ctx, text(get_description(auth_state)));

                alia_if (!get(auth_status).message.empty())
                {
                    auto json_message = _field(ref(&auth_status), message);
                    if(is_gettable(json_message))
                    {
                        // If not the standard invalid username/password error, show json.
                        // Could be useful for more complex connection problems
                        if (get(json_message).find("Invalid username") == string::npos)
                        {
                            do_paragraph(ctx, _field(ref(&auth_status), message));
                        }
                    }
                }
                alia_end
            }
            alia_end

            alia_if (auth_state == background_authentication_state::IN_PROGRESS)
            {
                {
                    row_layout row(ctx);
                    do_animated_astroid(ctx, size(4, 4, EM));
                    do_heading(ctx, text("status"), text("Authenticating..."),
                        CENTER);
                }
                do_spacer(ctx, height(0.25, EM));
            }
            alia_end

            alia_if (!get(is_initial_authentication))
            {
                row_layout row(ctx);
                do_spacer(ctx, GROW);
                alia_untracked_if (do_button(ctx, text("Exit")))
                {
                    window.close();
                    end_pass(ctx);
                }
                alia_end
                alia_untracked_if (do_primary_button(ctx, text("Sign In")))
                {
                    start_sign_in(app_ctx, username, remember_username, password);
                    end_pass(ctx);
                }
                alia_end
            }
            alia_end
        });
}

void static
do_realm_list_ui(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& realm_id,
    accessor<bool> const& remember_realm,
    accessor<string> const& version_selected,
    accessor<bool> const& valid_version_selected)
{
    auto show_all_realms = get_state(ctx, bool(false));
    //state_accessor<bool> show_all_realms;
    //set(show_all_realms, true);
    auto app_ver = app_ctx.instance->info.local_version_id;
    do_heading(ctx, text("sub-heading"), in(app_ver), CENTER);

    do_separator(ctx);
    do_spacer(ctx, height(0.25, EM));
    {
        scrollable_panel list_panel(ctx, text("item-list"), default_layout,
            PANEL_NO_HORIZONTAL_SCROLLING);

        auto realms =
            gui_get_request<std::vector<realm> >(ctx,
                printf(ctx, "%s/iam/realms", get_api_url(ctx, app_ctx)),
                in(no_headers));

        for_each(ctx,
            [&](gui_context& ctx, size_t index, accessor<realm> const& realm)
            {
                auto installed_versions =
                    gui_get_request<std::vector<realm_version> >(ctx,
                        gui_apply(ctx,
                            make_realm_version_url,
                            get_api_url(ctx, app_ctx),
                            _field(realm, name)),
                        in(no_headers));

                auto app_name = app_ctx.instance->info.thinknode_app_id;
                auto v =
                    gui_apply(ctx,
                        get_app_version_from_realm_versions,
                        installed_versions,
                        in(app_name));


                auto valid_version = gui_apply(ctx,
                                        realms_versions_match,
                                        in(app_ver),
                                        v);

                alia_if(is_equal(v, app_ver) || is_true(_field(realm, development)))
                {
                    auto panel_id = get_widget_id(ctx);
                    clickable_panel item_panel(ctx, text("item"), default_layout,
                        get(realm_id) == get(realm).name ? PANEL_SELECTED : NO_FLAGS,
                        panel_id);
                    if (item_panel.clicked())
                    {
                        set(version_selected, get(v));
                        set(valid_version_selected, get(valid_version));
                        set(realm_id, get(realm).name);
                        end_pass(ctx);
                    }
                    if (detect_double_click(ctx, panel_id, LEFT_BUTTON))
                    {
                        set(realm_id, get(realm).name);
                        app_ctx.instance->info.thinknode_version_id = get(v);
                        select_realm(app_ctx, realm_id, remember_realm, valid_version, _field(realm, development));
                        end_pass(ctx);
                    }
                    do_heading(ctx, text("heading"), _field(ref(&realm), name));
                    do_paragraph(ctx, _field(ref(&realm), description));
                    do_text(ctx, v);
                }
                alia_else_if(is_true(show_all_realms))
                {
                    panel item_panel(ctx, text("inactive-item"));
                    {
                        row_layout r(ctx);
                        do_heading(ctx, text("heading"), _field(ref(&realm), name));
                        do_text(ctx, text(" (version mismatch)"));
                    }

                    do_paragraph(ctx, _field(ref(&realm), description));
                    do_text(ctx, v);
                }
                alia_end
            },
            realms);
        }

        do_spacer(ctx, height(0.25, EM));

        do_check_box(ctx, show_all_realms, text("Show all realms."));
}

void static
do_realm_selection_ui(
    gui_context& ctx, app_context& app_ctx, app_window& window,
    accessor<background_context_request_status> const& request_status,
    accessor<string> const& realm_id)
{
    do_top_level_dialog(ctx, app_ctx, window, false,
        [&]() // do_content
        {
            state_accessor<bool> remember_realm;
            auto version_selected = get_state(ctx, string(""));
            auto valid_version_selected = get_state(ctx, false);

            alia_untracked_if (get_state(ctx, &remember_realm))
            {
                auto const& config = app_ctx.instance->config.get();
                set(remember_realm,
                        config.realm_id &&
                        get(config.realm_id) == get(realm_id));
                end_pass(ctx);
            }
            alia_end

            // Is this the initial request?
            // (The initial request is initiated via the command-line.)
            // If there's no request or we detect a failure, we know it's not.
            auto is_initial_request = get_state(ctx, optional<bool>());

            alia_if(!is_initial_request)
            {
                // If a realm is preselected from the config or command-line try to
                // connect to it.
                alia_if(realm_id != in(string("")))
                {
                    auto cl_realm =
                        gui_get_request<realm>(ctx,
                            printf(ctx, "%s/iam/realms/%s",
                                get_api_url(ctx, app_ctx), realm_id),
                            in(no_headers));

                    auto installed_versions =
                        gui_get_request<std::vector<realm_version> >(ctx,
                            gui_apply(ctx,
                                make_realm_version_url,
                                get_api_url(ctx, app_ctx),
                                _field(cl_realm, name)),
                            in(no_headers));

                    auto app_name = app_ctx.instance->info.thinknode_app_id;

                    auto v =
                        gui_apply(ctx,
                            get_app_version_from_realm_versions,
                            installed_versions,
                            in(app_name));

                    auto valid_version =
                        gui_apply(ctx,
                            realms_versions_match,
                            in(app_ctx.instance->info.local_version_id),
                            v);

                    alia_if(valid_version || _field(cl_realm, development))
                    {
                        select_realm(app_ctx,
                            realm_id,
                            remember_realm,
                            valid_version,
                            _field(cl_realm, development));
                        set(is_initial_request, some(true));
                        end_pass(ctx);
                    }
                    alia_else
                    {
                        set(is_initial_request, some(false));
                        end_pass(ctx);
                    }
                    alia_end
                }
                alia_else
                {
                    set(is_initial_request, some(false));
                    end_pass(ctx);
                }
                alia_end
            }
            alia_end

            auto request_state = get(request_status).state;

            alia_if(unwrap_optional(is_initial_request))
            {
                if (request_state == background_context_request_state::NO_REQUEST ||
                    is_failure(request_state))
                {
                    set(is_initial_request, some(false));
                    end_pass(ctx);
                }
            }
            alia_end

            // If this is the initial request, don't show the controls.
            alia_if (!unwrap_optional(is_initial_request))
            {
                do_heading(ctx, text("heading"), text("Realm Selection"),
                    CENTER);

                do_realm_list_ui(ctx, app_ctx, realm_id, remember_realm, version_selected, valid_version_selected);

                //do_spacer(ctx, height(0.25, EM));

                do_separator(ctx);

                do_check_box(ctx, remember_realm,
                    text("Remember my selection on this computer."));
            }
            alia_end

            alia_if (is_failure(request_state))
            {
                panel p(ctx, text("error-panel"));

                do_paragraph(ctx, text(get_description(request_state)));

                alia_if (!get(request_status).message.empty())
                {
                    auto json_message = _field(ref(&request_status), message);
                    if(is_gettable(json_message))
                    {
                        // If not the standard invalid username/password error, show json.
                        // Could be useful for more complex connection problems
                        if (get(json_message).find("Invalid username") == string::npos)
                        {
                            do_paragraph(ctx, _field(ref(&request_status), message));
                        }
                    }
                }
                alia_end
            }
            alia_end

            alia_if (request_state ==
                background_context_request_state::IN_PROGRESS)
            {
                {
                    row_layout row(ctx);
                    do_animated_astroid(ctx, size(4, 4, EM));
                    do_heading(ctx, text("status"),
                        text("Connecting to realm..."), CENTER);
                }
                do_spacer(ctx, height(0.5, EM));
            }
            alia_end

            alia_if (!unwrap_optional(is_initial_request))
            {

                row_layout row(ctx);
                do_spacer(ctx, GROW);
                if (do_button(ctx, text("Sign Out")))
                {
                    log_out(ctx, app_ctx);
                    end_pass(ctx);
                }

                alia_untracked_if (do_primary_button(ctx, text("Connect"), default_layout,
                    !is_equal(version_selected, string("")) ? NO_FLAGS : BUTTON_DISABLED))
                {
                    app_ctx.instance->info.thinknode_version_id = get(version_selected);
                    select_realm(app_ctx, realm_id, remember_realm, valid_version_selected, in(true));
                    end_pass(ctx);
                }
                alia_end


            }
            alia_end



        });
}

void static
do_top_level_ui(gui_context& ctx, app_context& app_ctx, app_window& window)
{
    {
        menu_bar mb(ctx);
        alia_if (!window.is_full_screen())
        {
            {
                submenu menu(ctx, text("Session"));
                if (do_menu_option(ctx, text("Switch Realms")))
                {
                    leave_realm(ctx, app_ctx);
                    end_pass(ctx);
                }
                if (do_menu_option(ctx, text("Sign Out")))
                {
                    log_out(ctx, app_ctx);
                    end_pass(ctx);
                }
                if (do_menu_option(ctx, text("Exit")))
                {
                    window.close();
                    end_pass(ctx);
                }
            }
            {
                submenu menu(ctx, text("View"));
                if (do_menu_option(ctx, text("Full Screen")))
                {
                    toggle_full_screen(ctx, window);
                    end_pass(ctx);
                }
                do_menu_separator(ctx);
                if (do_menu_option(ctx, text("App Info")))
                {
                    app_ctx.instance->selected_page =
                        app_level_page::APP_INFO;
                    end_pass(ctx);
                }
                if (do_menu_option(ctx, text("Notifications")))
                {
                    app_ctx.instance->selected_page =
                        app_level_page::NOTIFICATIONS;
                    end_pass(ctx);
                }
                if (do_menu_option(ctx, text("Settings")))
                {
                    app_ctx.instance->selected_page =
                        app_level_page::SETTINGS;
                    end_pass(ctx);
                }
                if (do_menu_option(ctx, text("Developer Console")))
                {
                    app_ctx.instance->selected_page =
                        app_level_page::DEV_CONSOLE;
                    end_pass(ctx);
                }
            }
        }
        alia_end
    }

    auto& instance = *app_ctx.instance;
    auto& system = *instance.gui_system;

    if (is_refresh_pass(ctx))
        issue_permanent_failure_notifications(ctx);

    if (is_refresh_pass(ctx))
        update_notifications(ctx);

    if (is_refresh_pass(ctx))
        process_mutable_cache_updates(*system.bg);

    // Keep the memory cache down at a reasonable size.
    if (is_refresh_pass(ctx))
        reduce_memory_cache_size(*system.bg, 1024); // in MB; TODO: Make this configurable.

    // Update the authentication status.
    auto auth_status =
        get_state(ctx,
            background_authentication_status(
                background_authentication_state::IN_PROGRESS));
    if (is_refresh_pass(ctx))
        set(auth_status, get_authentication_status(*system.bg));

    // Update the context retrieval status.
    auto context_status =
        get_state(ctx,
            background_context_request_status(
                background_context_request_state::IN_PROGRESS));
    if (is_refresh_pass(ctx))
    {
        background_context_request_status status;
        framework_context response;
        get_context_request_result(*system.bg, &status, &response);
        set(context_status, status);
        auto new_context =
            status.state == background_context_request_state::SUCCEEDED ?
            some(response) : none;
        if (system.framework_context.get() != new_context)
            system.framework_context.set(new_context);
    }

    // If we're missing either authentication or context info, do those UIs
    // instead of the normal app one.
    alia_if (get(auth_status).state !=
        background_authentication_state::SUCCEEDED)
    {
        do_authentication_ui(ctx, app_ctx, window, auth_status,
            make_accessor(app_ctx.instance->username));
    }
    alia_else_if (get(context_status).state !=
        background_context_request_state::SUCCEEDED)
    {
        do_realm_selection_ui(ctx, app_ctx, window, context_status,
            make_accessor(app_ctx.instance->realm_id));
    }
    alia_else
    {
        do_main_window_ui(ctx, app_ctx, window);
    }
    alia_end

    sync_app_config(ctx, instance.info.thinknode_app_id,
        make_accessor(instance.config));

    sync_window_state(ctx, window,
        _field(make_accessor(instance.config), window_state));

    // ctrl-plus (or ctrl-equal) and ctrl-minus adjust the font size.
    // ctrl-0 resets it.
    if (detect_key_press(ctx, key_code('='), KMOD_CTRL))
    {
        set_magnification_factor(*ctx.system, app_ctx,
            get_magnification_factor(app_ctx) * 1.1f);
    }
    if (detect_key_press(ctx, key_code('+'), KMOD_CTRL))
    {
        set_magnification_factor(*ctx.system, app_ctx,
            get_magnification_factor( app_ctx) * 1.1f);
    }
    if (detect_key_press(ctx, key_code('-'), KMOD_CTRL))
    {
        set_magnification_factor(*ctx.system, app_ctx,
            get_magnification_factor( app_ctx) / 1.1f);
    }
    if (detect_key_press(ctx, key_code('0'), KMOD_CTRL))
    {
        set_magnification_factor(*ctx.system, app_ctx, 1);
    }
    // F11 toggles full screen mode.
    if (detect_key_press(ctx, KEY_F11))
    {
        toggle_full_screen(ctx, window);
        end_pass(ctx);
    }

    // F5 refreshes mutable web requests.
    if (detect_key_press(ctx, KEY_F5))
    {
        clear_mutable_data_cache(get_background_system(ctx));
        // Also refresh the style file.
        set_system_style(*ctx.system,
            parse_style_file(instance.style_file_path.c_str()));
        end_pass(ctx);
    }
}

void app_window_controller::do_ui(alia::ui_context& ctx)
{
    scoped_gui_context scoped_context(ctx, *instance->gui_system);
    gui_context& cradle_ctx = scoped_context.context();

    app_context* app_ctx;
    if (get_data(ctx, &app_ctx))
        initialize_app_context(*app_ctx, *instance);

    resolve_gui_app_contexts(cradle_ctx, *app_ctx);

    do_top_level_ui(cradle_ctx, *app_ctx, *this->window);

    // If we just completed a refresh pass, this is a good time to write back
    // buffered state if we were requested to do so.
    if (is_refresh_pass(ctx))
    {
        if (instance->state_write_back_requested)
        {
            {
                state_write_back_event event;
                issue_event(*ctx.system, event);
            }
            instance->state_write_back_requested = false;
        }
    }
}

}
