#include "utilities.hpp"

ALIA_DEFINE_DEMO(
    text_control_demo, "Text Control", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<string> text = get_state<string>(ctx, "");
        do_text_control(ctx, text);
        do_paragraph(ctx, text);
    })

ALIA_DEFINE_DEMO(
    check_box_demo, "Check Box", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<bool> checked = get_state(ctx, false);
        do_check_box(ctx, checked, text("Check me"));
    })

ALIA_DEFINE_DEMO(
    radio_button_demo, "Radio Buttons", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<int> selection = get_state(ctx, 0);
        bool checked = false;
        do_radio_button_with_description(ctx, in(checked), text("An option"), text("An option"));
        do_radio_button_with_description(ctx, in(checked), 
            text("An option with a long description - Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris vulputate lectus vel odio euismod in dapibus justo mattis. Vestibulum semper pellentesque ultrices."), text("An option"));
        do_radio_button_with_description(ctx, in(checked), text("Another option"), text("An option"));
    })

ALIA_DEFINE_DEMO(
    slider_demo, "Slider", "The following produces a slider for a value that ranges from 0 to 10 in increments of 0.1.",
    void do_ui(ui_context& ctx)
    {
        row_layout row(ctx);
        state_accessor<double> x = get_state(ctx, 0.);
        do_slider(ctx, x, 0, 10, 0.1);
        do_text(ctx, x);
    })

ALIA_DEFINE_DEMO(
    ddl_demo, "Drop Down List", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<int> selection = get_state(ctx, 0);
        drop_down_list<int> ddl(ctx, selection);
        do_text(ctx, printf(ctx, "Item %d", selection));
        alia_if (ddl.do_list())
        {
            for (int i = 0; i != 20; ++i)
            {
                ddl_item<int> item(ddl, i);
                do_text(ctx, printf(ctx, "Item %d", in(i)));
            }
        }
        alia_end
    })

static demo_interface* control_demos[] =
    { &text_control_demo, &check_box_demo, &radio_button_demo, &slider_demo, &ddl_demo, 0 };

static demo_section controls_section =
    { "Controls", "", control_demos };

ALIA_DEFINE_DEMO(
    simple_button_demo, "Simple Button", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<int> click_count = get_state(ctx, 0);
        do_text(ctx, printf(ctx, "clicks: %d", click_count));
        if (do_button(ctx, text("Click Me!")))
        {
            set(click_count, get(click_count) + 1);
        }
    })

ALIA_DEFINE_DEMO(
    link_demo, "Link Button", "A link operates the same as a button, but it looks like a web link.",
    void do_ui(ui_context& ctx)
    {
        state_accessor<int> click_count = get_state(ctx, 0);
        do_text(ctx, printf(ctx, "clicks: %d", click_count));
        if (do_link(ctx, text("click me")))
        {
            set(click_count, get(click_count) + 1);
        }
    })

static demo_interface* button_demos[] =
    { &simple_button_demo, &link_demo, 0 };

static demo_section buttons_section =
    { "Buttons", "", button_demos };

static demo_section* section_list[] =
    { &controls_section, &buttons_section, 0 };

demo_page widgets_page =
    { "Widgets", section_list};
