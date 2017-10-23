#ifndef CRADLE_IO_RAW_MEMORY_IO_HPP
#define CRADLE_IO_RAW_MEMORY_IO_HPP

#include <boost/numeric/conversion/cast.hpp>

#include <cradle/common.hpp>
#include <cradle/endian.hpp>

// This file provides utilities for reading and writing data to and from
// raw memory buffers.

namespace cradle {

// READING

struct corrupt_data : exception
{
    corrupt_data() : exception("data block is corrupt") {}
    ~corrupt_data() throw() {}
};

struct raw_memory_reader
{
    raw_memory_reader(uint8_t const* buffer, size_t size)
      : buffer(buffer), size(size) {}
    uint8_t const* buffer;
    size_t size;
};

void raw_read(raw_memory_reader& r, void* dst, size_t size);

template<class Integer>
Integer read_int(raw_memory_reader& r)
{
    Integer i;
    raw_read(r, &i, sizeof(Integer));
    swap_on_little_endian(&i);
    return i;
}

string read_string(raw_memory_reader& r, size_t length);

template<class LengthType>
string read_string(raw_memory_reader& r)
{
    auto length = read_int<LengthType>(r);
    return read_string(r, length);
}

void static inline
advance(raw_memory_reader& r, size_t size)
{
    r.buffer += size;
    r.size -= size;
}

// WRITING

typedef std::vector<boost::uint8_t> byte_vector;

struct raw_memory_writer
{
    raw_memory_writer(byte_vector& buffer) : buffer(&buffer) {}
    byte_vector* buffer;
};

void raw_write(raw_memory_writer& w, void const* src, size_t size);

template<class Integer>
void write_int(raw_memory_writer& w, Integer i)
{
    swap_on_little_endian(&i);
    raw_write(w, &i, sizeof(Integer));
}

void write_float(raw_memory_writer& w, float f);

// Write the characters in a string, but not its length.
void write_string_contents(raw_memory_writer& w, string const& s);

template<class LengthType>
void write_string(raw_memory_writer& w, string const& s)
{
    auto length = boost::numeric_cast<LengthType>(s.length());
    write_int(w, length);
    write_string_contents(w, s);
}

}

#endif
