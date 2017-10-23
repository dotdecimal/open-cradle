#include <cradle/math/common.hpp>
#include <cradle/anonymous.hpp>

#define BOOST_TEST_MODULE math
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(compute_mean_integer_test)
{
    BOOST_CHECK_EQUAL(
        compute_mean<float>(
            anonymous<std::vector<int> >(4, 2, 7, 3), 0),
        4);
}

BOOST_AUTO_TEST_CASE(compute_mean_float_test)
{
    BOOST_CHECK_EQUAL(
        compute_mean<float>(
            anonymous<std::vector<float> >(4, 2, 1, 3), 0),
        2.5);
}

BOOST_AUTO_TEST_CASE(clamp_test)
{
    BOOST_CHECK_EQUAL(cradle::clamp(-0.5, 0., 1.), 0);
    BOOST_CHECK_EQUAL(cradle::clamp( 0.5, 0., 1.), 0.5);
    BOOST_CHECK_EQUAL(cradle::clamp( 1.5, 0., 1.), 1);
}

BOOST_AUTO_TEST_CASE(equality_test)
{
    BOOST_CHECK(almost_equal(1., 1.01, 0.1));
    BOOST_CHECK(!almost_equal(1., 1.2, 0.1));

    BOOST_CHECK(almost_equal(1.,
        1 + default_equality_tolerance<double>() / 2));
    BOOST_CHECK(!almost_equal(1.,
        1 + default_equality_tolerance<double>() * 2));
    BOOST_CHECK(!almost_equal(1., 2.));

    BOOST_CHECK(almost_equal(1.f,
        1 + default_equality_tolerance<float>() / 2));
    BOOST_CHECK(!almost_equal(1.f,
        1 + default_equality_tolerance<float>() * 2));
}

BOOST_AUTO_TEST_CASE(is_power_of_two_test)
{
    BOOST_CHECK(!is_power_of_two( 0));
    BOOST_CHECK( is_power_of_two( 1));
    BOOST_CHECK( is_power_of_two( 2));
    BOOST_CHECK(!is_power_of_two( 3));
    BOOST_CHECK( is_power_of_two( 4));
    BOOST_CHECK(!is_power_of_two( 5));
    BOOST_CHECK(!is_power_of_two( 6));
    BOOST_CHECK(!is_power_of_two( 7));
    BOOST_CHECK( is_power_of_two( 8));
    BOOST_CHECK(!is_power_of_two( 9));
    BOOST_CHECK(!is_power_of_two(10));
    BOOST_CHECK(!is_power_of_two(15));
    BOOST_CHECK( is_power_of_two(16));
    BOOST_CHECK( is_power_of_two(64));
    BOOST_CHECK(!is_power_of_two(65));
}

BOOST_AUTO_TEST_CASE(mod_test)
{
    BOOST_CHECK_EQUAL(nonnegative_mod(-3, 4), 1);
    BOOST_CHECK_EQUAL(nonnegative_mod(11, 4), 3);
    BOOST_CHECK_EQUAL(nonnegative_mod(12, 4), 0);
    BOOST_CHECK_EQUAL(nonnegative_mod(2, 4), 2);
    BOOST_CHECK_EQUAL(nonnegative_mod(-1, 12), 11);
    BOOST_CHECK_EQUAL(nonnegative_mod(-13, 12), 11);
    BOOST_CHECK_EQUAL(nonnegative_mod(-12, 12), 0);
    BOOST_CHECK_EQUAL(nonnegative_mod(0, 12), 0);
    BOOST_CHECK_EQUAL(nonnegative_mod(4, 12), 4);
    BOOST_CHECK_EQUAL(nonnegative_mod(13, 12), 1);
}

BOOST_AUTO_TEST_CASE(quadratic_function_test)
{
    quadratic_function<double> f(1, 3, 2);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, -2), 0.);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 1.5), 8.75);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 0), 2.);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 4), 30.);

    quadratic_function<double> g(2, 3, 0);
    BOOST_CHECK(f != g);

    BOOST_CHECK_EQUAL(f, f);

    // This is disabled because the prettier output was disabled when all
    // CRADLE types got automatically generated ostream operators.
    //BOOST_CHECK_EQUAL(to_string(g), "f(x) = 2x^2 + 3x + 0");
}

