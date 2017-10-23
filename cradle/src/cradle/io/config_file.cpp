#include <cradle/io/config_file.hpp>
#include <cradle/io/file.hpp>
#include <cctype>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace cradle { namespace impl { namespace config {

// VALUES AND MATCHING

static void handle_type_mismatch(value const& v, string expected_type)
{
    throw type_mismatch(v.file, v.location, expected_type, v.type_name);
}

template<class OriginalT, class ConvertedT>
static void match_and_convert_value(ConvertedT* dst, value const& v,
    string const& converted_type_name)
{
    OriginalT ov;
    match_value(&ov, v);
    ConvertedT cv = ConvertedT(ov);
    if (OriginalT(cv) != ov)
        handle_type_mismatch(v, converted_type_name);
    *dst = cv;
}

void match_value(int* dst, value const& v)
{
    match_and_convert_value<double>(dst, v, "integer");
}
void match_value(unsigned* dst, value const& v)
{
    match_and_convert_value<double>(dst, v, "unsigned integer");
}
void match_value(float* dst, value const& v)
{
    double d;
    match_value(&d, v);
    *dst = float(d);
}
void match_value(double* dst, value const& v)
{
    double_value const* dv = dynamic_cast<double_value const*>(&v);
    if (!dv)
        handle_type_mismatch(v, "real number");
    *dst = dv->number;
}
void match_value(bool* dst, value const& v)
{
    *dst = match_option(v, "false true") != 0;
}
void match_value(vector2i* dst, value const& v)
{
    match_and_convert_value<vector2d>(
        dst, v, "pair of integers");
}
void match_value(vector2u* dst, value const& v)
{
    match_and_convert_value<vector2d>(
        dst, v, "pair of unsigned integers");
}
void match_value(vector2f* dst, value const& v)
{
    vector2d v2d;
    match_value(&v2d, v);
    *dst = vector2f(v2d);
}
void match_value(vector2d* dst, value const& v)
{
    vector2d_value const* v2d = dynamic_cast<vector2d_value const*>(&v);
    if (!v2d)
        handle_type_mismatch(v, "pair of real numbers");
    *dst = v2d->vector;
}
void match_value(vector3i* dst, value const& v)
{
    match_and_convert_value<vector3d>(
        dst, v, "triple of integers");
}
void match_value(vector3u* dst, value const& v)
{
    match_and_convert_value<vector3d>(
        dst, v, "triple of unsigned integers");
}
void match_value(vector3f* dst, value const& v)
{
    vector3d v3d;
    match_value(&v3d, v);
    *dst = vector3f(v3d);
}
void match_value(vector3d* dst, value const& v)
{
    vector3d_value const* v3d = dynamic_cast<vector3d_value const*>(&v);
    if (!v3d)
        handle_type_mismatch(v, "triple of real numbers");
    *dst = v3d->vector;
}
void match_value(string* dst, value const& v)
{
    string_value const* sv = dynamic_cast<string_value const*>(&v);
    if (!sv)
        handle_type_mismatch(v, "string");
    *dst = sv->s;
}
void match_value(structure* dst, value const& v)
{
    structure_value const* sv = dynamic_cast<structure_value const*>(&v);
    if (!sv)
        handle_type_mismatch(v, "structure");
    *dst = structure(sv->data);
}
void match_value(untyped_list* dst, value const& v)
{
    list_value const* lv = dynamic_cast<list_value const*>(&v);
    if (!lv)
        handle_type_mismatch(v, "list");
    *dst = untyped_list(lv->data.get());
}

int match_option(value const& v, string const& options)
{
    option_value const* opt_v = dynamic_cast<option_value const*>(&v);
    if (!opt_v)
        handle_type_mismatch(v, "option");
    boost::char_separator<char> separator(" ");
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    tokenizer tokens(options, separator);
    int n = 0;
    string lowercase_option =
        boost::algorithm::to_lower_copy(opt_v->option);
    for (tokenizer::iterator i = tokens.begin(); i != tokens.end(); ++i, ++n)
    {
        if (*i == lowercase_option)
            return n;
    }
    throw bad_option(v.file, v.location, opt_v->option);
}

// STRUCTURE, LIST

value const& structure::get_value(string const& name) const
{
    std::map<string,value_ptr>::const_iterator i =
        data_->contents.find(name);
    if (i == data_->contents.end())
        throw missing_variable(data_->file, data_->location, name);
    else
        return *i->second;
}
value const* structure::get_optional_value(string const& name) const
{
    std::map<string,value_ptr>::const_iterator i =
        data_->contents.find(name);
    if (i == data_->contents.end())
        return 0;
    else
        return i->second.get();
}

