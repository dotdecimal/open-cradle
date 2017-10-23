#include <cradle/common.hpp>
#include <boost/shared_array.hpp>

#define BOOST_TEST_MODULE cradle_common
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(any_test)
{
    any a;
    a = int(1);
    BOOST_CHECK_EQUAL(unsafe_any_cast<int>(a), 1);
    BOOST_CHECK(!any_cast<double>(a));

    any b(a);
    BOOST_CHECK_EQUAL(unsafe_any_cast<int>(b), 1);
    BOOST_CHECK(!any_cast<double>(b));

    any c(1.5);
    BOOST_CHECK_EQUAL(unsafe_any_cast<double>(c), 1.5);
    BOOST_CHECK(any_cast<double>(c));
    BOOST_CHECK_EQUAL(*any_cast<double>(c), 1.5);

    b = c;
    BOOST_CHECK_EQUAL(unsafe_any_cast<double>(b), 1.5);
    BOOST_CHECK(any_cast<double>(b));
    BOOST_CHECK_EQUAL(*any_cast<double>(b), 1.5);
}

BOOST_AUTO_TEST_CASE(optional_test)
{
    optional<int> a;
    BOOST_CHECK(!a);
    a = 1;
    BOOST_CHECK(a);
    BOOST_CHECK_EQUAL(get(a), 1);

    optional<int> b = a;
    BOOST_CHECK(b);
    BOOST_CHECK(a == b);
    BOOST_CHECK_EQUAL(get(b), 1);

    a = none;
    BOOST_CHECK(a != b);
    BOOST_CHECK(a < b);
    BOOST_CHECK(!(b < a));
    BOOST_CHECK(!a);
    BOOST_CHECK(b);
    b = 2;
    a = 3;
    BOOST_CHECK(b);
    BOOST_CHECK_EQUAL(get(b), 2);
    BOOST_CHECK(a != b);
    BOOST_CHECK(b < a);
    BOOST_CHECK(!(a < b));
}

static blob make_blob(size_t size)
{
    blob blob;
    blob.size = size;
    boost::shared_array<cradle::uint8_t> storage(new cradle::uint8_t[size]);
    blob.ownership = storage;
    for (std::size_t i = 0; i != size; ++i)
        storage[i] = cradle::uint8_t(i + 1);
    blob.data = storage.get();
    return blob;
}

BOOST_AUTO_TEST_CASE(value_equality_tests)
{
    value t(true);
    value f(false);
    BOOST_CHECK(t == value(true));
    BOOST_CHECK(t != value(false));
    BOOST_CHECK(f == value(false));
    BOOST_CHECK(t != f);

    value x(0.1);
    value y(0.2);
    value z(0.1);
    BOOST_CHECK(x == value(0.1));
    BOOST_CHECK(x != y);
    BOOST_CHECK(y != z);
    BOOST_CHECK(x == z);
    BOOST_CHECK(z == x);

    BOOST_CHECK(t != x);
    BOOST_CHECK(f != y);

    value s("foo");
    BOOST_CHECK(s == value("foo"));
    BOOST_CHECK(s != value("bar"));

    blob blob = make_blob(10);
    value b1(blob);
    value b2(blob);
    BOOST_CHECK(b1 == b2);
    blob = make_blob(1);
    value b3(blob);
    BOOST_CHECK(b1 != b3);

    value_map r;
    r["x"] = x;
    BOOST_CHECK(get_field(r, "x") == x);

    value_map q;
    q["r"] = value(r);
    BOOST_CHECK(get_field(q, "r") == value(r));
    BOOST_CHECK(get_field(q, "r") != value(q));
    BOOST_CHECK(get_field(q, "r") != x);

    q.clear();
    q["y"] = x;
    BOOST_CHECK(q != r);

    q.clear();
    q["x"] = x;
    BOOST_CHECK(q == r);

    q.clear();
    q["x"] = y;
    BOOST_CHECK(q != r);

    value_list l;
    l.push_back(value(r));
    l.push_back(x);

    value_list m;
    BOOST_CHECK(l != m);
    m.push_back(value(r));
    BOOST_CHECK(l != m);
    m.push_back(x);
    BOOST_CHECK(l == m);
    m.push_back(y);
    BOOST_CHECK(l != m);
}
