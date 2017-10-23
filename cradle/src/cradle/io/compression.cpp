#include <cradle/io/compression.hpp>
#include <cradle/io/file.hpp>
#include <zlib.h>

#ifdef _WIN32
    #pragma comment (lib, "zlib.lib")
#endif

namespace cradle {

static std::size_t const buffer_size = 0x10000;
static std::size_t const block_size = 0x1000000;

struct inflate_ender
{
    inflate_ender(z_stream* strm)
      : strm_(strm)
    {}
    ~inflate_ender()
    { inflateEnd(strm_); }
    z_stream* strm_;
};

void decompress(void* dst, std::size_t dst_size,
    void const* src, std::size_t src_size)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    std::size_t remaining_src_size = src_size;
    strm.avail_in = uInt((std::min)(remaining_src_size, block_size));
    remaining_src_size -= strm.avail_in;
    strm.next_in = (Bytef*)(src);
    int rc = inflateInit(&strm);
    if (rc != Z_OK)
        throw zlib_error(rc);
    inflate_ender ender(&strm);

    std::size_t remaining_dst_size = dst_size;
    strm.avail_out = 0;
    strm.next_out = reinterpret_cast<Bytef*>(dst);

    do
    {
        if (strm.avail_out == 0)
        {
            if (remaining_dst_size == 0)
            {
                throw decompression_error(
                    "decompressed data is larger than expected");
            }
            strm.avail_out = uInt((std::min)(remaining_dst_size,
                block_size));
            remaining_dst_size -= strm.avail_out;
        }
        if (strm.avail_in == 0)
        {
            if (remaining_src_size == 0)
            {
                throw decompression_error(
                    "compressed data is corrupt; data ends unexpectedly");
            }
            strm.avail_in = uInt((std::min)(remaining_src_size,
                block_size));
            remaining_src_size -= strm.avail_in;
        }

        rc = inflate(&strm, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END)
            throw zlib_error(rc);
    }
    while (rc != Z_STREAM_END);

    if (strm.avail_out != 0 || remaining_dst_size != 0)
    {
        throw decompression_error(
            "decompressed data is smaller than expected");
    }
    if (strm.avail_in != 0 || remaining_src_size != 0)
    {
        throw decompression_error(
            "compressed data is corrupt; excess data at end");
    }
}
void decompress(void* dst, std::size_t dst_size, c_file& src)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int rc = inflateInit(&strm);
    if (rc != Z_OK)
        throw zlib_error(rc);
    inflate_ender ender(&strm);

    std::size_t remaining_dst_size = dst_size;
    strm.avail_out = 0;
    strm.next_out = reinterpret_cast<Bytef*>(dst);

    uint64_t remaining_input;
    src.read(&remaining_input);

    boost::scoped_array<uint8_t> in_buffer(new uint8_t[buffer_size]);
    do
    {
        if (strm.avail_out == 0)
        {
            if (remaining_dst_size == 0)
            {
                throw decompression_error(
                    "decompressed data is larger than expected");
            }
            strm.avail_out = uInt((std::min)(remaining_dst_size,
                block_size));
            remaining_dst_size -= strm.avail_out;
        }
        if (remaining_input == 0)
        {
            throw decompression_error(
                "compressed data is corrupt; data ends unexpectedly");
        }

        strm.avail_in = uInt((std::min)(remaining_input,
            uint64_t(buffer_size)));
        remaining_input -= strm.avail_in;
        src.read(in_buffer.get(), strm.avail_in);
        strm.next_in = in_buffer.get();

        rc = inflate(&strm, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END)
            throw zlib_error(rc);
    }
    while (rc != Z_STREAM_END);

    if (strm.avail_out != 0 || remaining_dst_size != 0)
    {
        throw decompression_error(
            "decompressed data is smaller than expected");
    }
    if (strm.avail_in != 0 || remaining_input != 0)
    {
        throw decompression_error(
            "compressed data is corrupt; excess data at end");
    }
}