bool untyped_list::empty() const
{
    return index_ == data_->contents.size();
}
size_t untyped_list::size() const
{
    return data_->contents.size() - index_;
}
value const& untyped_list::get_value()
{
    assert(!empty());
    return *data_->contents[index_++];
}

// PARSING

// Rather than operating directly on the file, the parser first preprocesses
// the file (and any included files) into a text buffer, then operates on that.

class parser : noncopyable
{
 public:
    parser(file_path const& file);

    // Skip over whitespace to find the next character.
    void skip_whitespace();

    // Get the next character in the file.  If no character is available, this
    // throws an exception.
    int get();

    // Peek at the next character in the file, but don't consume it.
    int peek() const;

    // This matches the next character in the file against the given
    // expected character.  If there is a match, the character is
    // skipped over and the function returns normally.  Otherwise, the
    // character remains and an exception is thrown.
    void match(int c);

    // Is the file at EOF?
    bool eof() const
    { return line_index_ == static_cast<int>(text_buffer_.size()); }

    // Get the name of the file we're current parsing.
    file_path const& get_file() const
    { return text_buffer_[clamped_line_index()].file; }

    // Get the current line number.
    int get_line_number() const
    { return text_buffer_[clamped_line_index()].line_number; }

    // Get the current column number.
    int get_column_number() const { return column_index_ + 1; }

    // Are we currently inside a string?
    // This is special because '#'s inside a string do NOT indicate the start
    // of a comment.
    bool in_string() const { return in_string_; }
    void in_string(bool in_string) { in_string_ = in_string; }

    void throw_syntax_error();

 private:
    // Load the contents of the given file into the text buffer.
    void read_file(file_path const& file);

    int clamped_line_index() const
    { return (std::min)(line_index_, int(text_buffer_.size()) - 1); }

    // the text buffer - Not the most efficient way to do this.
    struct text_buffer_line
    {
        file_path file;
        int line_number;
        string text;
    };
    std::vector<text_buffer_line> text_buffer_;

    int line_index_, column_index_;

    bool in_string_;
};

parser::parser(file_path const& file)
{
    read_file(file);

    // We need at least one line in the text buffer.
    if (text_buffer_.empty())
    {
        text_buffer_.push_back(text_buffer_line());
        text_buffer_line& tbl = text_buffer_.back();
        tbl.file = file;
        tbl.line_number = 1;
        tbl.text = "";
    }

    line_index_ = column_index_ = 0;

    in_string(false);
}

void parser::read_file(file_path const& file)
{
    std::ifstream f;
    open(f, file, std::ios::in);
    f.exceptions(std::ios::badbit);

    int line_number = 1;
    string line;
    while (getline(f, line))
    {
        if (!line.empty() && line[0] == '!')
        {
            boost::smatch what;
            if (boost::regex_match(line, what,
                boost::regex("^!include \"([^\"]+)\"$")))
            {
                read_file(file_path(string(what[1])));
            }
            else
                throw syntax_error(file, line_number, 1);
        }
        else
        {
            text_buffer_.push_back(text_buffer_line());
            text_buffer_line& tbl = text_buffer_.back();
            tbl.file = file;
            tbl.line_number = line_number;
            tbl.text = line + "\n";
        }

        ++line_number;
    }
}

void parser::throw_syntax_error()
{
    throw syntax_error(get_file(), get_line_number(), get_column_number());
}

void parser::skip_whitespace()
{
    while (std::isspace(peek()))
        get();
}

int parser::get()
{
    if (line_index_ >= static_cast<int>(text_buffer_.size()))
        return -1;

    int c = text_buffer_[line_index_].text[column_index_];
    ++column_index_;
    if (column_index_ >=
        static_cast<int>(text_buffer_[line_index_].text.length()))
    {
        ++line_index_;
        column_index_ = 0;
    }

    if (c == '#' && !in_string())
    {
        ++line_index_;
        column_index_ = 0;
        c = '\n';
    }

    if (c == '\n' && in_string())
        throw_syntax_error();

    return c;
}

int parser::peek() const
{
    if (line_index_ >= static_cast<int>(text_buffer_.size()))
        return -1;

    int c = text_buffer_[line_index_].text[column_index_];

    if (c == '#' && !in_string())
        return '\n';

    return c;
}

void parser::match(int c)
{
    if (get() != c)
        throw_syntax_error();
}

static void parse_structure(structure_data& data, parser& p,
    string const& location);
static void parse_list(list_data& data, parser& p,
    string const& location);

