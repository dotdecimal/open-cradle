#ifndef CRADLE_GUI_TYPES_HPP
#define CRADLE_GUI_TYPES_HPP

#include <cradle/common.hpp>

// This file defines some data types that are GUI-related but are still useful
// outside GUI code (e.g., as the result of functions) and some functions for
// working with them. This file is included even for command-line only builds.

namespace cradle {

// STYLED TEXT - styled_text represents a text with internal styling.
// It's represented as a list of styled_text_fragments, each with an optional
// style name and a string of text.
// Neighboring fragments are NOT implicitly separated by whitespace, so this
// must be explicitly done where needed.

api(struct)
struct styled_text_fragment
{
    optional<string> style;
    string text;
};

styled_text_fragment static inline
make_styled_text_fragment(string const& style, string const& text)
{
    styled_text_fragment st;
    st.style = some(style);
    st.text = text;
    return st;
};

styled_text_fragment static inline
make_unstyled_text_fragment(string const& text)
{
    styled_text_fragment st;
    st.style = none;
    st.text = text;
    return st;
};

typedef std::vector<styled_text_fragment> styled_text;

styled_text static inline
make_styled_text(styled_text_fragment const& fragment)
{
    styled_text p;
    p.push_back(fragment);
    return p;
};

styled_text static inline
make_simple_styled_text(string const& style, string const& text)
{
    return make_styled_text(make_styled_text_fragment(style, text));
};

styled_text static inline
make_unstyled_text(string const& text)
{
    return make_styled_text(make_unstyled_text_fragment(text));
};

// Append 'fragment' to 'text', merging it into the last fragment in 'text' if
// possible.
void
append_styled_text(styled_text& text, styled_text_fragment const& fragment);

// Append the full contents of b to a.
void append_styled_text(styled_text& a, styled_text const& b);

// Concatenate two styled texts.
styled_text concatenate_styled_text(styled_text const& a, styled_text const& b);

// Append 'fragment' as unstyled text to the end 'text'.
void append_unstyled_text(styled_text& text, string const& fragment);

// Append an unstyled space to the given styled_text.
void append_space(styled_text& text);

// Flatten the given styled text to a single string.
string flatten(styled_text const& text);

// +/+= operators for appending a styled_text_fragment onto a styled_text
static inline styled_text&
operator+=(styled_text& text, styled_text_fragment const& fragment)
{
    append_styled_text(text, fragment);
    return text;
}
static inline styled_text
operator+(styled_text const& text, styled_text_fragment const& fragment)
{
    styled_text concatenation = text;
    concatenation += fragment;
    return concatenation;
}

// +/+= operators for concatentating styled_texts
static inline styled_text&
operator+=(styled_text& a, styled_text const& b)
{
    append_styled_text(a, b);
    return a;
}
static inline styled_text
operator+(styled_text const& a, styled_text const& b)
{
    return concatenate_styled_text(a, b);
}

// + operator for concatenating styled_text_fragments
static inline styled_text
operator+(styled_text_fragment const& a, styled_text_fragment const& b)
{
    styled_text concatenation;
    concatenation += a;
    concatenation += b;
    return concatenation;
}

// +/+= operators for appending an unstyled string onto a styled_text
static inline styled_text&
operator+=(styled_text& text, string const& unstyled_fragment)
{
    append_unstyled_text(text, unstyled_fragment);
    return text;
}
static inline styled_text
operator+(styled_text const& text, string const& unstyled_fragment)
{
    styled_text concatenation = text;
    concatenation += unstyled_fragment;
    return concatenation;
}

// +/+= operators for appending additional text onto a styled_text_fragment
static inline styled_text_fragment&
operator+=(styled_text_fragment& fragment, string const& text)
{
    fragment.text += text;
    return fragment;
}
static inline styled_text_fragment
operator+(styled_text_fragment const& fragment, string const& text)
{
    styled_text_fragment concatenation = fragment;
    concatenation += text;
    return concatenation;
}

// MARKUP - markup_document is a rough attempt at a structure for representing
// arbitrary textual reports.

struct markup_form_row;

api(union)
union markup_block
{
    nil_type empty;
    styled_text text;
    std::vector<markup_block> column;
    std::vector<markup_block> bulleted_list;
    std::vector<markup_form_row> form;
};

api(struct)
struct markup_form_row
{
    string label;
    markup_block value;
};

typedef std::vector<markup_form_row> markup_form;

api(struct)
struct markup_document
{
    markup_block content;
};

// TODO: seems to not be working - Daniel
//static void inline
//ensure_default_initialization(markup_block& x)
//{ x = make_markup_block_with_empty(nil); }

// +/+= operators for concatenating markup_blocks
static inline std::vector<markup_block>&
operator+=(std::vector<markup_block>& list, markup_block const& block)
{
    list.push_back(block);
    return list;
}
static inline std::vector<markup_block>
operator+(std::vector<markup_block> const& list, markup_block const& block)
{
    std::vector<markup_block> concatenation = list;
    concatenation += block;
    return concatenation;
}
static inline std::vector<markup_block>
operator+(markup_block const& a, markup_block const& b)
{
    std::vector<markup_block> concatenation;
    concatenation += a;
    concatenation += b;
    return concatenation;
}

}

#endif
