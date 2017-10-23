#include <cradle/geometry/decode_matrix.hpp>
#include <cradle/geometry/transformations.hpp>

#define BOOST_TEST_MODULE decode_matrix
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(has_rotation_2d_test)
{
    typedef angle<double,degrees> angle_t;
    BOOST_CHECK(has_rotation(rotation(angle_t(1))));
    BOOST_CHECK(!has_rotation(translation(make_vector<double>(1, 0))));
    BOOST_CHECK(!has_rotation(rotation(angle_t(0))));
    BOOST_CHECK(has_rotation(rotation(angle_t(1)) *
        translation(make_vector<double>(1, 0))));
    BOOST_CHECK(!has_rotation(scaling_transformation(
        make_vector<double>(1, 0))));
    BOOST_CHECK(has_rotation(rotation(angle_t(1)) *
        scaling_transformation(make_vector<double>(1, 0))));
}
