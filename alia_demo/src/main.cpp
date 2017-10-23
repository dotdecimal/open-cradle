//#define USE_WIN32_BACKEND
#define USE_WX_BACKEND

#ifdef USE_WIN32_BACKEND

#include <alia/ui/backends/win32.hpp>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else

#ifdef USE_WX_BACKEND

#include <alia/ui/backends/wx.hpp>
#include <wx/glcanvas.h>
#include <wx/msgdlg.h>
#include <cradle/external/clean.hpp>

#else

#include <alia/ui/backends/qt.hpp>
#include <QApplication.h>

#endif

#endif

#include <alia/alia.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/system.hpp>
#include "utilities.hpp"

using namespace alia;

///

struct popup_menu_data
{
    popup_positioning positioning;
};

///

void do_footer(ui_context& ctx)
{
    panel header(ctx, text("footer"), default_layout, PANEL_HORIZONTAL);

    do_heading(ctx, text("title"), text(""));

    do_spacer(ctx, layout(height(4, EM), GROW));

    auto show_fps = get_state(ctx, false);
    alia_if (get(show_fps))
    {
        column_layout column(ctx);
        indirect_accessor<int> fps = compute_fps(ctx);
        alia_if (is_gettable(fps))
        {
            do_text(ctx, printf(ctx, "%i FPS", fps));
        }
        alia_else
        {
            do_text(ctx, text("measuring"));
        }
        alia_end
        if (do_link(ctx, text("stop"), RIGHT))
            set(show_fps, false);
    }
    alia_else
    {
        if (do_link(ctx, text("benchmark")))
            set(show_fps, true);
    }
    alia_end
}

void do_demo_ui(ui_context& ctx, demo_interface& demo)
{
    alia_cached_ui_block(no_id, default_layout)
    {
        do_heading(ctx, text("h3"), text(demo.get_label()));
        do_paragraph(ctx, text(demo.get_description()));

        auto selected_tab = get_state(ctx, 0);
        //alia_untracked_if (!get(selected_tab))
        //{
        //}
        //alia_end
        //{
        //    tab_strip strip(ctx);
        //    do_tab(ctx, make_indexed_accessor(selected_tab, 0), text("demo"));
        //    do_tab(ctx, make_indexed_accessor(selected_tab, 1), text("code"));
        //}
        //alia_if (!get(selected_tab))
        {
            panel p(ctx, text("demo"));
            demo.do_ui(ctx);
        }
        //alia_else
        {
            do_source_code(ctx, demo.get_code());
        }
        //alia_end

        if (do_link(ctx, text("copy the code")))
            ctx.system->os->set_clipboard_text(format_code(demo.get_code()));
    }
    alia_end
}

void do_section_contents(ui_context& ctx, demo_section& section)
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

void do_page_contents(ui_context& ctx, demo_page& page)
{
    {
        mark_location(ctx, make_id(&page));
        do_heading(ctx, text("h1"), text(page.label));
        for (int i = 0; page.sections[i]; ++i)
        {
            do_separator(ctx);
            do_section_contents(ctx, *page.sections[i]);
        }
    }
}

void do_page_nav_links(
    ui_context& ctx, accessor<demo_page*> const& active_page, demo_page& page)
{
    accordion_section block(ctx,
        make_radio_accessor(active_page, in(&page)));
    if (block.clicked())
    {
        jump_to_location(ctx, make_id(&page), JUMP_TO_LOCATION_ABRUPTLY);
        end_pass(ctx);
    }
    do_text(ctx, text(page.label));
    alia_if (block.do_content())
    {
        for (int i = 0; page.sections[i]; ++i)
        {
            demo_section& section = *page.sections[i];
            if (do_link(ctx, text(section.label)))
                jump_to_location(ctx, make_id(&section));
        }
    }
    alia_end
}

extern demo_page tutorial_page;
extern demo_page widgets_page;
extern demo_page layout_page;
extern demo_page containers_page;
extern demo_page timing_page;

void do_navigator(
    ui_context& ctx, accessor<demo_page*> const& selected_page)
{
    panel background(ctx, text("background"));

    panel nav(ctx, text("nav"), layout(width(16, EM), TOP));

    do_heading(ctx, text("title"), text("Contents"), CENTER);

    do_page_nav_links(ctx, selected_page, tutorial_page);
    do_page_nav_links(ctx, selected_page, widgets_page);
    do_page_nav_links(ctx, selected_page, layout_page);
    do_page_nav_links(ctx, selected_page, containers_page);
    do_page_nav_links(ctx, selected_page, timing_page);
}