void compress(boost::scoped_array<uint8_t>* dst, std::size_t* dst_size,
    void const* src, std::size_t src_size)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    std::size_t remaining_src_size = src_size;
    strm.avail_in = uInt((std::min)(remaining_src_size, block_size));
    remaining_src_size -= strm.avail_in;
    strm.next_in = (Bytef*)(src);
    int rc = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK)
        throw zlib_error(rc);

    try
    {
        // This is the conservative bound calculation done internally by zlib
        // in deflateBound(), but zlib does the calculation in unsigned long,
        // which Microsoft defines as 32-bit, even on 64-bit systems.
        std::size_t max_compressed_size =
            src_size + ((src_size + 7) >> 3) + ((src_size + 63) >> 6) + 11;
        dst->reset(new uint8_t[max_compressed_size]);

        std::size_t remaining_dst_size = max_compressed_size;
        strm.avail_out = 0;
        strm.next_out = dst->get();

        do
        {
            if (strm.avail_out == 0 && remaining_dst_size != 0)
            {
                strm.avail_out = uInt((std::min)(remaining_dst_size,
                    block_size));
                remaining_dst_size -= strm.avail_out;
            }
            if (strm.avail_in == 0 && remaining_src_size != 0)
            {
                strm.avail_in = uInt((std::min)(remaining_src_size,
                    block_size));
                remaining_src_size -= strm.avail_in;
            }

            rc = deflate(&strm,
                remaining_src_size != 0 ? Z_NO_FLUSH : Z_FINISH);
            if (rc != Z_OK && rc != Z_STREAM_END)
                throw zlib_error(rc);
        }
        while (rc != Z_STREAM_END);
        assert(strm.avail_in == 0);

        *dst_size = max_compressed_size - remaining_dst_size - strm.avail_out;
    }
    catch(...)
    {
        deflateEnd(&strm);
        throw;
    }

    deflateEnd(&strm);
}
void compress(c_file& dst, void const* src, std::size_t src_size)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    std::size_t remaining_src_size = src_size;
    strm.avail_in = uInt((std::min)(remaining_src_size, block_size));
    remaining_src_size -= strm.avail_in;
    strm.next_in = (Bytef*)(src);
    int rc = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK)
        throw zlib_error(rc);

    try
    {
        int64_t compressed_size_offset = dst.tell();
        uint64_t compressed_size = 0;
        dst.write(compressed_size);

        boost::scoped_array<uint8_t> out_buffer(new uint8_t[buffer_size]);
        do
        {
            if (strm.avail_in == 0 && remaining_src_size != 0)
            {
                strm.avail_in = uInt((std::min)(remaining_src_size,
                    block_size));
                remaining_src_size -= strm.avail_in;
            }

            strm.avail_out = buffer_size;
            strm.next_out = out_buffer.get();

            rc = deflate(&strm,
                remaining_src_size != 0 ? Z_NO_FLUSH : Z_FINISH);
            if (rc != Z_OK && rc != Z_STREAM_END)
                throw zlib_error(rc);

            uInt size_written = buffer_size - strm.avail_out;
            compressed_size += buffer_size - strm.avail_out;
            dst.write(out_buffer.get(), size_written);
        }
        while (rc != Z_STREAM_END);
        assert(strm.avail_in == 0 && remaining_src_size == 0);

        dst.seek(compressed_size_offset, SEEK_SET);
        dst.write(compressed_size);
        dst.seek(0, SEEK_END);
    }
    catch(...)
    {
        deflateEnd(&strm);
        throw;
    }

    deflateEnd(&strm);
}

string zlib_error_code_to_string(int error_code)
{
    switch (error_code)
    {
     case Z_DATA_ERROR:
        return "invalid input data";
     case Z_MEM_ERROR:
        return "out of memory";
     case Z_VERSION_ERROR:
        return "version mismatch";
     default:
        return "unknown error";
    }
}

zlib_error::zlib_error(int error_code)
 :  exception("zlib error: " + zlib_error_code_to_string(error_code))
 ,  error_code_(error_code)
{
}

}
