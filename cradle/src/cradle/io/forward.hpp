#ifndef CRADLE_IO_FORWARD_HPP
#define CRADLE_IO_FORWARD_HPP

#include <boost/version.hpp>

#if BOOST_VERSION >= 105100
    #define CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE filesystem
#else
    #define CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE filesystem3
#endif

namespace boost { namespace CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE {

class path;

}}

namespace cradle {

typedef boost::CRADLE_IO_BOOST_FILESYSTEM_NAMESPACE::path file_path;

struct c_file;

namespace impl { namespace config {
    class structure;
    template<typename T>
    class list;
}}

struct file_error;

struct simple_file_parser;

struct filesystem_item;
struct filesystem_item_contents;

struct text_parser;

}

#endif
