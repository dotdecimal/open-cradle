#include <cradle/imaging/integral.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/foreach.hpp>

#define BOOST_TEST_MODULE integral
#include <cradle/test.hpp>

using namespace cradle;

template<unsigned N, class View>
void check_segment_integral(View const& view,
    vector<N,double> const& p1,
    vector<N,double> const& p2,
    double correct_value,
    double tolerance = std::numeric_limits<double>::epsilon() * 100,
    bool test_inverse = true)
{
    CRADLE_CHECK_WITHIN_TOLERANCE(
        compute_image_integral_over_line_segment(
            view, line_segment<N,double>(p1, p2)),
        correct_value, tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        compute_image_integral_over_line_segment(
            view, line_segment<N,double>(p2, p1)),
        correct_value, tolerance);

    if (test_inverse)
    {
        CRADLE_CHECK_WITHIN_TOLERANCE(
            compute_inverse_image_integral_over_ray(view,
                ray<N,double>(p1, unit(p2 - p1)),
                correct_value),
            length(p2 - p1), tolerance);
        CRADLE_CHECK_WITHIN_TOLERANCE(
            compute_inverse_image_integral_over_ray(view,
                ray<N,double>(p2, unit(p1 - p2)),
                correct_value),
            length(p1 - p2), tolerance);
    }
}

struct filler_fn
{
    filler_fn() : n(0) {}
    int n;
    void operator()(cradle::uint8_t& x)
    { x = n++ / 10; }
};

BOOST_AUTO_TEST_CASE(over_line_segment_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));
    set_spatial_mapping(
        img, make_vector<double>(-2, -6), make_vector<double>(1, 2));
    img.value_mapping = linear_function<double>(1, 2);

    filler_fn fn;
    foreach_pixel(img, fn);

    double tolerance = 0.001;

    check_segment_integral(img, make_vector<double>( 0.1,  -5),
        make_vector<double>( 0.2,  -5),  0.1, tolerance, true);
    check_segment_integral(img, make_vector<double>(-0.2,  -5),
        make_vector<double>( 6.9,  -5),  7.1, tolerance, true);
    check_segment_integral(img, make_vector<double>(  -3,  -5),
        make_vector<double>(   9,  -5),   10, tolerance, false);
    check_segment_integral(img, make_vector<double>(  -3,  -5),
        make_vector<double>(   7,  -5),    9, tolerance, false);
    check_segment_integral(img, make_vector<double>(   0,  -5),
        make_vector<double>(   7,  -5),    7, tolerance, true);

    check_segment_integral(img, make_vector<double>(   0,  15),
        make_vector<double>(   0,  -7),  200, tolerance, false);

    // Now repeat all the tests on a variant view...

    image<2,variant,const_view> v =
        as_variant(as_const_view(img));

    check_segment_integral(v, make_vector<double>( 0.1,  -5),
        make_vector<double>( 0.2,  -5),  0.1, tolerance, true);
    check_segment_integral(v, make_vector<double>(-0.2,  -5),
        make_vector<double>( 6.9,  -5),  7.1, tolerance, true);
    check_segment_integral(v, make_vector<double>(  -3,  -5),
        make_vector<double>(   9,  -5),   10, tolerance, false);
    check_segment_integral(v, make_vector<double>(  -3,  -5),
        make_vector<double>(   7,  -5),    9, tolerance, false);
    check_segment_integral(v, make_vector<double>(   0,  -5),
        make_vector<double>(   7,  -5),    7, tolerance, true);

    check_segment_integral(v, make_vector<double>(   0,  15),
        make_vector<double>(   0,  -7),  200, tolerance, false);
}

template<unsigned N, class View>
void check_ray_integral(View const& view,
    vector<N,double> const& origin,
    vector<N,double> const& direction,
    double correct_value,
    double tolerance = std::numeric_limits<double>::epsilon() * 100)
{
    CRADLE_CHECK_WITHIN_TOLERANCE(
        compute_image_integral_over_ray(
            view, ray<N,double>(origin, unit(direction))),
        correct_value, tolerance);
}

BOOST_AUTO_TEST_CASE(over_ray_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));
    set_spatial_mapping(
        img, make_vector<double>(-2, -6), make_vector<double>(1, 2));
    img.value_mapping = linear_function<double>(1, 2);

    filler_fn fn;
    foreach_pixel(img, fn);

    double tolerance = 0.001;

    check_ray_integral(img, make_vector<double>(  -2,  -5),
        make_vector<double>(   1,   0),   10, tolerance);
    check_ray_integral(img, make_vector<double>(   0,  -5),
        make_vector<double>(   1,   0),    8, tolerance);
    check_ray_integral(img, make_vector<double>(-0.5,  -4),
        make_vector<double>(   0,   1),  198, tolerance);

    // Now repeat all the tests on a variant view...

    image<2,variant,const_view> v =
        as_variant(as_const_view(img));

    check_ray_integral(v, make_vector<double>(  -2,  -5),
        make_vector<double>(   1,   0),   10, tolerance);
    check_ray_integral(v, make_vector<double>(   0,  -5),
        make_vector<double>(   1,   0),    8, tolerance);
    check_ray_integral(v, make_vector<double>(-0.5,  -4),
        make_vector<double>(   0,   1),  198, tolerance);
}

BOOST_AUTO_TEST_CASE(computer_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));
    set_spatial_mapping(
        img, make_vector<double>(-2, -6), make_vector<double>(1, 2));
    img.value_mapping = linear_function<double>(1, 2);

    filler_fn fn;
    foreach_pixel(img, fn);

    double tolerance = 0.001;

    image_integral_computer<2,cradle::uint8_t,unique> computer(
        img, ray2d(make_vector<double>(-3, -1), make_vector<double>(1, 0)));

    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(9), 40., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(9), 40., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(20), 50., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(20), 50., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(7), 30., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(make_vector<double>(7, -1)), 45.,
        tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_integral_to(1), 0., tolerance);

    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(40), 9., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(40), 9., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(45), 10., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(45), 10., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(40), 9., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(45), 10., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(
        computer.compute_distance_to(30), 7., tolerance);
}
