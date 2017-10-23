#ifndef CRADLE_IO_FILE_HPP
#define CRADLE_IO_FILE_HPP

#include <cradle/common.hpp>
#include <cradle/io/forward.hpp>
#include <cstdio>
#include <fstream>
#include <boost/filesystem/path.hpp>

// This file provides low-level utilities for working with files.

namespace boost { namespace CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE {

static inline cradle::raw_type_info get_type_info(path const&)
{
    using cradle::get_type_info;
    return get_type_info(std::string());
}
size_t deep_sizeof(path const& x);
void to_value(cradle::value* v, path const& x);
void from_value(path* x, cradle::value const& v);

alia::string to_string(path const& x);
void from_string(path* x, alia::string const& s);

}}

namespace std {

template<>
struct hash<cradle::file_path>
{
    size_t operator()(cradle::file_path const& x) const
    { return alia::invoke_hash(x.string<std::string>()); }
};

}

namespace cradle {

// general-purpose file-related error
struct file_error : exception
{
    file_error(file_path const& file, string const& msg)
      : exception(file.string() + ": " + msg)
      , file_(new file_path(file))
    {}
    ~file_error() throw() {}

    file_path const& file() const
    { return *file_; }

 private:
    alia__shared_ptr<file_path> file_;
};

// An error in trying to open a file.
struct open_file_error : file_error
{
    open_file_error(file_path const& file,
        string const& msg = "unable to open file")
      : file_error(file, msg)
    {}

    // When using fstream, you can pass in the mode of the failed open
    // operation, and it will be used to format the error message.
    open_file_error(file_path const& file,
        std::fstream::openmode mode,
        string const& msg = "unable to open file");

    ~open_file_error() throw() {}
};

// Open a file into the given fstream.  Throws an error if the open operation
// fails, and also enables the exception bits on the fstream so that subsequent
// failures will throw exceptions.
void open(std::fstream& file, file_path const& path,
    std::ios::openmode mode);
void open(std::ifstream& file, file_path const& path,
    std::ios::openmode mode);
void open(std::ofstream& file, file_path const& path,
    std::ios::openmode mode);

// Get the extension of the given file path.
string get_extension(file_path const& path);
// Same, but just takes the leaf (the file name).
string get_extension(string const& name);

struct c_file : noncopyable
{
    // default constructor - Creates an invalid file object.
    c_file() { f_ = NULL; }

    // constructor - Calls fopen().
    // Throws an exception if the file fails to open.
    c_file(file_path const& file, const char *mode);

    // constructor - Accepts an already open file.
    // The path to the file is only supplied for error reporting purposes.
    c_file(FILE* f, file_path const& path)
     : f_(f), path_(path) {}

    // destructor
    ~c_file() { if (f_) fclose(f_); }

    // Read a block of data from the file.
    // Throws an exception if the read fails (including due to EOF).
    void read(void* data, size_t size);

    template<class Value>
    void read(Value* value) { read(value, sizeof(Value)); }

    // Write a block of data to the file.
    // Throws an exception if the write fails.
    void write(void const* data, size_t size);

    template<class Value>
    void write(Value value) { write(&value, sizeof(value)); }

    // Read a line of text.
    // Throws an exception if the read fails (including due to EOF).
    string read_line(void);

    // The CFile object can be used directly in place of a FILE*.
    operator FILE* () { return f_; }

    // Wrapper for fseek().
    void seek(int64_t offset, int whence);

    // Wrapper for ftell().
    int64_t tell();

    // Get the length of the file.
    int64_t length();

    // Detach from the file without closing it.
    void detach() { f_ = NULL; }

    // Throws an cradle::file::exception.  Checks for likely causes of the
    // error (using ferror() and feof()) and formats the message accordingly.
    void throw_error(string const& msg);

 private:
    FILE *f_;

    // path to the file (retained for error reporting purposes)
    file_path path_;
};

// A utility class for doing simple parsing of text files.
struct simple_file_parser
{
    // Constructor - Open the given file for parsing.
    simple_file_parser(file_path const& file_path);

    // Parsing modes.  The mode is the bitwise OR of any (sensible) set of the
    // following flags...
    // In LINE_BY_LINE mode, '\n's are special, and the user must manually
    // call get_line() before parsing each line.  This is the default.
    static int const LINE_BY_LINE = 0x0;
    // In CONTINUOUS mode, '\n's are treated like any other white space, and
    // the parser will automatically call get_line() when needed.
    static int const CONTINUOUS = 0x1;
    // With POUND_COMMENTS mode enabled, the parser will automatically skip any
    // # comments that it finds.  # comments begin with a '#' and continue
    // until the end of the line.
    static int const POUND_COMMENTS = 0x2;
    // Set the parsing mode.
    void set_mode(int mode) { mode_ = mode; }

    // Read the next line of the file.
    void get_line();

    // Is the file at EOF?
    bool eof();

    // Peek at the next character without consuming it.
    char peek();

    // Confirm that the next non-space character is as expected and advance the
    // current line position past it.  If the expected character is not found,
    // an exception is thrown.
    void check_char(char expected);

    // Is the file is at the end of the current line?
    bool eol();

    // Confirm that the file is at the end of the current line.  If not, throw
    // an exception.
    void check_eol();

    // Read an integer (and consume it).  If the next token is not a valid
    // integer, an exception is thrown.
    int read_int();

    // Read an unsigned integer (and consume it).  If the next token is not a
    // valid, non-negative integer, an exception is thrown.
    unsigned read_unsigned();

    // Read a double (and consume it).  If the next token is not a valid
    // number, an exception is thrown.
    double read_double();

    // Read a string (and consume it).  The optional argument is the character
    // which marks the ends of the string.  The default value is any white
    // space.
    // NOTE: The end marker is also consumed.
    string read_string(int end_marker = -1);

    // Read a C-style identifier (and consume it).  If the next token is not a
    // valid identifier, an exception is thrown.
    string read_identifier();

    // Read the rest of the line.
    string read_rest_of_line();

    // Skip the next character.
    void skip_char();

    // Get the underlying C FILE structure.
    std::FILE* get_file() { return f_; }

    // Give up ownership of the underlying C FILE structure.
    // If detach() is called, the underlying C FILE structure will remain open
    // after the parser is destroyed.
    // Note that no more parsing can be done after detach() is called.
    void detach() { f_ = NULL; }

    ~simple_file_parser();

    // Throw an exception with the file path, line number, and column number
    // included in the description.
    void throw_error(string const& msg) const;

 private:
    // Skip over white space (and comments) on the current line.
    void skip_space();

    // Throw an unexpected character exception.
    void throw_unexpected_char() const;

    // The file.
    std::FILE *f_;

    // The original path used to open the file (kept for error reporting).
    file_path file_path_;

    // Current line number and column number. */
    int line_n_, col_n_;

    // Buffer for holding current line.
    std::vector<char> buffer_;

    // Current position within the buffer.
    char *p_;

    // Is the file at EOF yet?
    bool eof_;

    // Parsing mode.
    int mode_;
};

inline simple_file_parser& operator >> (simple_file_parser& p, int& i)
{ i = p.read_int(); return p; }
inline simple_file_parser& operator >> (simple_file_parser& p, unsigned& u)
{ u = p.read_unsigned(); return p; }
inline simple_file_parser& operator >> (simple_file_parser& p, float& f)
{ f = float(p.read_double()); return p; }
inline simple_file_parser& operator >> (simple_file_parser& p, double& d)
{ d = p.read_double(); return p; }

// Get the contents of a file as a string.
string get_file_contents(file_path const& path);

}

#endif
