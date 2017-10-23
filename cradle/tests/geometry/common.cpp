#include <cradle/geometry/common.hpp>
#include <cradle/common.hpp>
#include <vector>
#include <boost/assign/std/vector.hpp>
#include <cradle/math/common.hpp>
#include <cradle/geometry/transformations.hpp>

#define BOOST_TEST_MODULE geometry
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(sizeof_vector)
{
    // Check that vectors don't have any size overhead.
    BOOST_CHECK_EQUAL(sizeof(vector<3,int>), 3 * sizeof(int));
    BOOST_CHECK_EQUAL(sizeof(vector<1,float>), sizeof(float));
    BOOST_CHECK_EQUAL(sizeof(vector<2,double>), 2 * sizeof(double));
}

void f(vector3i const& v) {}

// confirm that the vector operators work as expected
BOOST_AUTO_TEST_CASE(vector_operators)
{
    vector<3,int> p = make_vector<int>(1, 1, 0),
        q = make_vector<int>(4, 2, 6);
    vector<3,int> v = make_vector<int>(3, 1, 2);

    BOOST_CHECK_EQUAL(p - q, make_vector<int>(-3, -1, -6));
    BOOST_CHECK_EQUAL(p + v, make_vector<int>(4, 2, 2));

    BOOST_CHECK_EQUAL(v * 3, make_vector<int>(9, 3, 6));
    BOOST_CHECK_EQUAL(v / 2, make_vector<int>(1, 0, 1));

    BOOST_CHECK(!(p == q));
    BOOST_CHECK(p != q);

    BOOST_CHECK(!(p - q == v));
    BOOST_CHECK(p - q != v);
}

BOOST_AUTO_TEST_CASE(vector_slice)
{
    {
    vector3i p = make_vector<int>(6, 7, 8);
    BOOST_CHECK_EQUAL(slice(p, 0), make_vector<int>(7, 8));
    BOOST_CHECK_EQUAL(slice(p, 1), make_vector<int>(6, 8));
    BOOST_CHECK_EQUAL(slice(p, 2), make_vector<int>(6, 7));
    }

    {
    vector2f p = make_vector<float>(9, 17);
    BOOST_CHECK_EQUAL(slice(p, 0), make_vector<float>(17));
    BOOST_CHECK_EQUAL(slice(p, 1), make_vector<float>(9));
    }

    {
    vector4i p = make_vector<int>(4, 3, 2, 1);
    BOOST_CHECK_EQUAL(slice(p, 0), make_vector<int>(3, 2, 1));
    BOOST_CHECK_EQUAL(slice(p, 1), make_vector<int>(4, 2, 1));
    BOOST_CHECK_EQUAL(slice(p, 2), make_vector<int>(4, 3, 1));
    BOOST_CHECK_EQUAL(slice(p, 3), make_vector<int>(4, 3, 2));
    }
}

BOOST_AUTO_TEST_CASE(test_unslice_vector)
{
    {
    vector3i p = make_vector<int>(6, 7, 8);
    BOOST_CHECK_EQUAL(unslice(p, 0, 0), make_vector<int>(0, 6, 7, 8));
    BOOST_CHECK_EQUAL(unslice(p, 1, 0), make_vector<int>(6, 0, 7, 8));
    BOOST_CHECK_EQUAL(unslice(p, 2, 0), make_vector<int>(6, 7, 0, 8));
    BOOST_CHECK_EQUAL(unslice(p, 3, 0), make_vector<int>(6, 7, 8, 0));
    }

    {
    vector2d p = make_vector<double>(9, 17);
    BOOST_CHECK_EQUAL(unslice(p, 0, 2.1), make_vector<double>(2.1, 9, 17));
    BOOST_CHECK_EQUAL(unslice(p, 1, 2.1), make_vector<double>(9, 2.1, 17));
    BOOST_CHECK_EQUAL(unslice(p, 2, 2.1), make_vector<double>(9, 17, 2.1));
    }
}

BOOST_AUTO_TEST_CASE(test_uniform_vector)
{
    BOOST_CHECK_EQUAL((uniform_vector<3,int>(0)), make_vector<int>(0, 0, 0));
    BOOST_CHECK_EQUAL((uniform_vector<4,unsigned>(1)),
        make_vector<unsigned>(1, 1, 1, 1));
    BOOST_CHECK_EQUAL((uniform_vector<2,float>(6)), make_vector<float>(6, 6));
}

