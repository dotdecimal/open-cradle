#include <cradle/endian.hpp>

#define BOOST_TEST_MODULE cradle_endian
#include <cradle/test.hpp>

BOOST_AUTO_TEST_CASE(simple_test)
{
    using namespace cradle;

    BOOST_CHECK_EQUAL(swap_uint16_endian(0xf1f0), 0xf0f1);
    BOOST_CHECK_EQUAL(swap_uint32_endian(0xbaadf1f0), 0xf0f1adba);
}

BOOST_AUTO_TEST_CASE(array16_test)
{
    using namespace cradle;

    cradle::uint16_t array[] = { 0xf0f1, 0xbada, 0xcab1 },
        swapped[] = { 0xf1f0, 0xdaba, 0xb1ca };
    swap_array_endian(array, 3);
    for (int i = 0; i < 3; ++i)
        BOOST_CHECK_EQUAL(array[i], swapped[i]);
}

BOOST_AUTO_TEST_CASE(array32_test)
{
    using namespace cradle;

    cradle::uint32_t array[] = { 0xf0f113f3, 0xbadacab0, 0xcab14d3d },
        swapped[] = { 0xf313f1f0, 0xb0cadaba, 0x3d4db1ca };
    swap_array_endian(array, 3);
    for (int i = 0; i < 3; ++i)
        BOOST_CHECK_EQUAL(array[i], swapped[i]);
}
