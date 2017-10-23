#include <cradle/gui/types.hpp>

namespace cradle {

void
append_styled_text(styled_text& text, styled_text_fragment const& fragment)
{
    // If the last element is a matching style, just append this to it.
    if (!text.empty() && text.back().style == fragment.style)
        text.back().text += fragment.text;
    else
        text.push_back(fragment);
}

void append_styled_text(styled_text& a, styled_text const& b)
{
    for (auto const& fragment : b)
        append_styled_text(a, fragment);
}

styled_text concatenate_styled_text(styled_text const& a, styled_text const& b)
{
    auto r = a;
    append_styled_text(r, b);
    return r;
}

void append_unstyled_text(styled_text& text, string const& fragment)
{
    append_styled_text(text, make_unstyled_text_fragment(fragment));
}

void append_space(styled_text& text)
{
    append_unstyled_text(text, " ");
}

string flatten(styled_text const& text)
{
    string flattened;
    for (auto const& fragment : text)
        flattened += fragment.text;
    return flattened;
}

}