BOOST_AUTO_TEST_CASE(linear_function_test)
{
    linear_function<double> f(1, 3);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, -2), -5.);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 1.5), 5.5);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 2), 7.);

    f = inverse(f);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, -5), -2.);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 5.5), 1.5);
    CRADLE_CHECK_ALMOST_EQUAL(apply(f, 7), 2.);

    linear_function<double> g(2, 3);
    BOOST_CHECK(f != g);

    BOOST_CHECK_EQUAL(f, f);

    // This is disabled because the prettier output was disabled when all
    // CRADLE types got automatically generated ostream operators.
    //BOOST_CHECK_EQUAL(to_string(g), "f(x) = 3x + 2");
}

BOOST_AUTO_TEST_CASE(simple_interpolated_function_test)
{
    std::vector<double> samples = anonymous<std::vector<double> >(
        4, 2, 1, 7, 6, 4);
    interpolated_function f;
    initialize(f, 1, 4, samples, outside_domain_policy::ALWAYS_ZERO);

    BOOST_CHECK_EQUAL(get_sample_grid(f).p0[0], 1);
    BOOST_CHECK_EQUAL(get_sample_grid(f).spacing[0], 4);
    BOOST_CHECK_EQUAL(get_sample_grid(f).n_points[0], samples.size());

    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -1  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  0  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -0.1), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1  ), 4.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1.5), 3.75);
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  2  ), 3.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  5  ), 2.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 15  ), 6.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 20  ), 4.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 21  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 23  ), 0.  );
}

BOOST_AUTO_TEST_CASE(regularly_sampled_function_test)
{
    regularly_sampled_function data_set(
        1, 4,
        anonymous<std::vector<double> >(4, 2, 1, 7, 6, 4),
        outside_domain_policy::ALWAYS_ZERO);

    interpolated_function f;
    initialize(f, data_set);

    BOOST_CHECK_EQUAL(get_sample_grid(f).p0[0], 1);
    BOOST_CHECK_EQUAL(get_sample_grid(f).spacing[0], 4);
    BOOST_CHECK_EQUAL(get_sample_grid(f).n_points[0], 6);

    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -1  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  0  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -0.1), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1  ), 4.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1.5), 3.75);
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  2  ), 3.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  5  ), 2.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 15  ), 6.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 20  ), 4.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 21  ), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 23  ), 0.  );
}

BOOST_AUTO_TEST_CASE(irregularly_sampled_function_test)
{
    irregularly_sampled_function data_set(
        anonymous<std::vector<alia::vector<2,double> > >(
            alia::make_vector<double>(0, 4),
            alia::make_vector<double>(1, 2),
            alia::make_vector<double>(3, 1),
            alia::make_vector<double>(4, 3),
            alia::make_vector<double>(5, 3),
            alia::make_vector<double>(7, 1),
            alia::make_vector<double>(8, 1),
            alia::make_vector<double>(9, 2)),
        outside_domain_policy::ALWAYS_ZERO);

    interpolated_function f;
    initialize(f, data_set);

    CRADLE_CHECK_ALMOST_EQUAL(get_sample_grid(f).p0[0], 0.);
    CRADLE_CHECK_ALMOST_EQUAL(get_sample_grid(f).spacing[0], 1.);
    BOOST_CHECK_EQUAL(get_sample_grid(f).n_points[0], 10);

    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -0.5), 0.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  0  ), 4.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  0.5), 3.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1  ), 2.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1.5), 1.75);
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  2  ), 1.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  3.5), 2.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  6  ), 2.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  7  ), 1.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  9.1), 0.  );
}

BOOST_AUTO_TEST_CASE(interpolated_function_extend_with_copies_test)
{
    std::vector<double> samples = anonymous<std::vector<double> >(
        6, 2, 1, 7, 6, 4);
    interpolated_function f;
    initialize(f, 1, 4, samples, outside_domain_policy::EXTEND_WITH_COPIES);

    BOOST_CHECK_EQUAL(get_sample_grid(f).p0[0], 1);
    BOOST_CHECK_EQUAL(get_sample_grid(f).spacing[0], 4);
    BOOST_CHECK_EQUAL(get_sample_grid(f).n_points[0], samples.size());

    CRADLE_CHECK_ALMOST_EQUAL(sample(f, -1  ), 6.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  0  ), 6.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f,  1  ), 6.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 20  ), 4.5 );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 22  ), 4.  );
    CRADLE_CHECK_ALMOST_EQUAL(sample(f, 23  ), 4.  );
}
