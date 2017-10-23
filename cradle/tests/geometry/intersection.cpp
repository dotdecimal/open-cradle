#include <cradle/geometry/intersection.hpp>

#define BOOST_TEST_MODULE intersection
#include <cradle/test.hpp>

using namespace cradle;

template<typename T>
void assert_intersection(plane<T> const& plane,
    line_segment<3,T> const& segment, vector<3,T> const& correct_p)
{
    optional<vector<3,T> > p = intersection(plane, segment);
    BOOST_CHECK(p);
    CRADLE_CHECK_ALMOST_EQUAL(p.get(), correct_p);
}

template<typename T>
void assert_no_intersection(plane<T> const& plane,
    line_segment<3,T> const& segment)
{
    optional<vector<3,T> > p = intersection(plane, segment);
    BOOST_CHECK(!p);
}

BOOST_AUTO_TEST_CASE(plane_line_segment_intersection_test)
{
    assert_intersection(
        plane<double>(
            make_vector<double>(0, 1, 0), make_vector<double>(0, 1, 0)),
        line_segment<3,double>(
            make_vector<double>(0, 0, 0), make_vector<double>(0, 3, 0)),
        make_vector<double>(0, 1, 0));
    assert_intersection(
        plane<double>(
            make_vector<double>(0, 1, 0), make_vector<double>(0, 1, 0)),
        line_segment<3,double>(
            make_vector<double>(0, 0, 0), make_vector<double>(3, 3, 0)),
        make_vector<double>(1, 1, 0));
    assert_intersection(
        plane<double>(
            make_vector<double>(0, 1, 0), unit(make_vector<double>(1, 1, 0))),
        line_segment<3,double>(
            make_vector<double>(3, 2, 0), make_vector<double>(0, -1, 0)),
        make_vector<double>(1, 0, 0));
    assert_intersection(
        plane<double>(
            make_vector<double>(-1, 1, 0), make_vector<double>(0, 1, 0)),
        line_segment<3,double>(
            make_vector<double>(0, 0, 0), make_vector<double>(3, 3, 0)),
        make_vector<double>(1, 1, 0));
    assert_no_intersection(
        plane<double>(
            make_vector<double>(-1, 1, 0), make_vector<double>(0, 1, 0)),
        line_segment<3,double>(
            make_vector<double>(0, 0, 0), make_vector<double>(-3, -3, 0)));
}

template<typename T>
bool almost_equal(vector<3,T> const& a, vector<3,T> const& b)
{ return almost_equal<T>(length(a - b), 0); }

template<typename T>
void assert_intersection(plane<T> const& plane, triangle<3,T> const& tri,
    line_segment<3,T> const& correct_segment)
{
    optional<line_segment<3,T> > segment = intersection(plane, tri);
    BOOST_CHECK(segment);
    BOOST_CHECK(almost_equal(segment.get()[0], correct_segment[0]) &&
        almost_equal(segment.get()[1], correct_segment[1]) ||
        almost_equal(segment.get()[0], correct_segment[1]) &&
        almost_equal(segment.get()[1], correct_segment[0]));
}

template<typename T>
void assert_no_intersection(plane<T> const& plane, triangle<3,T> const& tri)
{
    optional<line_segment<3,T> > segment = intersection(plane, tri);
    BOOST_CHECK(!segment);
}

BOOST_AUTO_TEST_CASE(plane_triangle_intersection_test)
{
    assert_intersection(
        plane<double>(
            make_vector<double>(0, 1, 0), make_vector<double>(0, 1, 0)),
        triangle<3,double>(
            make_vector<double>(-2, 0, 0),
            make_vector<double>(2, 0, 0),
            make_vector<double>(0, 2, 0)),
        line_segment<3,double>(
            make_vector<double>(-1, 1, 0),
            make_vector<double>(1, 1, 0)));
}

