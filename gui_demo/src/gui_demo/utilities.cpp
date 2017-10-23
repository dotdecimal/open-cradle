#include <gui_demo/utilities.hpp>
#include <cctype>
#include <sstream>

namespace gui_demo {

static void
append_code(std::ostringstream& s, int brace_depth,
    utf8_ptr start, utf8_ptr end)
{
    for (int i = 0; i != brace_depth; ++i)
        s << "    ";
    for (utf8_ptr p = start; p != end; ++p)
        s << *p;
    s << '\n';
}

string format_code(char const* code)
{
    std::ostringstream s;
    char const* p = code;
    int brace_depth = 0, paren_depth = 0;
    while (*p != '\0')
    {
        char const* q = p;
        while (1)
        {
            switch (*q)
            {
             case '\0':
                append_code(s, brace_depth, p, q);
                goto line_end;
             case ';':
                if (paren_depth == 0)
                {
                    if (q - 1 > code && *(q - 1) == ' ')
                    {
                        append_code(s, brace_depth, p, q);
                        append_code(s, brace_depth + 1, q, q + 1);
                    }
                    else
                        append_code(s, brace_depth, p, q + 1);
                    ++q;
                    goto line_end;
                }
                break;
             case '(':
                ++paren_depth;
                break;
             case ')':
                --paren_depth;
                break;
             case '{':
                if (p != q)
                    append_code(s, brace_depth, p, q);
                append_code(s, brace_depth, q, q + 1);
                ++q;
                ++brace_depth;
                goto line_end;
             case '}':
                if (p != q)
                    append_code(s, brace_depth, p, q);
                --brace_depth;
                append_code(s, brace_depth, q, q + 1);
                ++q;
                goto line_end;
            }
            ++q;
        }
      line_end:
        p = q;
        while (std::isspace(*p))
            ++p;
    }
    return s.str();
}

static void
do_code_line(gui_context& ctx, table& table,
    int line_number, char const* start, char const* end)
{
    if (start == end)
        return;
    table_row row(table);
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%d.", in(line_number)), RIGHT);
    }
    {
        table_cell cell(row, GROW);
        row_layout row(ctx);
        do_paragraph(ctx, make_text(utf8_string(start, end), make_id(start)),
            UNPADDED | GROW);
    }
}

static void do_formatted_code(gui_context& ctx, string const& code)
{
    table table(ctx, text("table"));
    int line_number = 1;
    char const* p = code.c_str();
    while (*p != '\0')
    {
        char const* q = p;
        while (*q != '\n')
            ++q;                
        do_code_line(ctx, table, line_number, p, q);
        p = q + 1;
        ++line_number;
    }
}

void do_source_code(gui_context& ctx, char const* code)
{
    string* formatted;
    if (get_cached_data(ctx, &formatted))
        *formatted = format_code(code);

    do_formatted_code(ctx, *formatted);
}

}
