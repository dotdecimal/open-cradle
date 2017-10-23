#include <cradle/imaging/apply_palette.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE apply_palette
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(apply_palette_test)
{
    unsigned const s = 3;
    cradle::uint8_t data[s * s] = {
        4, 3, 0,
        0,10,70,
        1, 0, 9,
    };
    image<2,cradle::uint8_t,const_view> src =
        make_const_view(data, make_vector(s, s));

    cradle::uint16_t palette[256];
    for (int i = 0; i < 256; ++i)
        palette[i] = i * 7;

    image<2,cradle::uint16_t,unique> result;
    create_image(result, make_vector(s, s));

    apply_palette(result, src, palette);
    cradle::uint16_t correct_results[] = {
       28,21,  0,
        0,70,490,
        7, 0, 63,
    };
    CRADLE_CHECK_IMAGE(result, correct_results, correct_results + s * s);
}