template<unsigned N, typename T>
void assert_intersection(ray<N,T> const& ray, box<N,T> const& box,
    unsigned n_intersections, T entrance = 0, T exit = 0)
{
    auto result = intersection(ray, box);
    BOOST_CHECK_EQUAL(result.n_intersections, n_intersections);
    if (n_intersections > 0)
    {
        BOOST_CHECK_EQUAL(result.entrance_distance, entrance);
        BOOST_CHECK_EQUAL(result.exit_distance, exit);
    }
}

BOOST_AUTO_TEST_CASE(ray_box_intersection_test)
{
    assert_intersection(
        ray2d(make_vector<double>(-4, 0), make_vector<double>(1, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        2, 2., 6.);
    assert_intersection(
        ray2d(make_vector<double>(-4, 0), make_vector<double>(-1, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        0);
    assert_intersection(
        ray2d(make_vector<double>(0, 0), make_vector<double>(-1, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        1, 0., 2.);
    assert_intersection(
        ray2d(make_vector<double>(0, -4.5), unit(make_vector<double>(1, 1))),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        0);
    assert_intersection(
        ray2d(make_vector<double>(-4, -4), unit(make_vector<double>(1, 1))),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        2, 2 * sqrt(2.), 6 * sqrt(2.));
    assert_intersection(
        ray2d(make_vector<double>(-4, 0), unit(make_vector<double>(1, 1))),
        box2d(make_vector<double>(-2, 0), make_vector<double>(4, 6)),
        2, 2 * sqrt(2.), 6 * sqrt(2.));
}

template<unsigned N, typename T>
void assert_intersection(line_segment<N,T> const& segment,
    box<N,T> const& box,
    line_segment<N,T> const& correct_result)
{
    optional<line_segment<N,T> > result =
        intersection(segment, box);
    BOOST_CHECK(result);
    CRADLE_CHECK_ALMOST_EQUAL(correct_result[0], result.get()[0]);
    CRADLE_CHECK_ALMOST_EQUAL(correct_result[1], result.get()[1]);
}

template<unsigned N, typename T>
void assert_no_intersection(line_segment<N,T> const& segment,
    box<N,T> const& box)
{
    optional<line_segment<N,T> > result =
        intersection(segment, box);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(segment_box_intersection)
{
    assert_intersection(
        line_segment<2,double>(
            make_vector<double>(-4, 0), make_vector<double>(4, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        line_segment<2,double>(
            make_vector<double>(-2, 0), make_vector<double>(2, 0)));
    assert_no_intersection(
        line_segment<2,double>(
            make_vector<double>(-4, 0), make_vector<double>(-12, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)));
    assert_intersection(
        line_segment<2,double>(
            make_vector<double>(0, 0), make_vector<double>(-4, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        line_segment<2,double>(
            make_vector<double>(0, 0), make_vector<double>(-2, 0)));
    assert_no_intersection(
        line_segment<2,double>(
            make_vector<double>(0, -4.5), make_vector<double>(-4.5, 0)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)));
    assert_intersection(
        line_segment<2,double>(
            make_vector<double>(-4, -4), make_vector<double>(4, 4)),
        box2d(make_vector<double>(-2, -2), make_vector<double>(4, 4)),
        line_segment<2,double>(
            make_vector<double>(-2, -2), make_vector<double>(2, 2)));
    assert_intersection(
        line_segment<2,double>(
            make_vector<double>(-4, 0), make_vector<double>(4, 8)),
        box2d(make_vector<double>(-2, 0), make_vector<double>(4, 6)),
        line_segment<2,double>(
            make_vector<double>(-2, 2), make_vector<double>(2, 6)));
    assert_intersection(
        line_segment<2,double>(
            make_vector<double>(-4, 0), make_vector<double>(0, 4)),
        box2d(make_vector<double>(-2, 0), make_vector<double>(4, 6)),
        line_segment<2,double>(
            make_vector<double>(-2, 2), make_vector<double>(0, 4)));
}
