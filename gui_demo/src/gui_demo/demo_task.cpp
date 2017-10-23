#include <gui_demo/demo_task.hpp>
#include <gui_demo/demo_task_state.hpp>

#include <cradle/gui/app/interface.hpp>
#include <cradle/gui/task_controls.hpp>
#include <cradle/gui/task_interface.hpp>

#include <gui_demo/utilities.hpp>

namespace gui_demo {

extern demo_page tutorial_page;
extern demo_page widgets_page;
extern demo_page layout_page;
extern demo_page containers_page;
extern demo_page timing_page;

// CONTROLS - This is used to navigate between demos.

struct demo_block : control_block_interface
{
    demo_block(demo_page& page) : page_(page) {}

    void do_active_ui(gui_context& ctx)
    {
        row_layout row(ctx);
        do_spacer(ctx, width(1, EM));
        {
            column_layout column(ctx);
            for (int i = 0; page_.sections[i]; ++i)
            {
                demo_section& section = *page_.sections[i];
                if (do_link(ctx, text(section.label)))
                    jump_to_location(ctx, make_id(&section));
            }
        }
    }

    demo_page& page_;
};

void do_page_nav_links(
    gui_context& ctx, accessor<bool> const& is_active, demo_page& page)
{
    do_control_block(ctx, is_active, text(page.label), demo_block(page));
}

void do_navigator(
    gui_context& ctx, app_context& app_ctx, string const& task_id,
    accessor<demo_task_state> const& state)
{
    do_page_nav_links(ctx,
        make_radio_accessor_for_optional(
            _field(state, selected_demo),
            in(demo_id::TUTORIAL)),
        tutorial_page);
    do_page_nav_links(ctx,
        make_radio_accessor_for_optional(
            _field(state, selected_demo),
            in(demo_id::WIDGETS)),
        widgets_page);
    do_page_nav_links(ctx,
        make_radio_accessor_for_optional(
            _field(state, selected_demo),
            in(demo_id::LAYOUT)),
        layout_page);
    do_page_nav_links(ctx,
        make_radio_accessor_for_optional(
            _field(state, selected_demo),
            in(demo_id::CONTAINERS)),
        containers_page);
    do_page_nav_links(ctx,
        make_radio_accessor_for_optional(
            _field(state, selected_demo),
            in(demo_id::TIMING)),
        timing_page);
}

// DISPLAY - This shows the actual demo content.

void do_demo_ui(gui_context& ctx, demo_interface& demo)
{
    alia_cached_ui_block(no_id, default_layout)
    {
        do_heading(ctx, text("h3"), text(demo.get_label()));
        do_paragraph(ctx, text(demo.get_description()));

        auto selected_tab = get_state(ctx, 0);
        {
            panel p(ctx, text("demo-ui"));
            demo.do_ui(ctx);
        }
        {
            do_source_code(ctx, demo.get_code());
        }

        if (do_link(ctx, text("copy the code")))
            ctx.system->os->set_clipboard_text(format_code(demo.get_code()));
    }
    alia_end
}

void do_section_contents(gui_context& ctx, demo_section& section)
{
    alia_cached_ui_block(no_id, default_layout)
    {
        mark_location(ctx, make_id(&section));
        do_heading(ctx, text("h2"), text(section.label));
        do_paragraph(ctx, text(section.description));
        for (int i = 0; section.demos[i]; ++i)
            do_demo_ui(ctx, *section.demos[i]);
    }
    alia_end
}

void do_page_contents(gui_context& ctx, demo_page& page)
{
    scrollable_panel background(ctx, text("content"), GROW);

    scoped_substyle style(ctx, text("demo"));

    mark_location(ctx, make_id(&page));
    do_heading(ctx, text("h1"), text(page.label));
    for (int i = 0; page.sections[i]; ++i)
    {
        do_separator(ctx);
        do_section_contents(ctx, *page.sections[i]);
    }
}

void do_demo_page(
    gui_context& ctx, app_context& app_ctx, string const& task_id,
    accessor<demo_task_state> const& state)
{
    alia_if (has_value(_field(state, selected_demo)))
    {
        alia_switch (get(get(state).selected_demo))
        {
         alia_case(demo_id::TUTORIAL):
            do_page_contents(ctx, tutorial_page);
            break;
         alia_case(demo_id::WIDGETS):
            do_page_contents(ctx, widgets_page);
            break;
         alia_case(demo_id::LAYOUT):
            do_page_contents(ctx, layout_page);
            break;
         alia_case(demo_id::CONTAINERS):
            do_page_contents(ctx, containers_page);
            break;
         alia_case(demo_id::TIMING):
            do_page_contents(ctx, timing_page);
            break;
        }
        alia_end
    }
    alia_else
    {
        do_empty_display_panel(ctx);
    }
    alia_end
}

// TASK DEFINITION

CRADLE_DEFINE_SIMPLE_UI_TASK(demo_task, app_context, demo_task_state)

void
demo_task::do_title(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<demo_task_state> const& task)
{
    do_task_title(ctx, text("APP"), text("GUI Demo"));
}

void demo_task::do_control_ui(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<demo_task_state> const& task)
{
    do_navigator(ctx, app_ctx, task.id, task.state);
}

void demo_task::do_display_ui(
    gui_context& ctx, app_context& app_ctx,
    gui_task_context<demo_task_state> const& task)
{
    do_demo_page(ctx, app_ctx, task.id, task.state);
}

void register_demo_task()
{
    register_app_task("demo_task", new demo_task);
}

}
