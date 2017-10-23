#include "utilities.hpp"

ALIA_DEFINE_DEMO(
    form_demo, "Forms", "",
    void do_ui(ui_context& ctx)
    {
        form form(ctx);
        {
            form_field field(form, text("Email"));
            static string email;
            do_text_control(ctx, inout(&email));
        }
        {
            form_field field(form, text("Password"));
            static string password;
            do_text_control(ctx, inout(&password));
        }
        {
            form_buttons buttons(form);
            do_button(ctx, text("Submit"));
        }
    })

static demo_interface* form_demos[] =
    { &form_demo, 0 };

static demo_section forms_section =
    { "Forms", "", form_demos };

ALIA_DEFINE_DEMO(
    simple_tree_node_demo, "Simple Tree Node", "",
    void do_ui(ui_context& ctx)
    {
        tree_node node(ctx);
        do_text(ctx, text("Some Text"));
        alia_if (node.do_children())
        {
            do_paragraph(ctx, text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris vulputate lectus vel odio euismod in dapibus justo mattis. Vestibulum semper pellentesque ultrices. Nam justo metus, pellentesque in sodales id, viverra id elit. In hac habitasse platea dictumst. Aenean et ullamcorper sapien. Duis sit amet nibh leo, vitae varius velit. Proin pretium libero non libero scelerisque tincidunt. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Pellentesque tempus viverra mi, vel euismod sem aliquet vitae. Vestibulum pellentesque dignissim sem non sagittis. Etiam imperdiet interdum ligula ac malesuada. Cras et dui magna. Mauris sodales enim vel est pulvinar vel consequat neque blandit. Phasellus eu elit vel erat interdum ultrices."));
        }
        alia_end
    })

ALIA_DEFINE_DEMO(
    factor_trees_demo, "Recursive Tree Views",
    "This demonstrates the use of recursive functions to specify tree views.\n\n"
    "Given an integer input, this recursively produces a UI to represent the factor tree for that integer.",
    int factor(int n)
    {
        int i;
        for (i = int(std::sqrt(n) + 0.5); i > 1 && n % i != 0; --i)
            ;
        return i;
    }

    void do_factor_tree(ui_context& ctx, int n)
    {
        int f = factor(n);
        alia_if (f != 1)
        {
            do_text(ctx, printf(ctx, "%i: composite", in(n)));
            {
                tree_node node(ctx);
                do_text(ctx, text("factors"));
                alia_if (node.do_children())
                {
                    do_factor_tree(ctx, n / f);
                    do_factor_tree(ctx, f);
                }
                alia_end
            }
        }
        alia_else
        {
            do_text(ctx, printf(ctx, "%i: prime", in(n)));
        }
        alia_end
    }

    void do_ui(ui_context& ctx)
    {
        state_accessor<int> n = get_state(ctx, 1);
        do_text_control(ctx, enforce_min(n, in(1)));
        do_factor_tree(ctx, get(n));
    })

ALIA_DEFINE_DEMO(
    accordion_demo, "Accordions", "",
    void do_ui(ui_context& ctx)
    {
        accordion accordion(ctx);
        {
            accordion_section section(accordion);
            do_text(ctx, text("Some Text"));
            alia_if (section.do_content())
            {
                do_paragraph(ctx, text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris vulputate lectus vel odio euismod in dapibus justo mattis. Vestibulum semper pellentesque ultrices. Nam justo metus, pellentesque in sodales id, viverra id elit. In hac habitasse platea dictumst. Aenean et ullamcorper sapien. Duis sit amet nibh leo, vitae varius velit. Proin pretium libero non libero scelerisque tincidunt."));
            }
            alia_end
        }
        {
            accordion_section section(accordion);
            do_text(ctx, text("More Text"));
            alia_if (section.do_content())
            {
                do_paragraph(ctx, text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Pellentesque tempus viverra mi, vel euismod sem aliquet vitae. Vestibulum pellentesque dignissim sem non sagittis. Etiam imperdiet interdum ligula ac malesuada. Cras et dui magna. Mauris sodales enim vel est pulvinar vel consequat neque blandit. Phasellus eu elit vel erat interdum ultrices."));
            }
            alia_end
        }
    })

static demo_interface* collapsible_demos[] =
    { &simple_tree_node_demo, &factor_trees_demo, &accordion_demo, 0 };

static demo_section collapsibles_section =
    { "Collapsibles", "", collapsible_demos };

ALIA_DEFINE_DEMO(
    tab_demo, "Tabs", "",
    void do_ui(ui_context& ctx)
    {
        //state_accessor<int> selected_tab = get_state(ctx, 0);
        int currentTab = 0;
        bool checked = false;
        {
            tab_strip strip(ctx);
            do_tab(ctx, in(checked),
                text("some text"));
            do_tab(ctx, in(checked),
                text("more text"));
        }
        alia_if (currentTab == 0)
        {
            do_paragraph(ctx, text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris vulputate lectus vel odio euismod in dapibus justo mattis. Vestibulum semper pellentesque ultrices. Nam justo metus, pellentesque in sodales id, viverra id elit. In hac habitasse platea dictumst. Aenean et ullamcorper sapien. Duis sit amet nibh leo, vitae varius velit. Proin pretium libero non libero scelerisque tincidunt."));
        }
        alia_else
        {
            do_paragraph(ctx, text("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cum sociis natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Suspendisse potenti. Pellentesque tempus viverra mi, vel euismod sem aliquet vitae. Vestibulum pellentesque dignissim sem non sagittis. Etiam imperdiet interdum ligula ac malesuada. Cras et dui magna. Mauris sodales enim vel est pulvinar vel consequat neque blandit. Phasellus eu elit vel erat interdum ultrices."));
        }
        alia_end
    })

ALIA_DEFINE_DEMO(
    pill_tab_demo, "Pill Tabs", "",
    void do_ui(ui_context& ctx)
    {
        state_accessor<int> selected_tab = get_state(ctx, 0);
        bool checked = false;
        {
            scoped_substyle style(ctx, text("pill-tabs"));
            tab_strip strip(ctx);
            do_tab(ctx, in(checked),
                text("Home"));
            do_tab(ctx, in(checked),
                text("About"));
        }
    })

static demo_interface* tab_demos[] =
    { &tab_demo, &pill_tab_demo, 0 };

static demo_section tabs_section =
    { "Tabs", "", tab_demos };

static demo_section* section_list[] =
    { &collapsibles_section, &tabs_section, &forms_section, 0 };

demo_page containers_page =
    { "Containers", section_list};