BOOST_AUTO_TEST_CASE(vector_almost_equal)
{
    BOOST_CHECK(almost_equal(
        make_vector<double>(0, 0, 0), make_vector<double>(0, 0,
        default_equality_tolerance<double>() / 2)));
    BOOST_CHECK(!almost_equal(
        make_vector<float>(0, 0, 0), make_vector<float>(0, 0, 1)));
    BOOST_CHECK(almost_equal(
        make_vector<float>(0, 0, 0), make_vector<float>(0, 0, 1), 2));
}

BOOST_AUTO_TEST_CASE(vector_cross)
{
    // with objects
    BOOST_CHECK(
        almost_equal(
            cross(make_vector<double>(1, 0, 0), make_vector<double>(0, 1, 0)),
            make_vector<double>(0, 0, 1)));
    BOOST_CHECK(
        almost_equal(
            cross(make_vector<double>(0, 1, 0), make_vector<double>(1, 0, 0)),
            make_vector<double>(0, 0, -1)));

    // with expressions
    BOOST_CHECK(
        almost_equal(
            cross(
                make_vector<double>(1, 0, 0) - make_vector<double>(0, 0, 0),
                make_vector<double>(0, 1, 0) - make_vector<double>(0, 0, 0)),
            make_vector<double>(0, 0, 1)));

    // mixing expressions and objects
    BOOST_CHECK(
        almost_equal(
            cross(
                make_vector<double>(1, 0, 0) - make_vector<double>(0, 0, 0),
                make_vector<double>(0, 1, 0)),
            make_vector<double>(0, 0, 1)));
    BOOST_CHECK(
        almost_equal(
            cross(
                make_vector<double>(1, 0, 0),
                make_vector<double>(0, 1, 0) - make_vector<double>(0, 0, 0)),
            make_vector<double>(0, 0, 1)));
}

BOOST_AUTO_TEST_CASE(vector_dot)
{
    // with objects
    BOOST_CHECK(almost_equal(dot(make_vector<double>(1, 1),
        make_vector<double>(0.7, 0.3)), 1.));
    BOOST_CHECK(almost_equal(dot(make_vector<double>(1, 0, 0),
        make_vector<double>(0, 1, 0)), 0.));
    BOOST_CHECK(almost_equal(dot(make_vector<double>(1),
        make_vector<double>(0.6)), 0.6));
    BOOST_CHECK_EQUAL(dot(make_vector<int>(1, 2, 0),
        make_vector<int>(2, 3, 0)), 8);

    // with expressions
    BOOST_CHECK(almost_equal(dot(make_vector<double>(1, 0, 1) -
        make_vector<double>(0, 0, 0), make_vector<double>(0.7, 0, 0.3) -
        make_vector<double>(0, 0, 0)), 1.));

    // mixing expressions and objects
    BOOST_CHECK(almost_equal(dot(make_vector<float>(1, 1),
        make_vector<float>(0.7f,  0.3f) - make_vector<float>(0, 0)), 1.f));
    BOOST_CHECK(almost_equal(dot(make_vector<double>(1, 0, 1) -
        make_vector<double>(0, 0, 0), make_vector<double>(0.7, 0, 0.3)), 1.));
}

BOOST_AUTO_TEST_CASE(vector_length)
{
    // length2()
    BOOST_CHECK_EQUAL(length2(make_vector<int>(2, 0, 1)), 5);

    // length()
    BOOST_CHECK(almost_equal(
        length(make_vector<double>(2, 1)), sqrt(5.)));

    // length() with an expression
    BOOST_CHECK(almost_equal(
        length(make_vector<double>(2, 0, 1) - make_vector<double>(1, 0, 0)),
        sqrt(2.)));
}

BOOST_AUTO_TEST_CASE(unit_vector)
{
    BOOST_CHECK(almost_equal(
        unit(make_vector<double>(4, 0, 3)), make_vector<double>(0.8, 0, 0.6)));

    // with an expression
    BOOST_CHECK(almost_equal(
        unit(make_vector<double>(3, 0) - make_vector<double>(0, 4)),
        make_vector<double>(0.6, -0.8)));
}