static value* parse_value(parser& p, string const& location)
{
    value* v;
    v = new value();

    int c = p.peek();

    // structure
    if (c == '{')
    {
        p.skip_whitespace();
        p.match('{');
        alia__shared_ptr<structure_data> data(new structure_data);
        data->file = p.get_file();
        data->location = location;
        parse_structure(*data, p, location);
        p.match('}');
        v = new structure_value(data);
        v->type_name = "structure";
    }

    // list
    else if (c == '[')
    {
        p.skip_whitespace();
        p.match('[');
        alia__shared_ptr<list_data> data(new list_data);
        parse_list(*data, p, location);
        p.match(']');
        v = new list_value(data);
        v->type_name = "list";
    }

    // string
    else if (c == '"')
    {
        string s;
        int i = 0;
        p.match('"');
        p.in_string(true);
        while (1)
        {
            int c = p.get();
            if (c == '"')
                break;
            if (c == '\n')
                p.throw_syntax_error();
            s.resize(i + 1);
            s[i++] = c;
        }
        p.in_string(false);
        v = new string_value(s);
        v->type_name = "string";
    }

    // option
    else if (std::isalpha(c))
    {
        string s;
        int i = 0;
        while (1)
        {
            int c = p.peek();
            if (std::isalpha(c) || std::isdigit(c) || c == '-' || c == '_')
            {
                s.resize(i + 1);
                s[i++] = p.get();
            }
            else
                break;
        }
        v = new option_value(s);
        v->type_name = "option";
    }

    // number
    else if (std::isdigit(c) || c == '+' || c == '.' || c == '-')
    {
        string s;
        int i = 0;
        while (1)
        {
            int c = p.peek();
            if (std::isdigit(c) || c == '+' || c == '.' || c == '-')
            {
                s.resize(i + 1);
                s[i++] = p.get();
            }
            else
                break;
        }
        double d;
        try
        {
            d = boost::lexical_cast<double>(s);
        }
        catch (boost::bad_lexical_cast&)
        {
            p.throw_syntax_error();
        }
        v = new double_value(d);
        v->type_name = "real number";
    }

    // points
    else if (c == '(')
    {
        p.get();
        double nums[3];
        int n = 0;
        while (1)
        {
            p.skip_whitespace();
            string s;
            int i = 0;
            while (1)
            {
                int c = p.peek();
                if (std::isdigit(c) || c == '+' || c == '.' || c == '-')
                {
                    s.resize(i + 1);
                    s[i++] = p.get();
                }
                else
                    break;
            }

            try
            {
                nums[n++] = boost::lexical_cast<double>(s);
            }
            catch (boost::bad_lexical_cast&)
            {
                p.throw_syntax_error();
            }

            p.skip_whitespace();
            int next_c = p.get();
            if (next_c == ',')
            {
                if (n > 2)
                    p.throw_syntax_error();
            }
            else if (next_c == ')')
            {
                if (n == 3)
                {
                    v = new vector3d_value(make_vector<double>(
                        nums[0], nums[1], nums[2]));
                    v->type_name = "triple of real numbers";
                    break;
                }
                else if (n == 2)
                {
                    v = new vector2d_value(make_vector<double>(
                        nums[0], nums[1]));
                    v->type_name = "pair of real numbers";
                    break;
                }
                else
                    p.throw_syntax_error();
            }
            else
                p.throw_syntax_error();
        }
    }

    // anything else
    else
        p.throw_syntax_error();

    v->file = p.get_file();
    v->location = location;
    return v;
}

static void parse_structure(structure_data& data, parser& p,
    string const& location)
{
    while (1)
    {
        p.skip_whitespace();
        int c = p.peek();
        if (c == '}' || c == -1)
            break;

        string id;
        int i = 0;
        while (1)
        {
            c = p.peek();
            if (std::isalpha(c) || std::isdigit(c) || c == '-' || c == '_')
            {
                id.resize(i + 1);
                id[i++] = p.get();
            }
            else
                break;
        }
        if (id.empty() || (!std::isspace(c) && c != '{' && c != '[' && c != '('))
            p.throw_syntax_error();

        p.skip_whitespace();
        data.contents[id].reset(parse_value(p, location + "." + id));
    }
}

static void parse_list(list_data& data, parser& p, string const& location)
{
    int n = 0;
    while (1)
    {
        p.skip_whitespace();
        int c = p.peek();
        if (c == ']' || c == -1)
            break;
        p.skip_whitespace();
        data.contents.push_back(value_ptr(parse_value(p,
            location + "[" + boost::lexical_cast<string>(n) + "]")));
    }
}

void read_file(config::structure& structure,
    file_path const& file)
{
    alia__shared_ptr<structure_data> data(new structure_data);
    data->file = file;
    structure = config::structure(data);
    parser p(file);
    parse_structure(*data, p, "");
    p.skip_whitespace();
    if (!p.eof())
        p.throw_syntax_error();
}

syntax_error::syntax_error(file_path const& file,
    int line, int column, string const& msg)
  : exception(file, str(boost::format("%i:%i") % line % column), msg)
  , line_(line)
  , column_(column)
{}

}}}
