#ifndef CRADLE_IO_MSGPACK_IO_HPP
#define CRADLE_IO_MSGPACK_IO_HPP

#define MSGPACK_USE_CPP03
#ifdef _WIN32
#pragma warning(push, 0)
#include <msgpack.hpp>
#pragma warning(pop)
#else
#include <msgpack.hpp>
#endif

#include <cradle/common.hpp>
#include <cradle/date_time.hpp>
#include <cradle/endian.hpp>

// This file defines a generic implementation of msgpack I/O on cradle::values.
// (Currently, it actually only supplies output.)
//
// In other words, this takes care of understanding cradle::value and
// interfacing it with msgpack-c, but it leaves it up to you to supply the
// implementation of msgpack-c's Buffer concept and initialize the
// msgpack::packer object.

namespace cradle {

template<class Buffer>
void
write_msgpack_value(msgpack::packer<Buffer>& packer, value const& v)
{
    switch (v.type())
    {
     case value_type::NIL:
        packer.pack_nil();
        break;
     case value_type::BOOLEAN:
        if (cast<bool>(v))
            packer.pack_true();
        else
            packer.pack_false();
        break;
     case value_type::INTEGER:
        packer.pack_int64(cast<integer>(v));
        break;
     case value_type::FLOAT:
        packer.pack_double(cast<double>(v));
        break;
     case value_type::STRING:
      {
        auto const& s = cast<string>(v);
        packer.pack_str(boost::numeric_cast<uint32_t>(s.length()));
        packer.pack_str_body(
            s.c_str(),
            boost::numeric_cast<uint32_t>(s.length()));
        break;
      }
     case value_type::BLOB:
      {
        blob const& x = cast<blob>(v);
        // Check to make sure that the blob size is smaller than the
        // msgpack specification allows
        if (x.size >= 4294967296)
        {
            throw exception("blob size exceeds msgpack limit (4GB)");
        }
        packer.pack_bin(boost::numeric_cast<uint32_t>(x.size));
        packer.pack_bin_body(
            reinterpret_cast<char const*>(x.data),
            boost::numeric_cast<uint32_t>(x.size));
        break;
      }
     case value_type::DATETIME:
      {
        int8_t const ext_type = 1; // Thinknode datetime ext type
        int64_t t =
            (cast<time>(v) -
                boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))
              ).total_milliseconds();
        // We need to use the smallest possible int type to store the datetime.
        if (t >= -0x80 && t < 0x80)
        {
            int8_t x = int8_t(t);
            packer.pack_ext(1, ext_type);
            packer.pack_ext_body(reinterpret_cast<char const*>(&x), 1);
        }
        else if (t >= -0x8000 && t < 0x8000)
        {
            int16_t x = int16_t(t);
            swap_on_little_endian(reinterpret_cast<uint16_t*>(&x));
            packer.pack_ext(2, ext_type);
            packer.pack_ext_body(reinterpret_cast<char const*>(&x), 2);
        }
        else if (t >= -int64_t(0x80000000) && t < int64_t(0x80000000))
        {
            int32_t x = int32_t(t);
            swap_on_little_endian(reinterpret_cast<uint32_t*>(&x));
            packer.pack_ext(4, ext_type);
            packer.pack_ext_body(reinterpret_cast<char const*>(&x), 4);
        }
        else
        {
            int64_t x = int64_t(t);
            swap_on_little_endian(reinterpret_cast<uint64_t*>(&x));
            packer.pack_ext(8, ext_type);
            packer.pack_ext_body(reinterpret_cast<char const*>(&x), 8);
        }
        break;
      }
     case value_type::LIST:
      {
        value_list const& x = cast<value_list>(v);
        size_t size = x.size();
        packer.pack_array(boost::numeric_cast<uint32_t>(size));
        for (size_t i = 0; i != size; ++i)
            write_msgpack_value(packer, x[i]);
        break;
      }
     case value_type::MAP:
      {
        value_map const& x = cast<value_map>(v);
        packer.pack_map(boost::numeric_cast<uint32_t>(x.size()));
        for (auto const& i : x)
        {
            write_msgpack_value(packer, i.first);
            write_msgpack_value(packer, i.second);
        }
        break;
      }
    }
}

}

#endif
