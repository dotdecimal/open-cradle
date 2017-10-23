#ifndef CRADLE_IO_CONFIG_FILE_HPP
#define CRADLE_IO_CONFIG_FILE_HPP

#include <cradle/io/file.hpp>
#include <cradle/geometry/common.hpp>

// This is the original interface to the config file parser.
// Once cradle::value was introduced, this became an implementation detail.
// See read_nptc_config_file for the new interface.

namespace cradle { namespace impl { namespace config {

struct value;
class structure;
class untyped_list;

// types that can be read from a config file...
void match_value(int* dst, value const& v);
void match_value(unsigned* dst, value const& v);
void match_value(float* dst, value const& v);
void match_value(double* dst, value const& v);
void match_value(bool* dst, value const& v);
void match_value(vector2i* dst, value const& v);
void match_value(vector2u* dst, value const& v);
void match_value(vector2f* dst, value const& v);
void match_value(vector2d* dst, value const& v);
void match_value(vector3i* dst, value const& v);
void match_value(vector3u* dst, value const& v);
void match_value(vector3f* dst, value const& v);
void match_value(vector3d* dst, value const& v);
void match_value(string* dst, value const& v);
void match_value(structure* dst, value const& v);
void match_value(untyped_list* dst, value const& v);

// Gets an option value and matches the result against the given list of
// option strings (lowercase, separated by spaces).
// If it matches one, the index of the matching option is returned.
// If no match is found, a bad_option exception is thrown.
int match_option(value const& v, string const& options);

struct structure_data;

class structure
{
 public:
    structure(alia__shared_ptr<structure_data> const& external_data)
      : data_(external_data)
    {}
    structure() {}

    template<class T>
    T get(string const& name) const
    {
        T r;
        match_value(&r, get_value(name));
        return r;
    }

    template<class T>
    bool get(T* dst, string const& name) const
    {
        value const* v = get_optional_value(name);
        if (v)
        {
            match_value(dst, *v);
            return true;
        }
        else
            return false;
    }

    int get_option(string const& name, string const& options) const
    { return match_option(get_value(name), options); }

    bool get_option(int* dst, string const& name,
        string const& options) const
    {
        value const* v = get_optional_value(name);
        if (v)
        {
            *dst = match_option(*v, options);
            return true;
        }
        else
            return false;
    }

    structure_data const& data() const { return *data_; };

 private:
    value const& get_value(string const& name) const;
    value const* get_optional_value(string const& name) const;

    alia__shared_ptr<structure_data> data_;
};

struct list_data;

class untyped_list
{
 public:
    untyped_list(list_data const* external_data)
      : data_(external_data), index_(0)
    {}
    untyped_list() {}

    bool empty() const;
    size_t size() const;

    // Gets a value and advances the iterator past it.
    value const& get_value();

 private:
    list_data const* data_;
    size_t index_;
};

template<class T>
class list
{
 public:
    list(untyped_list const& list)
      : list_(list)
    {}

    bool empty() const { return list_.empty(); }
    unsigned size() const { return list_.size(); }

    // Gets a value and advances the iterator past it.
    T get()
    {
        T r;
        match_value(&r, list_.get_value());
        return r;
    }

 private:
    untyped_list list_;
};

// Open the given config file, parse its contents, and load it into the given
// structure.
void read_file(structure& structure, file_path const& path);

class exception : public file_error
{
 public:
    exception(file_path const& path,
        string const& location, string const& message)
      : file_error(path, location + ": " + message)
      , location_(new string(location))
    {}
    ~exception() throw() {}

    // Get the location within the config file at which the error occurred.
    string const& get_location() const { return *location_; }

 private:
    alia__shared_ptr<string> location_;
};

// thrown by get_option() when the option fails to match any of the options
class bad_option : public config::exception
{
 public:
    bad_option(file_path const& path,
        string const& location, string const& option)
      : exception(path, location, "bad option: " + option)
      , option_(new string(option))
    {}
    ~bad_option() throw() {}

    string const& get_option() const { return *option_; }

 private:
    alia__shared_ptr<string> option_;
};

class missing_variable : public config::exception
{
 public:
    missing_variable(file_path const& path,
        string const& location, string const& variable_name)
      : exception(path, location, "missing variable: " + variable_name)
      , variable_name_(new string(variable_name))
    {}
    ~missing_variable() throw() {}

    string const& get_variable_name() const { return *variable_name_; }

 private:
    alia__shared_ptr<string> variable_name_;
};

class type_mismatch : public config::exception
{
 public:
    type_mismatch(file_path const& path,
        string const& location,
        string const& expected,
        string const& got)
      : exception(path, location,
            "type mismatch, expected: " + expected + ", got: " + got)
      , expected_(new string(expected))
      , got_(new string(got))
    {}
    ~type_mismatch() throw() {}

    string const& get_expected() const { return *expected_; }
    string const& get_got() const { return *got_; }

 private:
    alia__shared_ptr<string> expected_;
    alia__shared_ptr<string> got_;
};

class syntax_error : public exception
{
  public:
    syntax_error(file_path const& file,
        int line, int column,
        string const& msg = "syntax error");
    ~syntax_error() throw() {}

    int get_line() const { return line_; }
    int get_column() const { return column_; }

  private:
    int line_, column_;
};

// internals - These were exposed to enable the creation of a
// cradle::value-oriented interface.

struct value : noncopyable
{
    virtual ~value() {}
    file_path file;
    string location;
    string type_name;
};
typedef alia__shared_ptr<value> value_ptr;

struct structure_data
{
    std::map<string,value_ptr> contents;
    file_path file;
    string location;
};

struct list_data
{
    std::vector<value_ptr> contents;
};

struct double_value : value
{
    double_value(double number) : number(number) {}
    double number;
};
struct vector2d_value : value
{
    vector2d_value(vector2d const& vector) : vector(vector) {}
    vector2d vector;
};
struct vector3d_value : value
{
    vector3d_value(vector3d const& vector) : vector(vector) {}
    vector3d vector;
};
struct string_value : value
{
    string_value(string const& s) : s(s) {}
    string s;
};
struct option_value : value
{
    option_value(string const& option) : option(option) {}
    string option;
};

struct structure_value : value
{
    structure_value(alia__shared_ptr<structure_data> const& data)
      : data(data)
    {}
    alia__shared_ptr<structure_data> data;
};
struct list_value : value
{
    list_value(alia__shared_ptr<list_data> const& data) : data(data) {}
    alia__shared_ptr<list_data> data;
};

}}}

#endif
