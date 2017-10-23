#ifndef CRADLE_GUI_NOTIFICATIONS_HPP
#define CRADLE_GUI_NOTIFICATIONS_HPP

#include <cradle/gui/common.hpp>
#include <cradle/date_time.hpp>
#include <alia/ui/utilities/timing.hpp>

// This file defines the interface for posting notifications within the UI.

namespace cradle {

struct notification_controller
{
    virtual ~notification_controller() {}
    virtual void do_ui(gui_context& ctx) = 0;
    virtual char const* overlay_style() const
    { return "notification-overlay"; }

    data_block block;
};

struct notification_content
{
    cradle::time clock_time;
    bool seen;
    alia__shared_ptr<notification_controller> controller;
};

struct active_notification
{
    notification_content content;
    ui_time_type ui_time;
    value_smoother<float> opacity;
    bool expired;
};

struct simple_notification : notification_controller
{
    simple_notification(string const& message)
      : message_(message) {}
    void do_ui(gui_context& ctx)
    {
        do_paragraph(ctx, make_custom_getter(&message_, get_id(id_)));
    }
 private:
    string message_;
    local_identity id_;
};

struct notification_system
{
    std::vector<notification_content> history;
    std::deque<active_notification> active;
};

struct notification_context
{
    notification_system* system;
};

// Post a new notification to the GUI.
void post_notification(gui_context& ctx, notification_controller* contoller);

// Update the notification system for the given context.
// Currently, this just takes care of moving expired notifications out of the
// active list and into the history.
void update_notifications(gui_context& ctx);

// Display active notifications.
// This is intended to be layered over the UI.
void display_active_notifications(gui_context& ctx);

// Display the history of all notifications.
// (This uses normal layout.)
void display_notification_history(gui_context& ctx);

// Clear the notification history.
void clear_notification_history(gui_context& ctx);

}

#endif
