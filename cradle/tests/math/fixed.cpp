#include <cradle/math/fixed.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE fixed
#include <cradle/test.hpp>

using namespace cradle;

template<unsigned FractionalBits>
void test_fixed()
{
    typedef fixed<int32_t,int64_t,FractionalBits>
        fixed;

    // constructors
    fixed a(1.25);
    fixed b(1);
    fixed c(b);

    // comparisons
    BOOST_CHECK(a == a);
    BOOST_CHECK(b == c);
    BOOST_CHECK(!(a == b));
    BOOST_CHECK(a != b);
    BOOST_CHECK(!(a != a));
    BOOST_CHECK(a <= a);
    BOOST_CHECK(a >= a);
    BOOST_CHECK(a >= b);
    BOOST_CHECK(b <= a);
    BOOST_CHECK(!(a < a));
    BOOST_CHECK(!(a > a));
    BOOST_CHECK(a > b);
    BOOST_CHECK(b < a);

    // conversions
    BOOST_CHECK(a.as_double() == 1.25);
    BOOST_CHECK(b.as_float() == 1);
    BOOST_CHECK(a.as_integer() == 1);

    // assignment
    c = fixed(2.5);
    BOOST_CHECK(b != c);
    BOOST_CHECK(c == fixed(2.5));

    // arithmetic
    BOOST_CHECK(a * fixed(2) == c);
    b = fixed(2);
    BOOST_CHECK(a * b == c);
    BOOST_CHECK(a + a == c);
    BOOST_CHECK(-b == fixed(-2));
    BOOST_CHECK(-a - a == -c);
    BOOST_CHECK(a - b == fixed(-0.75));
    BOOST_CHECK(a / b == fixed(0.625));
    a -= fixed(1);
    BOOST_CHECK(a == fixed(0.25));
    a *= fixed(4);
    BOOST_CHECK(a == fixed(1));
    a += fixed(2);
    BOOST_CHECK(a == fixed(3));
    a /= fixed(2);
    BOOST_CHECK(a == fixed(1.5));
}

BOOST_AUTO_TEST_CASE(fixed_test)
{
    test_fixed<16>();
    test_fixed<4>();
    test_fixed<12>();
    test_fixed<6>();
}