void do_main_ui(ui_context& ctx)
{
    column_layout top(ctx, GROW);

    {
        clamped_content background(ctx, text("background"), text("background"),
            width(70, EM), GROW);

        {
            row_layout content_row(ctx, GROW);

            state_accessor<demo_page*> selected_page =
                get_state(ctx, &tutorial_page);

            {
                clip_evasion_layout clip_evader(ctx, TOP);
                do_navigator(ctx, selected_page);
            }

            {
                panel content(ctx, text("content"), GROW);

                #if 1

                naming_context nc(ctx);
                {
                    named_block block(nc, make_id(get(selected_page)),
                        manual_delete(true));
                    do_page_contents(ctx, *get(selected_page));
                }

                #else

                do_page_contents(ctx, tutorial_page);
                do_spacer(ctx, height(3, EM));
                do_page_contents(ctx, widgets_page);
                do_spacer(ctx, height(3, EM));
                do_page_contents(ctx, layout_page);
                do_spacer(ctx, height(3, EM));
                do_page_contents(ctx, containers_page);
                do_spacer(ctx, height(3, EM));
                do_page_contents(ctx, timing_page);

                #endif
            }
        }
    }

    do_footer(ctx);
}

struct controller : alia::app_window_controller
{
    void do_ui(ui_context& ctx)
    {
        state_accessor<bool> light_on_dark = get_state(ctx, false);
        if (detect_key_press(ctx, KEY_F9))
        {
            set(light_on_dark, !get(light_on_dark));
            end_pass(ctx);
        }
        scoped_substyle theme_style;
        alia_if (get(light_on_dark))
        {
            theme_style.begin(ctx, text("light-on-dark"));
        }
        alia_end

        do_main_ui(ctx);

        // ctrl-plus and ctrl-minus adjust the font size. ctrl-0 resets it.
        if (detect_key_press(ctx, KEY_PLUS, KMOD_CTRL) ||
            detect_key_press(ctx, KEY_EQUALS, KMOD_CTRL))
        {
            set_magnification_factor(*ctx.system,
                get_magnification_factor(*ctx.system) * 1.1f);
        }
        if (detect_key_press(ctx, KEY_MINUS, KMOD_CTRL))
        {
            set_magnification_factor(*ctx.system,
                get_magnification_factor(*ctx.system) / 1.1f);
        }
        if (detect_key_press(ctx, key_code('0'), KMOD_CTRL))
        {
            set_magnification_factor(*ctx.system, 1);
        }
        // F11 toggles full screen mode.
        if (detect_key_press(ctx, KEY_F11))
        {
            this->window->set_full_screen(!this->window->is_full_screen());
            end_pass(ctx);
        }
    }
};

#ifdef USE_WIN32_BACKEND

int WINAPI WinMain(
    HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show)
{
    try
    {
        //auto style = parse_style_file("alia.style");
        //write_style_cpp_file("alia_style.cpp", "alia_style", style);

        native_window wnd("alia demo",
            alia__shared_ptr<app_window_controller>(new controller),
            app_window_state(none, make_vector<int>(850, 1000)));
        wnd.do_message_loop();
    }
    catch (std::exception& e)
    {
        MessageBox(0, e.what(), "alia error", MB_OK | MB_ICONEXCLAMATION);
    }
    return 0;
}

#else

#ifdef USE_WX_BACKEND

struct application : public wxGLApp
{
    application();
    bool OnInit();
    int OnRun();
    int OnExit();
 private:
    int return_code_;
};

application::application()
{
    int attribs[] = { WX_GL_DOUBLEBUFFER, 0 };
    if (!this->InitGLVisual(attribs))
    {
        wxMessageBox("OpenGL not available");
        return_code_ = -1;
    }
    else
        return_code_ = 0;
}

bool application::OnInit()
{
    try
    {
        style_tree_ptr style = parse_style_file("alia.style");

        alia__shared_ptr<app_window_controller> controller_ptr(new controller);

        int gl_canvas_attribs[] = {
            WX_GL_RGBA,
            WX_GL_DOUBLEBUFFER,
            WX_GL_STENCIL_SIZE, 1,
            WX_GL_SAMPLE_BUFFERS, 1,
            WX_GL_SAMPLES, 4,
            0 };
        auto* frame =
            create_wx_framed_window(
                "Planning App",
                controller_ptr, style,
                app_window_state(none, make_vector<int>(850, 1000)),
                gl_canvas_attribs);
    }
    catch (std::exception& e)
    {
        wxMessageBox(std::string("An error occurred during application"
            " initialization.\n\n") + e.what());
        return_code_ = -1;
    }
    catch (...)
    {
        wxMessageBox("An unknown error occurred during application"
            " initialization.");
        return_code_ = -1;
    }
    return true;
}

int application::OnRun()
{
    if (return_code_ == 0)
        return wxGLApp::OnRun();
    else
        return return_code_;
}

int application::OnExit()
{
    return wxGLApp::OnExit();
}

IMPLEMENT_APP(application)

#else

int WINAPI WinMain(
    HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show)
{
    QApplication app(__argc, __argv);
    qt_window wnd("alia demo", new controller,
        qt_window::state_data(make_vector<int>(0, 0), make_vector<int>(850, 1000)));
    return app.exec();
}

#endif

#endif
