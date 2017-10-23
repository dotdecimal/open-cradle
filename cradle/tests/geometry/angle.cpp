#include <boost/assign/std/vector.hpp>
#include <cradle/geometry/angle.hpp>

#define BOOST_TEST_MODULE angle
#include <cradle/test.hpp>

using namespace cradle;
using namespace boost::assign;

BOOST_AUTO_TEST_CASE(angle_test)
{
    angle<double,radians> r(7 * pi / 2);
    angle<double,degrees> d(r);
    d.normalize();
    CRADLE_CHECK_ALMOST_EQUAL(d.get(), -90.);
    d = -1000;
    d.normalize();
    CRADLE_CHECK_ALMOST_EQUAL(d.get(), 80.);
    d = -180;
    d.normalize();
    CRADLE_CHECK_ALMOST_EQUAL(d.get(), 180.);
    CRADLE_CHECK_ALMOST_EQUAL((angle<double,radians>(d).get()), pi);
    d = d + d;
    d.normalize();
    CRADLE_CHECK_ALMOST_EQUAL(d.get(), 0.);
    CRADLE_CHECK_ALMOST_EQUAL(sin(d), 0.);
    CRADLE_CHECK_ALMOST_EQUAL(cos(d), 1.);
    d = d + angle<double,degrees>(1);
    CRADLE_CHECK_ALMOST_EQUAL(d.get(), 1.);
    CRADLE_CHECK_ALMOST_EQUAL(sin(d), 0.017452406437283512819418978516316);
    CRADLE_CHECK_ALMOST_EQUAL(cos(d), 0.99984769515639123915701155881391);
}
