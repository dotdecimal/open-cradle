#ifndef CRADLE_IO_TEXT_PARSER_HPP
#define CRADLE_IO_TEXT_PARSER_HPP

#include <cradle/common.hpp>
#include <cradle/io/forward.hpp>

namespace cradle {

// for parsing simply formatted text
struct text_parser
{
    // original text
    char const* text;
    size_t text_size;
    ownership_holder ownership;
    // current position within the text
    char const* p;
    // label identifying this text (e.g., file name)
    string label;
    int line_number;
    char const* line_start;
};

void initialize(text_parser& p, string const& label, char const* text,
    size_t text_size, ownership_holder const& ownership = ownership_holder());

// The string must remain allocated as long as the parser is in use.
void initialize_parser_with_string(
    text_parser& p, string const& label, string const& text);

void initialize_parser_with_file(text_parser& p, file_path const& path);

void initialize_parser_with_blob(
    text_parser& p, string const& label, blob const& b);

// Peek at the next character.
static inline char peek(text_parser& p)
{
    if (p.p >= p.text + p.text_size)
    {
        return '\0';
    }
    return *p.p;
}

// Advance past the next character.
void advance(text_parser& p);

// Check that the next character is as expected and skip it.
// If it's not the expected character, throw an error.
void check_char(text_parser& p, char expected);

// Is the parser at the end of the current line?
bool is_eol(text_parser& p);

// Check that the parser is at the end of the line.
// If it's not, throw an error.
void check_eol(text_parser& p);

// Is the rest of the line empty?
bool is_line_empty(text_parser& p);

// Check that the rest of the line is empty.
// If it's not, throw an error.
void check_line_empty(text_parser& p);

// Advance to the next line of text.
void advance_line(text_parser& p);

// Is the parser at the end of the text?
static inline bool is_eof(text_parser& p) { return peek(p) == '\0'; }

// Check that the parser is at the end of the text.
// If it's not, throw an error.
void check_eof(text_parser& p);

// Skip over spaces and tabs.
void skip_space(text_parser& p);

// Note that the functions for reading numbers and strings will skip over
// spaces and tabs before attempting the read.

void read(text_parser& p, int64_t& x);
void read(text_parser& p, uint64_t& x);
void read(text_parser& p, int32_t& x);
void read(text_parser& p, uint32_t& x);
void read(text_parser& p, int16_t& x);
void read(text_parser& p, uint16_t& x);
void read(text_parser& p, int8_t& x);
void read(text_parser& p, uint8_t& x);
// Apparently Visual C++ considers int and unsigned to be distinct types from
// the above, whereas gcc considers them redundant.
#ifdef _MSC_VER
    void read(text_parser& p, int& x);
    void read(text_parser& p, unsigned& x);
#endif
void read(text_parser& p, double& x);
void read(text_parser& p, float& x);

static inline int read_int(text_parser& p)
{ int x; read(p, x); return x; }

static inline double
read_double(text_parser& p)
{
    double x;
    read(p, x);
    return x;
}

void read_string(text_parser& p, string& x);
static inline string read_string(text_parser& p)
{ string x; read_string(p, x); return x; }

void read_quoted_string(text_parser& p, string& x);
static inline string read_quoted_string(text_parser& p)
{ string x; read_quoted_string(p, x); return x; }

void read_rest_of_line(text_parser& p, string& x);
static inline string read_rest_of_line(text_parser& p)
{ string x; read_rest_of_line(p, x); return x; }

// Throw an exception with the given error message plus some context info.
void throw_error(text_parser& p, string const& message);

// Throw an unexpected character error.
void throw_unexpected(text_parser& p);

struct parse_error : exception
{
    parse_error(string const& label, int line_number, int column_number,
        string const& message)
      : cradle::exception("parse error: " + label + ":" +
            to_string(line_number) + ":" + to_string(column_number) + ": " +
            message)
      , label_(new string(label))
      , line_number_(line_number)
      , column_number_(column_number)
      , message_(new string(message))
    {}
    ~parse_error() throw() {}
    string const& label() const { return *label_; }
    int line_number() const { return line_number_; }
    int column_number() const { return column_number_; }
    string const& message() const { return *message_; }
 private:
    alia__shared_ptr<string> label_;
    int line_number_, column_number_;
    alia__shared_ptr<string> message_;
};

}

#endif
