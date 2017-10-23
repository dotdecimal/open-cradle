#include <cradle/imaging/histogram_equalize.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE histogram_equalize
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(histogram_equalize_test)
{
    unsigned const s = 2;
    cradle::uint16_t data[s * s] = {
        4, 4,
        5, 6
    };
    image<2,cradle::uint16_t,const_view> src =
        make_const_view(data, make_vector(s, s));

    image<2,cradle::uint8_t,unique> result;
    create_image(result, make_vector(s, s));
    histogram_equalize(result, src);
    cradle::uint8_t results[s * s] = {
        0,   0,
      170, 255,
    };
    CRADLE_CHECK_IMAGE(result, &results[0], results + s * s);
}
