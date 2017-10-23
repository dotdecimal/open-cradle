#include <cradle/geometry/regular_grid.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/imaging/inclusion_image.hpp>

#include <boost/assign/std/vector.hpp>

#define BOOST_TEST_MODULE regular_grid
#include <cradle/test.hpp>

using namespace cradle;
using namespace boost::assign;

BOOST_AUTO_TEST_CASE(regular_grid_test)
{
    regular_grid<1,float> default_constructed;

    vector2d p0 = make_vector<double>(0, 0);
    vector2d spacing = make_vector<double>(1, 0.5);
    vector2u n_points = make_vector<unsigned>(2, 3);

    regular_grid<2,double> grid(p0, spacing, n_points);
    BOOST_CHECK_EQUAL(grid.p0, p0);
    BOOST_CHECK_EQUAL(grid.spacing, spacing);
    BOOST_CHECK_EQUAL(grid.n_points, n_points);

    std::vector<vector2d> correct_points;
    correct_points +=
        make_vector<double>(0, 0),
        make_vector<double>(1, 0),
        make_vector<double>(0, 0.5),
        make_vector<double>(1, 0.5),
        make_vector<double>(0, 1),
        make_vector<double>(1, 1);
    CRADLE_CHECK_RANGES_ALMOST_EQUAL(get_point_list(grid), correct_points);
}

BOOST_AUTO_TEST_CASE(grid_bounding_box_test)
{
    vector2d p0 = make_vector<double>(-1, 0);
    vector2d spacing = make_vector<double>(1, 0.5);
    vector2u n_points = make_vector<unsigned>(2, 3);

    regular_grid<2,double> grid(p0, spacing, n_points);

    BOOST_CHECK_EQUAL(bounding_box(grid),
        box2d(make_vector<double>(-1, 0), make_vector<double>(1, 1)));
}

vector3d get_point_at_index(
    regular_grid<3,double> const& grid, size_t index)
{
    size_t x_index = index % grid.n_points[0];
    size_t y_index = (index / grid.n_points[0]) % grid.n_points[1];
    size_t z_index = (index / grid.n_points[0]) / grid.n_points[1];
    return make_vector(
        grid.p0[0] + x_index * grid.spacing[0],
        grid.p0[1] + y_index * grid.spacing[1],
        grid.p0[2] + z_index * grid.spacing[2]);
}

BOOST_AUTO_TEST_CASE(grid_inclusion_test)
{
    // This polygon was chosen because it exposed bugs in the original
    // implementation.
    std::vector<vertex2> vertices;
    vertices +=
        make_vector<double>(  97.2, -49.8),
        make_vector<double>(  98.7, -51.5),
        make_vector<double>( 125.4, -83.5),
        make_vector<double>( 134.6, -96.3),
        make_vector<double>( 138.1,-102.9),
        make_vector<double>( 135.8,-107.6),
        make_vector<double>( 134.7,-109.9),
        make_vector<double>( 125.3,-113.8),
        make_vector<double>( 121.8,-114.7),
        make_vector<double>(  36.9,-126.2),
        make_vector<double>(  35.5,-126.7),
        make_vector<double>(  31.9,-129.8),
        make_vector<double>(  30.9,-131.2),
        make_vector<double>(  29.6,-132.1),
        make_vector<double>(  28.2,-132.8),
        make_vector<double>(  22.7,-136.2),
        make_vector<double>(  18.4,-138.1),
        make_vector<double>(  16.8,-138.3),
        make_vector<double>(  15.3,-139.0),
        make_vector<double>(  14.0,-140.5),
        make_vector<double>(  12.7,-142.6),
        make_vector<double>(  11.2,-143.9),
        make_vector<double>(   9.6,-144.5),
        make_vector<double>(   8.0,-145.4),
        make_vector<double>(   6.3,-145.1),
        make_vector<double>( -10.4,-145.2),
        make_vector<double>( -15.1,-142.8),
        make_vector<double>( -16.5,-141.0),
        make_vector<double>( -19.2,-138.2),
        make_vector<double>( -20.7,-137.9),
        make_vector<double>( -23.7,-136.6),
        make_vector<double>( -27.8,-134.1),
        make_vector<double>( -38.1,-127.1),
        make_vector<double>( -41.1,-126.3),
        make_vector<double>(-129.9,-112.9),
        make_vector<double>(-133.4,-111.8),
        make_vector<double>(-135.7,-110.0),
        make_vector<double>(-138.5,-108.4),
        make_vector<double>(-139.4,-105.9),
        make_vector<double>(-139.4,-100.3),
        make_vector<double>(-137.6, -97.0),
        make_vector<double>(-135.6, -93.7),
        make_vector<double>(-133.0, -90.3),
        make_vector<double>(-111.9, -65.5),
        make_vector<double>( -91.7, -36.9),
        make_vector<double>( -91.0, -35.4),
        make_vector<double>( -90.2, -33.9),
        make_vector<double>( -81.4, -17.0),
        make_vector<double>( -80.8, -15.7),
        make_vector<double>( -70.8,   3.2),
        make_vector<double>( -70.1,   4.7),
        make_vector<double>( -69.3,   6.0),
        make_vector<double>( -68.3,   7.2),
        make_vector<double>( -64.7,  12.3),
        make_vector<double>( -63.9,  13.7),
        make_vector<double>( -53.0,  25.4),
        make_vector<double>( -47.7,  29.0),
        make_vector<double>( -46.2,  29.6),
        make_vector<double>( -23.2,  41.7),
        make_vector<double>( -21.7,  42.7),
        make_vector<double>( -20.2,  43.3),
        make_vector<double>( -18.5,  43.4),
        make_vector<double>(  -8.7,  45.5),
        make_vector<double>(   2.9,  45.4),
        make_vector<double>(   4.6,  44.8),
        make_vector<double>(   7.8,  43.9),
        make_vector<double>(  15.6,  41.1),
        make_vector<double>(  17.2,  40.8),
        make_vector<double>(  18.7,  40.0),
        make_vector<double>(  20.0,  38.5),
        make_vector<double>(  30.3,  30.1),
        make_vector<double>(  35.4,  27.0),
        make_vector<double>(  44.9,  19.9),
        make_vector<double>(  46.2,  19.2),
        make_vector<double>(  47.2,  18.2),
        make_vector<double>(  48.1,  16.9),
        make_vector<double>(  57.3,   6.0),
        make_vector<double>(  70.1, -13.8),
        make_vector<double>(  87.0, -37.5),
        make_vector<double>(  88.2, -38.9),
        make_vector<double>(  89.2, -40.4);

    polygon2 poly;
    initialize(&poly.vertices, vertices);

    polyset slice0;
    add_polygon(slice0, poly);

    structure_geometry volume;
    volume.slices +=
        structure_geometry_slice(0.5, 1, slice0);

    regular_grid<3,double> grid(
        make_vector<double>(-200, -200, 0.5),
        make_vector<double>(4, 4, 1),
        make_vector<unsigned>(100, 100, 1));

    // TODO: This is no longer correct.
    //auto info = compute_grid_cells_in_structure(grid, volume);
    //size_t n_inside = info.cells_inside.size();
    //size_t index = 0;
    //for (size_t i = 0; i != n_inside; ++i)
    //{
    //    size_t next_inside = info.cells_inside[i].index;
    //    for (; index != next_inside; ++index)
    //        BOOST_CHECK(!is_inside(volume, get_point_at_index(grid, index)));
    //    BOOST_CHECK(is_inside(volume, get_point_at_index(grid, index)));
    //    ++index;
    //}
}
