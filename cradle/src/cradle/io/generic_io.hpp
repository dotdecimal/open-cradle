#ifndef CRADLE_IO_GENERIC_IO_HPP
#define CRADLE_IO_GENERIC_IO_HPP

#include <cradle/common.hpp>
#include <cradle/io/file.hpp>
#include <cradle/io/raw_memory_io.hpp>

// This file provides various forms of I/O for dynamic values.
// Since all regular CRADLE types are convertible to and from dynamic values,
// these provide generic mechanisms for reading and writing most data.

namespace cradle {

// STRING I/O - These convert values to and from human-readable strings.

void parse_value_string(value* v, string const& s);
void value_to_string(string* s, value const& v);

// Convert a string into blob data.
// This does NOT include a terminating null character.
blob string_to_blob(string const& s);

// JSON I/O - conversion to and from JSON strings

struct json_parse_error : exception
{
    json_parse_error(string const& message) : exception(message) {}
    ~json_parse_error() throw() {}
};

void parse_json_value(value* v, char const* json, size_t length);

value static inline
parse_json_value(char const* json, size_t length)
{
    value v;
    parse_json_value(&v, json, length);
    return v;
}

void static inline
parse_json_value(value* v, string const& json)
{ return parse_json_value(v, json.c_str(), json.length()); }

value static inline
parse_json_value(string const& json)
{
    value v;
    parse_json_value(&v, json);
    return v;
}

void value_to_json(string* json, value const& v);

string static inline
value_to_json(value const& v)
{
    string json;
    value_to_json(&json, v);
    return json;
}

// Write a value to a blob in JSON format.
// This does NOT include a terminating null character.
blob value_to_json_blob(value const& v);

// MSGPACK I/O - These convert values to and from MessagePack.

void parse_msgpack_value(value* v, uint8_t const* data, size_t size);

value parse_msgpack_value(string const& msgpack);

// This form takes a separate parameter that provides ownership of the data
// buffer. This allows the parser to store blobs by pointing into the original
// data rather than copying them.
void
parse_msgpack_value(
    value* v,
    ownership_holder const& ownership,
    uint8_t const* data,
    size_t size);

string value_to_msgpack_string(value const& v);

blob value_to_msgpack_blob(value const& v);

// MEMORY I/O - These convert values to and from blocks of bytes.
// The blocks are compressed and store an additional CRC value.
// The CRC check is done internally (a crc_error is throw if it doesn't match).
// Addtionally, both will write the CRC value to *crc if crc's not null.

struct crc_error : exception
{
    crc_error() : exception("CRC check failed") {}
    ~crc_error() throw() {}
};

void deserialize_value(value* v, uint8_t const* data, size_t size,
    uint32_t* crc = 0);

void serialize_value(byte_vector* data, value const& v, uint32_t* crc = 0);

// BASE-64 - CRC'd conversion to and from base-64 strings

void parse_base64_value_string(value* v, string const& s, uint32_t* crc = 0);

value static inline
parse_base64_value_string(string const& s)
{
    value v;
    parse_base64_value_string(&v, s);
    return v;
}

void value_to_base64_string(string* s, value const& v, uint32_t* crc = 0);

string static inline
value_to_base64_string(value const& v)
{
    string s;
    value_to_base64_string(&s, v);
    return s;
}

// FILE I/O - CRC'D file storage

void read_value_file(value* v, file_path const& file, uint32_t* crc = 0);
void write_value_file(file_path const& file, value const& v,
    uint32_t* crc = 0);


template<class T>
T read_value_file_as(cradle::file_path const& file)
{
    value v;
    read_value_file(&v, file);
    return from_value<T>(v);
}
// FILE/DIRECTORY SNAPSHOTS

struct filesystem_item;

// file system item contents
api(union)
union filesystem_item_contents
{
    // directory location of file system
    std::vector<object_reference<filesystem_item> > directory;
    // blob of file contents (struct: blob)
    blob file;
};

// file system item
api(struct)
struct filesystem_item
{
    // file system name
    string name;
    // union of either directory or file
    filesystem_item_contents contents;
};

void initialize_parser_with_file(text_parser& p, filesystem_item const& item);

}

#endif
