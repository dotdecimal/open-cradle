#include <cradle/io/compression.hpp>
#include <cradle/io/file.hpp>
#include <boost/scoped_array.hpp>
#include <boost/filesystem/operations.hpp>
#include <cstdlib>

#define BOOST_TEST_MODULE compression
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(to_file)
{
    unsigned const data_size = 0x20301;
    boost::scoped_array<cradle::uint8_t> data(new cradle::uint8_t[data_size]);
    for (unsigned i = 0; i != data_size; ++i)
        data[i] = (std::rand() & 0x7f) + 0x70;

    {
    c_file f("compressed", "wb");
    compress(f, data.get(), data_size);
    }
    boost::scoped_array<cradle::uint8_t> decompressed_data(
        new cradle::uint8_t[data_size]);
    {
    c_file f("compressed", "rb");
    decompress(decompressed_data.get(), data_size, f);
    }

    BOOST_CHECK_EQUAL_COLLECTIONS(data.get(), data.get() + data_size,
        decompressed_data.get(), decompressed_data.get() + data_size);

    remove(boost::filesystem::path("compressed"));
}

BOOST_AUTO_TEST_CASE(to_memory)
{
    std::size_t const data_size = 0x3020401;
    boost::scoped_array<cradle::uint8_t> data(new cradle::uint8_t[data_size]);
    for (std::size_t i = 0; i < data_size; ++i)
        data[i] = (std::rand() & 0x7f) + 0x70;

    boost::scoped_array<cradle::uint8_t> compressed_data;
    std::size_t compressed_data_size;
    compress(&compressed_data, &compressed_data_size,
        data.get(), data_size);

    boost::scoped_array<cradle::uint8_t> decompressed_data(
        new cradle::uint8_t[data_size]);
    decompress(decompressed_data.get(), data_size,
        compressed_data.get(), compressed_data_size);

    BOOST_CHECK_EQUAL_COLLECTIONS(data.get(), data.get() + data_size,
        decompressed_data.get(), decompressed_data.get() + data_size);
}
