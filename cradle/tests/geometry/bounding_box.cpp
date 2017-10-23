#include <boost/assign/std/vector.hpp>
#include <cradle/geometry/common.hpp>

#define BOOST_TEST_MODULE bouding_box
#include <cradle/test.hpp>

using namespace cradle;
using namespace boost::assign;

BOOST_AUTO_TEST_CASE(vector_of_points_test)
{
    std::vector<vector2i> points;
    points +=
        make_vector( 0,  0),
        make_vector(-1, -1),
        make_vector(-3,  0),
        make_vector( 0,  7),
        make_vector( 3,  3),
        make_vector( 3,  2);
    BOOST_CHECK_EQUAL(bounding_box(points),
        box2i(make_vector(-3, -1), make_vector(6, 8)));
}

BOOST_AUTO_TEST_CASE(structure_geometry_test)
{
}
