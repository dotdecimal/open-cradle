#include <gui_demo/utilities.hpp>

namespace gui_demo {

DEFINE_DEMO(
    hello_world_demo, "Hello, World!",
    "This is the \"Hello, World!\" UI in alia.",
    void do_ui(gui_context& ctx)
    {
        do_text(ctx, text("Hello, World!"));
    })

DEFINE_DEMO(
    event_handling_demo, "Event Handling",
    "This demonstrates how event handling works in alia.",
    void do_ui(gui_context& ctx)
    {
        static string message = "Please!";
        if (do_button(ctx, text("Push me!")))
        {
            message = "Thanks!";
        }
        do_text(ctx, in(message));
    })

DEFINE_DEMO(
    controls_demo, "Controls",
    "This demostrates how simple controls work in alia.",
    void do_ui(gui_context& ctx)
    {
        static bool checked = false;
        do_check_box(ctx, inout(&checked), text("Check me"));
        do_text(ctx, text(checked ? "Thanks!" : "Please!"));
    })

DEFINE_DEMO(
    conditional_widgets_demo, "Conditional Widgets",
    "This demostrates how to implement widgets that are only present under certain conditions.",
    void do_ui(gui_context& ctx)
    {
        static bool checked = false;
        do_check_box(ctx, inout(&checked), text("Show text"));
        alia_if (checked)
        {
            do_text(ctx, text("Hello!"));
        }
        alia_end
    })

static demo_interface* introduction_demos[] =
  { &hello_world_demo, &event_handling_demo, &controls_demo,
    &conditional_widgets_demo, 0 };

static demo_section introduction_section =
    { "Introduction", "This page contains working demonstrations of the examples in the alia tutorial.\n\nNote that the tutorial itself contains much more detailed descriptions of these examples.", introduction_demos };

static demo_section* section_list[] =
  { &introduction_section, 0 };

demo_page tutorial_page =
    { "Tutorial", section_list };
    
}
