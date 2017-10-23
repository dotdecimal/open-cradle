#include <cradle/io/raw_memory_io.hpp>

#include <cstring>

namespace cradle {

void raw_read(raw_memory_reader& r, void* dst, size_t size)
{
    if (size > r.size)
        throw corrupt_data();
    std::memcpy(dst, r.buffer, size);
    advance(r, size);
}

string read_string(raw_memory_reader& r, size_t length)
{
    string s;
    s.resize(length);
    raw_read(r, &s[0], length);
    return s;
}

// WRITING

void raw_write(raw_memory_writer& w, void const* src, size_t size)
{
    size_t current_size = w.buffer->size();
    w.buffer->resize(w.buffer->size() + size);
    std::memcpy(&(*w.buffer)[0] + current_size, src, size);
}

void write_float(raw_memory_writer& w, float f)
{
    swap_on_little_endian(reinterpret_cast<uint32_t*>(&f));
    raw_write(w, &f, 4);
}

void write_string_contents(raw_memory_writer& w, string const& s)
{
    raw_write(w, &s[0], s.length());
}

}
