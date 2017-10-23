#include <cradle/math/interpolate.hpp>
#include <cradle/anonymous.hpp>
#include <cradle/geometry/common.hpp>

#include <vector>

#define BOOST_TEST_MODULE :interpolate
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(compute_interpolation_grid_test0)
{
    std::vector<double> positions = anonymous<std::vector<double> >(
        0, 1, 3, 4, 5, 7, 8, 9);
    auto grid = compute_interpolation_grid(positions);
    CRADLE_CHECK_ALMOST_EQUAL(grid.p0[0], 0.);
    CRADLE_CHECK_ALMOST_EQUAL(grid.spacing[0], 1.);
    BOOST_CHECK_EQUAL(grid.n_points[0], 10);
}

BOOST_AUTO_TEST_CASE(compute_interpolation_grid_test1)
{
    std::vector<double> positions = anonymous<std::vector<double> >(
        0.2, 1.2, 3.2, 4.2, 5.2, 7.2, 8.2, 9.2);
    auto grid = compute_interpolation_grid(positions);
    CRADLE_CHECK_ALMOST_EQUAL(grid.p0[0], 0.2);
    CRADLE_CHECK_ALMOST_EQUAL(grid.spacing[0], 1.);
    BOOST_CHECK_EQUAL(grid.n_points[0], 10);
}

BOOST_AUTO_TEST_CASE(compute_interpolation_grid_test2)
{
    std::vector<double> positions = anonymous<std::vector<double> >(
        -4, -3, 1, 3, 4, 5, 7, 8, 9);
    auto grid = compute_interpolation_grid(positions);
    CRADLE_CHECK_ALMOST_EQUAL(grid.p0[0], -4.);
    CRADLE_CHECK_ALMOST_EQUAL(grid.spacing[0], 1.);
    BOOST_CHECK_EQUAL(grid.n_points[0], 14);
}

BOOST_AUTO_TEST_CASE(interpolate_test)
{
    std::vector<double> source_positions =
        anonymous<std::vector<double> >(0, 1, 3, 4, 5, 6.5, 8, 9);
    regular_grid<1,double> grid(make_vector(0.), make_vector(1.),
        make_vector(10u));

    std::vector<double> source_values =
        anonymous<std::vector<double> >(3, 0, 1, 2, 7, 0, -1, 2);
    std::vector<double> interpolated_values;

    interpolate(&interpolated_values, grid, source_values,
        source_positions);

    std::vector<double> correct_values =
        anonymous<std::vector<double> >(3, 0, 0.5, 1, 2, 7, 7. / 3,
            -1. / 3, -1, 2);

    CRADLE_CHECK_RANGES_ALMOST_EQUAL(interpolated_values, correct_values);
}
