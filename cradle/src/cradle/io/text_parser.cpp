#include <cradle/io/text_parser.hpp>
#include <cctype>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cradle/io/file.hpp>

namespace cradle {

void initialize(text_parser& p, string const& label, char const* text,
    size_t text_size, ownership_holder const& ownership)
{
    p = text_parser();
    p.text = text;
    p.text_size = text_size;
    p.ownership = ownership;
    p.p = text;
    p.label = label;
    p.line_number = 1;
    p.line_start = text;
}

void initialize_parser_with_string(
    text_parser& p, string const& label, string const& text)
{
    initialize(p, label, text.c_str(), text.size());
}

void initialize_parser_with_file(text_parser& p, file_path const& path)
{
    p = text_parser();
    c_file f(path, "rb");
    int64_t file_length = f.length();
    alia__shared_ptr<char> text(
        new char[boost::numeric_cast<size_t>(file_length + 1)],
        array_deleter<char>());
    size_t array_length = boost::numeric_cast<size_t>(file_length);
    f.read(text.get(), boost::numeric_cast<size_t>(array_length));
    text.get()[array_length] = '\0';
    initialize(p, path.string(), text.get(), array_length,
        ownership_holder(text));
}

void initialize_parser_with_blob(
    text_parser& p, string const& label, blob const& b)
{
    initialize(p, label, reinterpret_cast<char const*>(b.data), b.size,
        b.ownership);
}

void advance(text_parser& p)
{
    if (*p.p == '\n')
    {
        ++p.line_number;
        p.line_start = p.p + 1;
    }
    ++p.p;
}

void check_char(text_parser& p, char expected)
{
    if (peek(p) != expected)
        throw_unexpected(p);
    advance(p);
}

bool is_eol(text_parser& p)
{
    char c = peek(p);
    return c == '\n' || c == '\r' || c == '\0';
}

void check_eol(text_parser& p)
{
    if (!is_eol(p))
        throw_unexpected(p);
}

bool is_line_empty(text_parser& p)
{
    skip_space(p);
    return is_eol(p);
}

void check_line_empty(text_parser& p)
{
    skip_space(p);
    check_eol(p);
}

void advance_line(text_parser& p)
{
    while (!is_eol(p))
        advance(p);
    if (peek(p) == '\r')
        advance(p);
    if (peek(p) == '\n')
        advance(p);
}

void check_eof(text_parser& p)
{
    if (!is_eof(p))
        throw_unexpected(p);
}

template<class T>
void read_unsigned_integer(text_parser& p, T& x)
{
    std::stringstream s;
    skip_space(p);
    while (std::isdigit(peek(p)))
    {
        s << peek(p);
        advance(p);
    }
    try
    {
        x = boost::lexical_cast<T>(s.str());
    }
    catch (...)
    {
        throw_error(p, "expected unsigned integer");
    }
}
template<class T>
void read_signed_integer(text_parser& p, T& x)
{
    skip_space(p);
    std::stringstream s;
    if (peek(p) == '-')
    {
        s << peek(p);
        advance(p);
    }
    while (std::isdigit(peek(p)))
    {
        s << peek(p);
        advance(p);
    }
    try
    {
        x = boost::lexical_cast<T>(s.str());
    }
    catch (...)
    {
        throw_error(p, "expected integer");
    }
}

void read(text_parser& p, int64_t& x)
{
    read_signed_integer(p, x);
}
void read(text_parser& p, uint64_t& x)
{
    read_unsigned_integer(p, x);
}
void read(text_parser& p, int32_t& x)
{
    read_signed_integer(p, x);
}
void read(text_parser& p, uint32_t& x)
{
    read_unsigned_integer(p, x);
}
void read(text_parser& p, int16_t& x)
{
    read_signed_integer(p, x);
}
void read(text_parser& p, uint16_t& x)
{
    read_unsigned_integer(p, x);
}
void read(text_parser& p, int8_t& x)
{
    int32_t i;
    read(p, i);
    x = boost::numeric_cast<int8_t>(i);
}
void read(text_parser& p, uint8_t& x)
{
    uint32_t i;
    read(p, i);
    x = boost::numeric_cast<uint8_t>(i);
}
// Apparently Visual C++ considers int and uint to be separate types, whereas
// gcc considers them redundant.
//#ifdef _MSC_VER
//    void read(text_parser& p, int& x)
//    {
//        read_signed_integer(p, x);
//    }
//    void read(text_parser& p, uint& x)
//    {
//        read_unsigned_integer(p, x);
//    }
//#endif
void read(text_parser& p, double& x)
{
    skip_space(p);
    if (is_eol(p))
        throw_error(p, "expected number");
    char* end;
    double d = strtod(p.p, &end);
    if (end == p.p)
        throw_error(p, "expected number");
    p.p = end;
    x = d;
}
void read(text_parser& p, float& x)
{
    double d;
    read(p, d);
    x = float(d);
}

void read_string(text_parser& p, string& x)
{
    std::stringstream s;
    skip_space(p);
    while (!is_eol(p) && peek(p) != ' ' && peek(p) != '\t')
    {
        s << peek(p);
        advance(p);
    }
    x = s.str();
}

void read_quoted_string(text_parser& p, string& x)
{
    std::stringstream s;
    skip_space(p);
    check_char(p, '"');
    while (peek(p) != '"')
    {
        if (peek(p) == '\0')
            throw_unexpected(p);
        s << peek(p);
        advance(p);
    }
    x = s.str();
}

void read_rest_of_line(text_parser& p, string& x)
{
    std::stringstream s;
    while (!is_eol(p))
    {
        s << peek(p);
        advance(p);
    }
    x = s.str();
}

void throw_error(text_parser& p, string const& message)
{
    throw parse_error(p.label, p.line_number, int(p.p - p.line_start) + 1,
        message);
}

void throw_unexpected(text_parser& p)
{
    if (peek(p) == '\0')
        throw_error(p, "unexpected end-of-string\n");
    throw_error(p, str(boost::format("unexpected character: %c (0x%02x).") %
        peek(p) % int(peek(p))));
}

void skip_space(text_parser& p)
{
    while (peek(p) == ' ' || peek(p) == '\t')
        advance(p);
}

}
