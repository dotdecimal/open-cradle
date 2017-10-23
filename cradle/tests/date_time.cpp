#include <cradle/date_time.hpp>
#include <cradle/common.hpp>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#define BOOST_TEST_MODULE date_time
#include <cradle/test.hpp>

BOOST_AUTO_TEST_CASE(value_conversion)
{
    using namespace cradle;

    date d(2012, boost::gregorian::Jun, 13);
    value v;
    to_value(&v, d);
    date e;
    from_value(&e, v);
    BOOST_CHECK_EQUAL(d, e);

    cradle::time t(d, boost::posix_time::hours(1));
    to_value(&v, t);
    cradle::time u;
    from_value(&u, v);
    BOOST_CHECK_EQUAL(t, u);
}
