// Enable 64-bit file offsets on Linux.
#define _FILE_OFFSET_BITS 64

#include <cradle/io/file.hpp>
#include <cstring>
#include <boost/format.hpp>

namespace boost { namespace CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE {

size_t deep_sizeof(path const& x)
{
    return cradle::deep_sizeof(x.string());
}

void to_value(cradle::value* v, path const& x)
{
    to_value(v, x.string());
}
void from_value(path* x, cradle::value const& v)
{
    cradle::string s;
    from_value(&s, v);
    *x = path(s);
}

alia::string to_string(path const& x)
{
    return x.string();
}
void from_string(path* x, alia::string const& s)
{
    *x = path(s);
}

}}

namespace cradle {

string get_extension(file_path const& path)
{
    return get_extension(path.filename().string());
}

string get_extension(string const& name)
{
    size_t pos = name.rfind('.');
    if (pos == string::npos)
        return "";
    else
        return name.substr(pos + 1);
}

// OPEN

// Convert an open mode to a string.
static string openmode_to_string(std::ios::openmode mode)
{
    string str;

    if ((mode & std::ios::app) != 0)
        str += "app";
    if ((mode & std::ios::ate) != 0)
    {
        if (str != "")
            str += "|";
        str += "ate";
    }
    if ((mode & std::ios::binary) != 0)
    {
        if (str != "")
            str += "|";
        str += "binary";
    }
    if ((mode & std::ios::in) != 0)
    {
        if (str != "")
            str += "|";
        str += "in";
    }
    if ((mode & std::ios::out) != 0)
    {
        if (str != "")
            str += "|";
        str += "out";
    }
    if ((mode & std::ios::trunc) != 0)
    {
        if (str != "")
            str += "|";
        str += "trunc";
    }
    return str;
}

// When using fstream, you can pass in the mode of the failed open
// operation, and it will be used to format the error message.
open_file_error::open_file_error(file_path const& file,
    std::fstream::openmode mode, string const& msg)
  : file_error(file, msg + " (mode: " + openmode_to_string(mode) + ")")
{}

void open(std::fstream& file, file_path const& path,
    std::ios::openmode mode)
{
    file.open(path.c_str(), mode);
    if (!file)
        throw open_file_error(path, mode);
    file.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
}

void open(std::ifstream& file, file_path const& path,
    std::ios::openmode mode)
{
    file.open(path.c_str(), mode);
    if (!file)
        throw open_file_error(path, mode);
    file.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
}

void open(std::ofstream& file, file_path const& path,
    std::ios::openmode mode)
{
    file.open(path.c_str(), mode);
    if (!file)
        throw open_file_error(path, mode);
    file.exceptions(std::ios::eofbit | std::ios::failbit | std::ios::badbit);
}

// C_FILE

c_file::c_file(file_path const& file, const char* mode)
{
    f_ = fopen(file.string<string>().c_str(), mode);
    if (f_ == NULL)
    {
        throw file_error(file, string("unable to open file (mode '")
            + mode + "')");
    }
    path_ = file;
}

void c_file::read(void* data, size_t size)
{
    // MS can't deal with large blocks of data, so we need to break it up for
    // them.
    size_t max_block_size = 0x40000000; // 1GB
    while (size > max_block_size)
    {
        if (fread(data, 1, max_block_size, f_) != max_block_size)
            throw_error("fread failed");
        data = reinterpret_cast<uint8_t*>(data) + max_block_size;
        size -= max_block_size;
    }
    if (fread(data, 1, size, f_) != size)
        throw_error("fread failed");
}

void c_file::write(void const* data, size_t size)
{
    // MS can't deal with large blocks of data, so we need to break it up for
    // them.
    size_t max_block_size = 0x40000000; // 1GB
    while (size > max_block_size)
    {
        if (fwrite(data, 1, max_block_size, f_) != max_block_size)
            throw_error("fwrite failed");
        data = reinterpret_cast<uint8_t const*>(data) + max_block_size;
        size -= max_block_size;
    }
    if (fwrite(data, 1, size, f_) != size)
        throw_error("fwrite failed");
}

string c_file::read_line()
{
    char buf[1024];
    if (fgets(buf, 1023, f_) == NULL)
        throw_error("fgets failed");
    buf[strlen(buf) - 1] = '\0';
    return buf;
}

void c_file::seek(int64_t offset, int whence)
{
  #ifdef WIN32
    if (_fseeki64(f_, offset, whence) != 0)
        throw_error("fseek failed");
  #else
    if (fseeko(f_, offset, whence) != 0)
        throw_error("fseek failed");
  #endif
}

int64_t c_file::tell()
{
  #ifdef WIN32
    int64_t p = _ftelli64(f_);
  #else
    int64_t p = ftello(f_);
  #endif
    if (p < 0)
        throw_error("ftell failed");
    return p;
}

int64_t c_file::length()
{
    int64_t cur = tell();
    seek(0, SEEK_END);
    int64_t end = tell();
    seek(cur, SEEK_SET);
    return end;
}

void c_file::throw_error(string const& msg)
{
    int error = ferror(f_);
    if (error != 0)
        throw file_error(path_, msg + std::strerror(error));
    else if (feof(f_))
        throw file_error(path_, msg + ": EOF");
    else
        throw file_error(path_, msg);
}

// SIMPLE_PARSER

simple_file_parser::simple_file_parser(file_path const& file_path)
{
    file_path_ = file_path;
    f_ = std::fopen(file_path.string<string>().c_str(), "rt");
    if (f_ == NULL)
        throw file_error(file_path, "unable to open file");
    line_n_ = col_n_ = 0;
    buffer_.resize(1, '\0');
    p_ = &buffer_[0];
    eof_ = false;
    mode_ = LINE_BY_LINE;
}

void simple_file_parser::get_line()
{
    if (!eof_)
    {
        line_n_++;
        buffer_.clear();
        while (1)
        {
            int c = fgetc(f_);
            if (c == EOF)
            {
                if (ferror(f_))
                {
                    throw_error("disk access error");
                }
                else
                {
                    buffer_.push_back('\0');
                    eof_ = true;
                    break;
                }
            }
            else if (c == '\n')
            {
                buffer_.push_back('\0');
                break;
            }
            else
            {
                buffer_.push_back(char(c));
            }
        }
        p_ = &buffer_[0];
    }
}

char simple_file_parser::peek()
{
    skip_space();
    return *p_;
}

void simple_file_parser::check_char(char expected)
{
    skip_space();
    if (*p_ != expected)
        throw_unexpected_char();
    ++p_;
}

void simple_file_parser::skip_char()
{
    if (*p_ == '\0')
        throw_error("unexpected EOL");
    ++p_;
}

bool simple_file_parser::eol()
{
    skip_space();
    return *p_ == '\0';
}

void simple_file_parser::check_eol()
{
    if (!eol())
        throw_unexpected_char();
}

int simple_file_parser::read_int()
{
    skip_space();
    char *end;
    int i = strtol(p_, &end, 10);
    if (end == p_)
        throw_error("missing integer");
    p_ = end;
    return i;
}

unsigned simple_file_parser::read_unsigned()
{
    int i = read_int();
    if (i < 0)
        throw_error("expected natural number; got negative");
    return unsigned(i);
}

double simple_file_parser::read_double()
{
    skip_space();
    char *end;
    double d = strtod(p_, &end);
    if (end == p_)
        throw_error("missing number");
    p_ = end;
    return d;
}

string simple_file_parser::read_string(int end_marker)
{
    skip_space();
    if (*p_ == '\0')
        throw_error("missing string");
    char *start = p_;
    if (end_marker == -1)
    {
        while (!isspace (*p_) && *p_ != '\0')
            ++p_;
    }
    else
    {
        while (*p_ != (char) end_marker && *p_ != '\0')
            ++p_;
    }
    if (*p_ != '\0')
    {
        *p_ = '\0';
        ++p_;
    }
    return start;
}

string simple_file_parser::read_rest_of_line()
{
    char *start = p_;
    while (*p_ != '\r' && *p_ != '\n' && *p_ != '\0')
        ++p_;
    if (*p_ != '\0')
    {
        *p_ = '\0';
        ++p_;
    }
    return start;
}

string simple_file_parser::read_identifier()
{
    skip_space();
    if (!isalpha(*p_) && *p_ != '_')
        throw_error("missing identifier");
    char *start = p_;
    while (isalnum (*p_) || *p_ == '_')
        ++p_;
    char buf[1024];
    std::memcpy(buf, start, p_ - start);
    buf[p_ - start] = '\0';
    return buf;
}

simple_file_parser::~simple_file_parser()
{
    if (f_ != NULL)
        std::fclose (f_);
}

void simple_file_parser::skip_space()
{
start:
    while (isspace(*p_))
        ++p_;

    if ((mode_ & POUND_COMMENTS) != 0 && *p_ == '#')
    {
        get_line();
        goto start;
    }

    if ((mode_ & CONTINUOUS) != 0 && *p_ == '\0' && !eof_)
    {
        get_line();
        goto start;
    }
}

bool simple_file_parser::eof()
{
    skip_space();
    return eof_;
}

void simple_file_parser::throw_error(string const& msg) const
{
    throw file_error(file_path_, (boost::format("%i:%i: error: %s")
        % line_n_ % (p_ - &buffer_[0]) % msg).str());
}

void simple_file_parser::throw_unexpected_char() const
{
    if (*p_ == '\0')
    {
        throw_error("unexpected EOL");
    }
    else
    {
        throw_error((boost::format("unexpected character: %c (0x%02x)")
            % *p_ % static_cast<int>(*p_)).str());
    }
}

string get_file_contents(file_path const& path)
{
    std::ifstream in;
    open(in, path, std::ios::in | std::ios::binary);
    if (in)
    {
        string contents;
        in.seekg(0, std::ios::end);
        contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    }
    throw(errno);
}

}
