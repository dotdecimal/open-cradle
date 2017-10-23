#include <cradle/gui/notifications.hpp>
#include <cradle/gui/internals.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/regions.hpp>

#include <cradle/gui/widgets.hpp>

namespace cradle {

void post_notification(gui_context& ctx, notification_controller* contoller)
{
    active_notification n;
    n.content.controller =
        alia__shared_ptr<notification_controller>(contoller);
    n.content.clock_time = boost::posix_time::second_clock::local_time();
    n.content.seen = false;
    n.ui_time = get_animation_tick_count(ctx);
    request_animation_refresh(ctx);
    n.expired = false;
    reset_smoothing(n.opacity, 0.f);
    ctx.gui_system->notifications.active.push_back(n);
}

void update_notifications(gui_context& ctx)
{
    auto& notifications = ctx.gui_system->notifications;
    if (is_refresh_pass(ctx))
    {
        auto& active = notifications.active;
        while (!active.empty() && active.front().expired &&
            active.front().opacity.new_value == 0 &&
            !active.front().opacity.in_transition)
        {
            notifications.history.push_back(active.front().content);
            active.pop_front();
        }
    }
}

void display_active_notifications(gui_context& ctx)
{
    auto& notifications = ctx.gui_system->notifications;
    alia_if (!notifications.active.empty())
    {
        column_layout column(ctx, UNPADDED | BOTTOM | RIGHT);

        ui_time_type const notification_duration = 5000;
        ui_time_type const notification_transition_time = 500;

        alia_for (auto& n : notifications.active)
        {
            float opacity =
                smooth_raw_value(ctx, n.opacity, n.expired ? 0.f : 1.f,
                    animated_transition(default_curve,
                        notification_transition_time));
            scoped_surface_opacity scoped_opacity(ctx, opacity);
            panel p(ctx, text(n.content.controller->overlay_style()));
            if (get_animation_ticks_left(ctx,
                    n.ui_time + notification_duration) <= 0 &&
                is_render_pass(ctx) &&
                !is_mouse_inside_box(ctx, box<2,double>(p.outer_region())))
            {
                n.expired = true;
                request_animation_refresh(ctx);
            }
            {
                scoped_data_block block(ctx, n.content.controller->block);
                n.content.controller->do_ui(ctx);
            }
        }
        alia_end
    }
    alia_end
}

void display_notification_history(gui_context& ctx)
{
    auto& notifications = ctx.gui_system->notifications;
    bool first = true;
    alia_for(auto const& notification : notifications.history)
    {
        alia_if (!first)
        {
            do_separator(ctx);
        }
        alia_else
        {
            first = false;
        }
        alia_end
        alia_if(notification.seen)
        {
            do_styled_text(ctx,
                text("timestamp"),
                in(to_string(notification.clock_time.date())
                    + " " + to_string(notification.clock_time.time_of_day())));
            {
                scoped_data_block block(ctx, notification.controller->block);
                notification.controller->do_ui(ctx);
            }
        }
        alia_else
        {
            {
                row_layout r(ctx);
                do_styled_text(ctx,
                    text("timestamp"),
                    in(to_string(notification.clock_time.date())
                        + " " + to_string(notification.clock_time.time_of_day())));
                scoped_substyle style(ctx, text("close-buttons"));
                do_spacer(ctx, GROW_X);
                scoped_substyle style2(ctx, text("emphasized"));
                do_text(ctx, text("NEW"));
            }
            {
                scoped_data_block block(ctx, notification.controller->block);
                notification.controller->do_ui(ctx);
            }
        }
        alia_end
    }
    alia_end
}

void clear_notification_history(gui_context& ctx)
{
    auto& notifications = ctx.gui_system->notifications;
    notifications.history.clear();
}

}
