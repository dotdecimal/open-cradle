#include <cradle/imaging/blend.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE blend
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(blend_test)
{
    unsigned const s = 3;

    cradle::uint8_t data1[s * s] = {
        0, 0, 0,
        0,10, 0,
        0, 0, 0,
    };
    image<2,cradle::uint8_t,const_view> src1 =
        make_const_view(data1, make_vector<unsigned>(s, s));

    cradle::uint8_t data2[s * s] = {
        2, 2, 0,
        0, 0, 6,
        0, 0, 8,
    };
    image<2,cradle::uint8_t,const_view> src2 =
        make_const_view(data2, make_vector<unsigned>(s, s));

    image<2,cradle::uint8_t,unique> result;
    create_image(result, make_vector<unsigned>(s, s));

    blend_images(result, src1, src2, 0.5);
    cradle::uint8_t results1[] = {
        1, 1, 0,
        0, 5, 3,
        0, 0, 4,
    };
    CRADLE_CHECK_IMAGE(result, results1, results1 + s * s);

    blend_images(result, src1, src2, 0.3);
    cradle::uint8_t results2[] = {
        1, 1, 0,
        0, 3, 4,
        0, 0, 6,
    };
    CRADLE_CHECK_IMAGE(result, results2, results2 + s * s);
}
