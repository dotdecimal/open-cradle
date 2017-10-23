#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

using namespace alia;

struct demo_interface
{
    virtual char const* get_label() const = 0;
    virtual char const* get_description() const = 0;
    virtual char const* get_code() const = 0;
    virtual void do_ui(ui_context& ctx) = 0;
};

string format_code(char const* code);

void do_source_code(ui_context& ctx, char const* code);

#define ALIA_DEFINE_DEMO(id, label, description, code) \
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