BOOST_AUTO_TEST_CASE(perpendicular_vector)
{
    for (int x = -1; x < 2; ++x)
    {
        for (int y = -1; y < 2; ++y)
        {
            for (int z = -1; z < 2; ++z)
            {
                if (x != 0 || y != 0 || z != 0)
                {
                    vector<3,double> v = make_vector<double>(x, y, z);
                    CRADLE_CHECK_ALMOST_EQUAL(dot(v, get_perpendicular(v)),
                        0.);
                    CRADLE_CHECK_ALMOST_EQUAL(length(get_perpendicular(v)),
                        1.);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(product_test)
{
    BOOST_CHECK_EQUAL(product(make_vector<int>(2, 3, 1)), 6);
    BOOST_CHECK_EQUAL(product(make_vector<int>(2, -1, 3, 1)), -6);

    BOOST_CHECK(almost_equal(
        product(make_vector<float>(2.5, 4, 2)), 20.f));
    BOOST_CHECK(almost_equal(
        product(make_vector<double>(2.5, 4)), 10.));

    // with an expression
    BOOST_CHECK_EQUAL(
        product(make_vector<int>(2, -1, 3, 0) - make_vector<int>(6, 0, 0, 1)),
        -12);
}

BOOST_AUTO_TEST_CASE(vector_io)
{
    vector3i p = make_vector<int>(2, 0, 3);
    BOOST_CHECK_EQUAL(to_string(p), "(2, 0, 3)");
}

BOOST_AUTO_TEST_CASE(compute_mean_vector_test)
{
    using namespace boost::assign;

    std::vector<vector3d> vectors;
    vectors +=
        make_vector<double>(2, 0, 3),
        make_vector<double>(6, 1, 7),
        make_vector<double>(0, 0, 0),
        make_vector<double>(1, 2, 0),
        make_vector<double>(3, 2, 1),
        make_vector<double>(6, 4, 1);

    BOOST_CHECK(almost_equal(
        compute_mean(vectors, uniform_vector<3>(0.)),
        make_vector<double>(3, 1.5, 2)));
}

BOOST_AUTO_TEST_CASE(plane_test)
{
    plane<double> default_constructed;

    vector3d p = make_vector<double>(0, 0, 0);
    vector3d normal = make_vector<double>(1, 0, 0);

    plane<double> plane(p, normal);
    BOOST_CHECK_EQUAL(plane.get_point(), p);
    BOOST_CHECK_EQUAL(plane.get_normal(), normal);

    vector3d q = make_vector<double>(0, 0, 1);
    plane.set_point(q);
    BOOST_CHECK_EQUAL(plane.get_point(), q);
    BOOST_CHECK_EQUAL(plane.get_normal(), normal);

    normal = make_vector<double>(0, 1, 0);
    plane.set_normal(normal);
    BOOST_CHECK_EQUAL(plane.get_point(), q);
    BOOST_CHECK_EQUAL(plane.get_normal(), normal);
}

BOOST_AUTO_TEST_CASE(simple_box1i_test)
{
    box1i b(make_vector<int>(-1), make_vector<int>(4));

    BOOST_CHECK_EQUAL(get_center(b)[0], 1);
    BOOST_CHECK_EQUAL(b.corner[0], -1);
    BOOST_CHECK_EQUAL(b.size[0], 4);

    BOOST_CHECK(!is_inside(b, make_vector<int>(-2)));
    BOOST_CHECK(is_inside(b, make_vector<int>(-1)));
    BOOST_CHECK(is_inside(b, make_vector<int>(2)));
    BOOST_CHECK(!is_inside(b, make_vector<int>(3)));
    BOOST_CHECK(!is_inside(b, make_vector<int>(4)));
}

BOOST_AUTO_TEST_CASE(simple_box1d_test)
{
    box1d b(make_vector<double>(-1), make_vector<double>(3));

    CRADLE_CHECK_ALMOST_EQUAL(get_center(b), make_vector<double>(0.5));
    CRADLE_CHECK_ALMOST_EQUAL(b.corner, make_vector<double>(-1));
    CRADLE_CHECK_ALMOST_EQUAL(b.size, make_vector<double>(3));

    BOOST_CHECK(!is_inside(b, make_vector<double>(-2)));
    BOOST_CHECK(is_inside(b, make_vector<double>(-1)));
    BOOST_CHECK(is_inside(b, make_vector<double>(0)));
    BOOST_CHECK(is_inside(b, make_vector<double>(1)));
    BOOST_CHECK(is_inside(b, make_vector<double>(1.5)));
    BOOST_CHECK(is_inside(b, make_vector<double>(1.9)));
    BOOST_CHECK(!is_inside(b, make_vector<double>(2)));
    BOOST_CHECK(!is_inside(b, make_vector<double>(4)));
}

BOOST_AUTO_TEST_CASE(simple_box2d_test)
{
    box2d b(make_vector<double>(-1, -1), make_vector<double>(3, 3));

    CRADLE_CHECK_ALMOST_EQUAL(area(b), 9.);

    CRADLE_CHECK_ALMOST_EQUAL(get_center(b), make_vector<double>(0.5, 0.5));
    CRADLE_CHECK_ALMOST_EQUAL(b.corner, make_vector<double>(-1, -1));
    CRADLE_CHECK_ALMOST_EQUAL(b.size, make_vector<double>(3, 3));

    BOOST_CHECK(!is_inside(b, make_vector<double>(-2, -2)));
    BOOST_CHECK(!is_inside(b, make_vector<double>(-2, 0)));
    BOOST_CHECK(!is_inside(b, make_vector<double>(0, 4)));
    BOOST_CHECK(!is_inside(b, make_vector<double>(0, 2)));
    BOOST_CHECK(is_inside(b, make_vector<double>(-1, -1)));
    BOOST_CHECK(is_inside(b, make_vector<double>(0, 1.9)));
    BOOST_CHECK(is_inside(b, make_vector<double>(0, 0)));
    BOOST_CHECK(is_inside(b, make_vector<double>(1.5, 1.5)));
    BOOST_CHECK(is_inside(b, make_vector<double>(0, 1)));
}

BOOST_AUTO_TEST_CASE(box_slicing_test)
{
    BOOST_CHECK_EQUAL(
        slice(box3d(make_vector<double>(0, 2, 1),
            make_vector<double>(4, 3, 5)), 0),
        box2d(make_vector<double>(2, 1), make_vector<double>(3, 5)));
    BOOST_CHECK_EQUAL(
        slice(box3d(make_vector<double>(0, 2, 1),
            make_vector<double>(4, 3, 5)), 1),
        box2d(make_vector<double>(0, 1), make_vector<double>(4, 5)));
    BOOST_CHECK_EQUAL(
        slice(box3d(make_vector<double>(0, 2, 1),
            make_vector<double>(4, 3, 5)), 2),
        box2d(make_vector<double>(0, 2), make_vector<double>(4, 3)));

    BOOST_CHECK_EQUAL(
        slice(box2i(make_vector<int>(0, 2), make_vector<int>(4, 3)), 0),
            box1i(make_vector<int>(2), make_vector<int>(3)));
    BOOST_CHECK_EQUAL(
        slice(box2i(make_vector<int>(0, 2), make_vector<int>(4, 3)), 1),
            box1i(make_vector<int>(0), make_vector<int>(4)));
}

BOOST_AUTO_TEST_CASE(add_box_border_test)
{
    BOOST_CHECK_EQUAL(
        add_border(box3i(make_vector<int>(0, 2, 1),
            make_vector<int>(4, 3, 5)), 2),
        box3i(make_vector<int>(-2, 0, -1), make_vector<int>(8, 7, 9)));

    BOOST_CHECK_EQUAL(
        add_border(box3i(make_vector<int>(0, 2, 1), make_vector<int>(4, 3, 5)),
            make_vector<int>(2, 1, 0)),
        box3i(make_vector<int>(-2, 1, 1), make_vector<int>(8, 5, 5)));
}

BOOST_AUTO_TEST_CASE(simple_test)
{
    circle<double> c(make_vector<double>(0, 0), 1);
    CRADLE_CHECK_ALMOST_EQUAL(area(c), pi);
    BOOST_CHECK(!is_inside(c, make_vector<double>(0, 2)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(2, 0)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(1.1, 0)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(0.9, 0.9)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(0, -1.1)));
    BOOST_CHECK(is_inside(c, make_vector<double>(0.9, 0)));
    BOOST_CHECK(is_inside(c, make_vector<double>(0, 0)));
    BOOST_CHECK(is_inside(c, make_vector<double>(0, -0.9)));
    BOOST_CHECK(is_inside(c, make_vector<double>(-0.7, 0.7)));
    BOOST_CHECK(is_inside(c, make_vector<double>(0.3, 0.5)));
}

BOOST_AUTO_TEST_CASE(off_center_test)
{
    circle<double> c(make_vector<double>(4, 1), 2);
    CRADLE_CHECK_ALMOST_EQUAL(area(c), 4 * pi);
    BOOST_CHECK(!is_inside(c, make_vector<double>(0, 0)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(1.9, 1)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(6.1, 1)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(4, 3.1)));
    BOOST_CHECK(!is_inside(c, make_vector<double>(4, -1.1)));
    BOOST_CHECK(is_inside(c, make_vector<double>(4, 1)));
    BOOST_CHECK(is_inside(c, make_vector<double>(2.6, 2.4)));
}

BOOST_AUTO_TEST_CASE(by_value_test)
{
    vector2d p0 = make_vector<double>(0, 1), p1 = make_vector<double>(4, 4);
    line_segment<2,double> segment(p0, p1);

    BOOST_CHECK_EQUAL(segment[0], p0);
    BOOST_CHECK_EQUAL(segment[1], p1);
    CRADLE_CHECK_ALMOST_EQUAL(length(segment), 5.);
}

BOOST_AUTO_TEST_CASE(identity_matrix_test)
{
    BOOST_CHECK_EQUAL(
        (identity_matrix<4,double>()),
        make_matrix<double>(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1));

    BOOST_CHECK_EQUAL(
        (identity_matrix<3,double>()),
        make_matrix<double>(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1));
}

BOOST_AUTO_TEST_CASE(matrix_operations_test)
{
    matrix<3,3,double> m, i = identity_matrix<3,double>();

    m = i - 2 * i;
    BOOST_CHECK_EQUAL(
        m,
        make_matrix<double>(
            -1, 0, 0,
            0, -1, 0,
            0, 0, -1));

    m *= 2;
    BOOST_CHECK_EQUAL(
        m,
        make_matrix<double>(
            -2, 0, 0,
            0, -2, 0,
            0, 0, -2));

    m = i;
    m /= 2;
    BOOST_CHECK_EQUAL(
        m,
        make_matrix<double>(
            0.5, 0, 0,
            0, 0.5, 0,
            0, 0, 0.5));

    m += i * 3;
    BOOST_CHECK_EQUAL(
        m,
        make_matrix<double>(
            3.5, 0, 0,
            0, 3.5, 0,
            0, 0, 3.5));

    m -= 2 * i;
    BOOST_CHECK_EQUAL(
        m,
        make_matrix<double>(
            1.5, 0, 0,
            0, 1.5, 0,
            0, 0, 1.5));

    BOOST_CHECK(m == m);
    BOOST_CHECK(m != i);
}

BOOST_AUTO_TEST_CASE(matrix_conversion_test)
{
    matrix<3,3,double> m(identity_matrix<3,float>());
}

BOOST_AUTO_TEST_CASE(matrix_inverse3_test)
{
    matrix<4,4,double> m =
        translation(make_vector<double>(4, 3, 7)) *
        scaling_transformation(make_vector<double>(.1, 2, 1.2)) *
        rotation_about_x(angle<double,degrees>(90));

    matrix<4,4,double> inv_m = inverse(m);

    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m,
            transform_point(m, make_vector<double>(0, 0, 0))),
        make_vector<double>(0, 0, 0));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m,
            transform_point(m, make_vector<double>(2, 1, 7))),
        make_vector<double>(2, 1, 7));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m,
            transform_point(m, make_vector<double>(1, 0, 17))),
        make_vector<double>(1, 0, 17));
}

BOOST_AUTO_TEST_CASE(matrix_inverse2_test)
{
    matrix<3,3,double> m =
        translation(make_vector<double>(3, 7)) *
        scaling_transformation(make_vector<double>(.1, 1.2)) *
        rotation(angle<double,degrees>(90));

    matrix<3,3,double> inv_m = inverse(m);

    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(0, 0))),
        make_vector<double>(0, 0));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(1, 7))),
        make_vector<double>(1, 7));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(0, 17))),
        make_vector<double>(0, 17));
}

BOOST_AUTO_TEST_CASE(matrix_inverse1_test)
{
    matrix<2,2,double> m =
        translation(make_vector<double>(1)) *
        scaling_transformation(make_vector<double>(.1));

    matrix<2,2,double> inv_m = inverse(m);

    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(0))),
        make_vector<double>(0));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(7))),
        make_vector<double>(7));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(17))),
        make_vector<double>(17));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(inv_m, transform_point(m, make_vector<double>(1))),
        make_vector<double>(1));
}
