#include <cradle/imaging/discretize.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE discretize
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(discretize_test)
{
    unsigned const s = 3;
    double data[s * s] = {
        13, 11.5,   7,
         4,    1,   4,
         2,    2, 5.5,
    };

    image<2,double,const_view> source =
        make_const_view(data, make_vector(s, s));
    source.value_mapping = linear_function<double>(0, 1);
    set_spatial_mapping(source, make_vector<double>(4, 0),
        make_vector<double>(3, 2));

    image<2,cradle::uint8_t,shared> result;
    discretize(result, source, 255);

    cradle::uint8_t correct_result[] = {
        255, 223, 128,
         64,   0,  64,
         21,  21,  96
    };
    CRADLE_CHECK_IMAGE(result, &correct_result[0], correct_result + s * s);
}
