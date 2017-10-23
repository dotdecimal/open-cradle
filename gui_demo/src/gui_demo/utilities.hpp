#ifndef GUI_DEMO_UTILITIES_HPP
#define GUI_DEMO_UTILITIES_HPP

#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

#include <cradle/gui/common.hpp>

#include <gui_demo/common.hpp>

namespace gui_demo {

struct demo_interface
{
    virtual char const* get_label() const = 0;
    virtual char const* get_description() const = 0;
    virtual char const* get_code() const = 0;
    virtual void do_ui(gui_context& ctx) = 0;
};

string format_code(char const* code);

void do_source_code(gui_context& ctx, char const* code);

#define DEFINE_DEMO(id, label, description, code) \
    struct id##_type : demo_interface \
    { \
        char const* get_label() const \
        { \
            return label; \
        } \
        char const* get_description() const \
        { \
            return description; \
        } \
        char const* get_code() const \
        { \
            return #code; \
        } \
        code \
    }; \
    static id##_type id;

struct demo_section
{
    char const* label;
    char const* description;
    demo_interface** demos; // NULL-terminated array of pointers
};

struct demo_page
{
    char const* label;
    demo_section** sections; // NULL-terminated array of pointers
};

}

#endif
