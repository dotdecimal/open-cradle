#include <cradle/geometry/polygonal.hpp>
#include <boost/assign/std/vector.hpp>
#include <cradle/anonymous.hpp>

#define BOOST_TEST_MODULE polygonal
#include <cradle/test.hpp>

using namespace cradle;
using namespace boost::assign;

// polygon2

BOOST_AUTO_TEST_CASE(simple_ccw_poly_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(0, 0),
            make_vector<double>(5, 0),
            make_vector<double>(2, 2),
            make_vector<double>(0, 2);
        initialize(&poly.vertices, vertices);
    }
    CRADLE_CHECK_ALMOST_EQUAL(get_area(poly), 7.);
    BOOST_CHECK(!is_inside(poly, make_vector<double>(-1, 1)));
    BOOST_CHECK(is_inside(poly, make_vector<double>(1, 1)));
}

BOOST_AUTO_TEST_CASE(simple_cw_poly_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(0, 0),
            make_vector<double>(0, 2),
            make_vector<double>(2, 2),
            make_vector<double>(5, 0);
        initialize(&poly.vertices, vertices);
    }
    CRADLE_CHECK_ALMOST_EQUAL(get_area(poly), 7.);
    BOOST_CHECK(!is_inside(poly, make_vector<double>(-1, 1)));
    BOOST_CHECK(is_inside(poly, make_vector<double>(1, 1)));
}

BOOST_AUTO_TEST_CASE(triangle_test)
{
    triangle<3,double> tri(
        make_vector<double>(0, 0, 0),
        make_vector<double>(0, 0, 2),
        make_vector<double>(0, 1, 0));

    CRADLE_CHECK_ALMOST_EQUAL(get_normal(tri),
        make_vector<double>(-1, 0, 0));
}

BOOST_AUTO_TEST_CASE(square_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 2, -2);
        initialize(&poly.vertices, vertices);
    }
    CRADLE_CHECK_ALMOST_EQUAL(get_area(poly), 16.);
    BOOST_CHECK(!is_inside(poly, make_vector<double>(-3, 3)));
    BOOST_CHECK(is_inside(poly, make_vector<double>(-1, 1)));
}

BOOST_AUTO_TEST_CASE(concave_poly_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 0,  0),
            make_vector<double>( 2, -2);
        initialize(&poly.vertices, vertices);
    }
    CRADLE_CHECK_ALMOST_EQUAL(get_area(poly), 12.);
    BOOST_CHECK(!is_inside(poly, make_vector<double>(-3, 3)));
    BOOST_CHECK(!is_inside(poly, make_vector<double>(1, 0)));
    BOOST_CHECK(is_inside(poly, make_vector<double>(-1, 1)));
}

BOOST_AUTO_TEST_CASE(edge_view_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(0, 0),
            make_vector<double>(0, 1),
            make_vector<double>(1, 1);
        initialize(&poly.vertices, vertices);
    }

    vertex2_array::const_iterator
        vertex_i = poly.vertices.begin(), vertex_j = poly.vertices.end() - 1;
    for (polygon2_edge_view ev(poly); !ev.done(); ev.advance(),
        vertex_j = vertex_i++)
    {
        BOOST_CHECK_EQUAL(ev.p0(), *vertex_j);
        BOOST_CHECK_EQUAL(ev.p1(), *vertex_i);
    }
}

BOOST_AUTO_TEST_CASE(circle_test)
{
    circle<double> circle(make_vector<double>(0, 0), 2);
    polygon2 poly = as_polygon(circle, 8);

    std::vector<vector<2,double> > correct_vertices;
    double srt = std::sqrt(2.);
    correct_vertices +=
        make_vector<double>(   2,    0),
        make_vector<double>( srt,  srt),
        make_vector<double>(   0,    2),
        make_vector<double>(-srt,  srt),
        make_vector<double>(  -2,    0),
        make_vector<double>(-srt, -srt),
        make_vector<double>(   0,   -2),
        make_vector<double>( srt, -srt);

    CRADLE_CHECK_RANGES_ALMOST_EQUAL(poly.vertices, correct_vertices);
}

BOOST_AUTO_TEST_CASE(box_test)
{
    box2d box(make_vector<double>(0, 0), make_vector<double>(2, 2));
    polygon2 poly = as_polygon(box);

    std::vector<vector<2,double> > correct_vertices;
    correct_vertices +=
        make_vector<double>(0, 0),
        make_vector<double>(2, 0),
        make_vector<double>(2, 2),
        make_vector<double>(0, 2);

    CRADLE_CHECK_RANGES_ALMOST_EQUAL(poly.vertices, correct_vertices);
}

// polyset

double tolerance = 0.00001;

BOOST_AUTO_TEST_CASE(two_polygons_test)
{
    polygon2 poly0, poly1;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 2, -2);
        initialize(&poly0.vertices, vertices);
    }
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(4, 4),
            make_vector<double>(4, 5),
            make_vector<double>(5, 5),
            make_vector<double>(5, 4);
        initialize(&poly1.vertices, vertices);
    }

    polyset region;
    add_polygon(region, poly0);
    add_polygon(region, poly1);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(region), 17., tolerance);

    BOOST_CHECK(!is_inside(region, make_vector<double>(-3, 3)));
    BOOST_CHECK(!is_inside(region, make_vector<double>(3, 3)));
    BOOST_CHECK(!is_inside(region, make_vector<double>(6, 6)));
    BOOST_CHECK(is_inside(region, make_vector<double>(-1, 1)));
    BOOST_CHECK(is_inside(region, make_vector<double>(4.5, 4.5)));
    BOOST_CHECK(is_inside(region, make_vector<double>(0, 0)));
}

BOOST_AUTO_TEST_CASE(polygon_with_hole_test)
{
    polygon2 poly, hole;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 2, -2);
        initialize(&poly.vertices, vertices);
    }
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-1, -1),
            make_vector<double>(-1,  1),
            make_vector<double>( 1,  1),
            make_vector<double>( 1, -1);
        initialize(&hole.vertices, vertices);
    }
    polyset frame;
    add_polygon(frame, poly);
    add_hole(frame, hole);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(frame), 12., tolerance);

    BOOST_CHECK(!is_inside(frame, make_vector<double>(-3, 3)));
    BOOST_CHECK(!is_inside(frame, make_vector<double>(0, 0)));
    BOOST_CHECK(is_inside(frame, make_vector<double>(-1.5, 1.5)));
}

BOOST_AUTO_TEST_CASE(polyset_as_polygon_list_test)
{
    polygon2 outside, hole, inside;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-3, -3),
            make_vector<double>(-3,  3),
            make_vector<double>( 3,  3),
            make_vector<double>( 3, -3);
        initialize(&outside.vertices, vertices);
    }
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 2, -2);
        initialize(&hole.vertices, vertices);
    }
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-1, -1),
            make_vector<double>(-1,  1),
            make_vector<double>( 1,  1),
            make_vector<double>( 1, -1);
        initialize(&inside.vertices, vertices);
    }

    polyset frame;
    add_polygon(frame, outside);
    add_hole(frame, hole);
    add_polygon(frame, inside);

    std::vector<polygon2> polys = as_polygon_list(frame);
    BOOST_CHECK_EQUAL(polys.size(), 2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(polys[0]) + get_area(polys[1]),
        get_area(frame), tolerance);

    polyset reconstructed;
    add_polygon(reconstructed, polys[0]);
    add_polygon(reconstructed, polys[1]);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(reconstructed), get_area(frame),
        tolerance);
}

BOOST_AUTO_TEST_CASE(polyset_set_operations_test)
{
    polygon2 p;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-6, -3),
            make_vector<double>(-6,  3),
            make_vector<double>( 6,  3),
            make_vector<double>( 6, -3);
        initialize(&p.vertices, vertices);
    }
    polyset wide_rect;
    create_polyset(&wide_rect, p);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(wide_rect), 72., tolerance);

    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-3, -6),
            make_vector<double>(-3,  6),
            make_vector<double>( 3,  6),
            make_vector<double>( 3, -6);
        initialize(&p.vertices, vertices);
    }
    polyset tall_rect;
    create_polyset(&tall_rect, p);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(tall_rect), 72., tolerance);

    polyset cross, square;
    do_set_operation(&cross, set_operation::UNION, wide_rect, tall_rect);
    do_set_operation(&square, set_operation::INTERSECTION, wide_rect, tall_rect);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(cross), 108., tolerance);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(square), 36., tolerance);

    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-1, -1),
            make_vector<double>(-1,  1),
            make_vector<double>( 1,  1),
            make_vector<double>( 1, -1);
        initialize(&p.vertices, vertices);
    }
    polyset small_square;
    create_polyset(&small_square, p);
    do_set_operation(&cross, set_operation::DIFFERENCE, cross, small_square);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_area(cross), 104., tolerance);
    BOOST_CHECK(!is_inside(cross, make_vector<double>(0, 0)));
    BOOST_CHECK(is_inside(cross, make_vector<double>(2, 0)));
    BOOST_CHECK(!is_inside(cross, make_vector<double>(8, 0)));
    BOOST_CHECK(is_inside(cross, make_vector<double>(0, -5)));
}

BOOST_AUTO_TEST_CASE(polyset_comparisons_test)
{
    polygon2 poly1, poly2;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-2, -2),
            make_vector<double>(-2,  2),
            make_vector<double>( 2,  2),
            make_vector<double>( 2, -2);
        initialize(&poly1.vertices, vertices);
    }
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-1, -1),
            make_vector<double>(-1,  1),
            make_vector<double>( 1,  1),
            make_vector<double>( 1, -1);
        initialize(&poly2.vertices, vertices);
    }

    polyset region1;
    add_polygon(region1, poly1);
    add_hole(region1, poly2);

    polyset region2;
    add_polygon(region2, poly1);
    add_hole(region2, poly2);

    polyset region3;
    add_polygon(region3, poly1);
    add_polygon(region3, poly2);

    BOOST_CHECK(almost_equal(region1, region2, 0.001));
    BOOST_CHECK(!almost_equal(region1, region3, 0.001));
}

// structure_geometry

BOOST_AUTO_TEST_CASE(structure_geometry_test0)
{
    polyset area0, area1, area2;

    add_polygon(area0, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 6),
        make_vector<double>(3, 3))));
    BOOST_CHECK_EQUAL(area0.polygons.size(), 1);

    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(1, 0), make_vector<double>(0, 0),
        make_vector<double>(0, 1))));
    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(1, 3), make_vector<double>(0, 3),
        make_vector<double>(0, 4))));
    BOOST_CHECK_EQUAL(area1.polygons.size(), 2);

    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 1),
        make_vector<double>(1, 0))));
    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(2, 0), make_vector<double>(2, 1),
        make_vector<double>(3, 0))));
    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(4, 0), make_vector<double>(4, 1),
        make_vector<double>(5, 0))));
    BOOST_CHECK_EQUAL(area2.polygons.size(), 3);

    structure_geometry volume;
    volume.slices +=
        structure_geometry_slice(0.0, 1.5, area0),
        structure_geometry_slice(1.5, 1.5, area1),
        structure_geometry_slice(3.0, 1.5, area2);

    BOOST_CHECK(get_slice(volume, -0.8) == 0);
    BOOST_CHECK(get_slice(volume, -0.7) != 0);
    BOOST_CHECK(get_slice(volume, 3.7) != 0);
    BOOST_CHECK(get_slice(volume, 3.8) == 0);

    BOOST_CHECK_EQUAL(get_slice(volume, 1  )->position, 1.5);
    BOOST_CHECK_EQUAL(get_slice(volume, 1  )->region.polygons.size(), 2);
    BOOST_CHECK_EQUAL(get_slice(volume, 2.5)->position, 3);
    BOOST_CHECK_EQUAL(get_slice(volume, 2.5)->region.polygons.size(), 3);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume), 17.25, tolerance);
}

BOOST_AUTO_TEST_CASE(structure_geometry_test1)
{
    polyset area0, area1, area2, area3;

    add_polygon(area0, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 6),
        make_vector<double>(3, 3))));

    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(1, 0), make_vector<double>(0, 0),
        make_vector<double>(0, 1))));
    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(1, 3), make_vector<double>(0, 3),
        make_vector<double>(0, 4))));

    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 1),
        make_vector<double>(1, 0))));
    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(2, 0), make_vector<double>(2, 1),
        make_vector<double>(3, 0))));

    add_polygon(area3, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(4, 0), make_vector<double>(4, 1),
        make_vector<double>(5, 0))));

    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 1, area1),
        structure_geometry_slice(2.0, 1, area2),
        structure_geometry_slice(2.5, 1, area3);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(0.0, 1, area0),
        structure_geometry_slice(1.0, 1, area1),
        structure_geometry_slice(2.0, 1, area2),
        structure_geometry_slice(2.5, 1, area3);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 11.125, tolerance);

    BOOST_CHECK(almost_equal(volume2, volume2, tolerance));
    BOOST_CHECK(!almost_equal(volume1, volume2, tolerance));
}

BOOST_AUTO_TEST_CASE(set_operation_test)
{
    polyset area0, area1, area2, area3, area4, area5;

    add_polygon(area0, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 5),
        make_vector<double>(4, 5), make_vector<double>(4, 0))));
    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 5),
        make_vector<double>(4, 5), make_vector<double>(4, 0))));
    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 5),
        make_vector<double>(4, 5), make_vector<double>(4, 0))));

    add_polygon(area3, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(6, 0), make_vector<double>(6, 5),
        make_vector<double>(10, 5), make_vector<double>(10, 0))));
    add_polygon(area4, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(2, 0), make_vector<double>(2, 5),
        make_vector<double>(6, 5), make_vector<double>(6, 0))));
    add_polygon(area5, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(0, 5),
        make_vector<double>(4, 5), make_vector<double>(4, 0))));

    {
    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 2, area0),
        structure_geometry_slice(3.0, 2, area1),
        structure_geometry_slice(4.5, 1, area2),
        structure_geometry_slice(6.0, 2, polyset());
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume1), 100., tolerance);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(1.0, 2, polyset()),
        structure_geometry_slice(3.0, 2, area3),
        structure_geometry_slice(4.5, 1, area4),
        structure_geometry_slice(6.0, 2, area5);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 100., tolerance);

    structure_geometry union_;
    do_set_operation(&union_, set_operation::UNION, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(union_), 190., tolerance);

    structure_geometry intersection;
    do_set_operation(&intersection, set_operation::INTERSECTION, volume1,
        volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(intersection), 10., tolerance);

    structure_geometry xor_;
    do_set_operation(&xor_, set_operation::XOR, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(xor_), 180., tolerance);

    structure_geometry difference;
    do_set_operation(&difference, set_operation::DIFFERENCE, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(difference), 90., tolerance);
    }

    {
    structure_geometry volume;
    volume.slices +=
        structure_geometry_slice(1.0, 2, area0),
        structure_geometry_slice(3.0, 2, area1),
        structure_geometry_slice(4.5, 1, area2),
        structure_geometry_slice(6.0, 2, polyset());

    structure_geometry result;

    structure_geometry mismatched_volume1;
    mismatched_volume1.slices +=
        structure_geometry_slice(1.0, 2, polyset()),
        structure_geometry_slice(3.0, 2, area3),
        structure_geometry_slice(4.5, 1, area4);
    BOOST_CHECK_THROW(
        do_set_operation(&result, set_operation::UNION,
            volume, mismatched_volume1),
        std::exception);

    structure_geometry mismatched_volume2;
    mismatched_volume2.slices +=
        structure_geometry_slice(1.0, 2, polyset()),
        structure_geometry_slice(3.0, 2, area1),
        structure_geometry_slice(4.5, 1, area2),
        structure_geometry_slice(6.0, 1, area3);
    BOOST_CHECK_THROW(
        do_set_operation(&result, set_operation::UNION,
            volume, mismatched_volume2),
        std::exception);

    structure_geometry mismatched_volume3;
    mismatched_volume3.slices +=
        structure_geometry_slice(1.1, 2, polyset()),
        structure_geometry_slice(3.0, 2, area1),
        structure_geometry_slice(4.5, 1, area2),
        structure_geometry_slice(6.0, 2, area3);
    BOOST_CHECK_THROW(
        do_set_operation(&result, set_operation::UNION,
            volume, mismatched_volume3),
        std::exception);
    }

    {
    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 2, polyset()),
        structure_geometry_slice(3.0, 2, area0),
        structure_geometry_slice(4.5, 1, area1),
        structure_geometry_slice(6.0, 2, area2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume1), 100., tolerance);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(1.0, 2, area3),
        structure_geometry_slice(3.0, 2, area4),
        structure_geometry_slice(4.5, 1, area5),
        structure_geometry_slice(6.0, 2, polyset());
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 100., tolerance);

    structure_geometry union_;
    do_set_operation(&union_, set_operation::UNION, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(union_), 160., tolerance);

    structure_geometry intersection;
    do_set_operation(&intersection, set_operation::INTERSECTION, volume1,
        volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(intersection), 40., tolerance);

    structure_geometry xor_;
    do_set_operation(&xor_, set_operation::XOR, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(xor_), 120., tolerance);

    structure_geometry difference;
    do_set_operation(&difference, set_operation::DIFFERENCE, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(difference), 60., tolerance);
    }

    {
    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 2, polyset()),
        structure_geometry_slice(3.0, 2, area0),
        structure_geometry_slice(4.5, 1, area1),
        structure_geometry_slice(6.0, 2, area2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume1), 100., tolerance);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(1.0, 2, area3),
        structure_geometry_slice(3.0, 2, area4),
        structure_geometry_slice(4.5, 1, area5),
        structure_geometry_slice(6.0, 2, polyset());
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 100., tolerance);

    structure_geometry union_;
    do_set_operation(&union_, set_operation::UNION, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(union_), 160., tolerance);

    structure_geometry intersection;
    do_set_operation(&intersection, set_operation::INTERSECTION, volume1,
        volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(intersection), 40., tolerance);

    structure_geometry xor_;
    do_set_operation(&xor_, set_operation::XOR, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(xor_), 120., tolerance);

    structure_geometry difference;
    do_set_operation(&difference, set_operation::DIFFERENCE, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(difference), 60., tolerance);
    }

    {
    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 2, area0),
        structure_geometry_slice(3.0, 2, polyset()),
        structure_geometry_slice(4.5, 1, area1),
        structure_geometry_slice(6.0, 2, area2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume1), 100., tolerance);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(1.0, 2, area3),
        structure_geometry_slice(3.0, 2, area4),
        structure_geometry_slice(4.5, 1, area5),
        structure_geometry_slice(6.0, 2, polyset());
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 100., tolerance);

    structure_geometry union_;
    do_set_operation(&union_, set_operation::UNION, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(union_), 180., tolerance);

    structure_geometry intersection;
    do_set_operation(&intersection, set_operation::INTERSECTION, volume1,
        volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(intersection), 20., tolerance);

    structure_geometry xor_;
    do_set_operation(&xor_, set_operation::XOR, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(xor_), 160., tolerance);

    structure_geometry difference;
    do_set_operation(&difference, set_operation::DIFFERENCE, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(difference), 80., tolerance);
    }

    {
    structure_geometry volume1;
    volume1.slices +=
        structure_geometry_slice(1.0, 2, area0),
        structure_geometry_slice(3.0, 2, area1),
        structure_geometry_slice(4.5, 1, area2),
        structure_geometry_slice(6.0, 2, polyset());
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume1), 100., tolerance);

    structure_geometry volume2;
    volume2.slices +=
        structure_geometry_slice(1.0, 2, area3),
        structure_geometry_slice(3.0, 2, polyset()),
        structure_geometry_slice(4.5, 1, area4),
        structure_geometry_slice(6.0, 2, area5);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(volume2), 100., tolerance);

    structure_geometry union_;
    do_set_operation(&union_, set_operation::UNION, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(union_), 190., tolerance);

    structure_geometry intersection;
    do_set_operation(&intersection, set_operation::INTERSECTION, volume1,
        volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(intersection), 10., tolerance);

    structure_geometry xor_;
    do_set_operation(&xor_, set_operation::XOR, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(xor_), 180., tolerance);

    structure_geometry difference;
    do_set_operation(&difference, set_operation::DIFFERENCE, volume1, volume2);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(difference), 90., tolerance);
    }
}

BOOST_AUTO_TEST_CASE(expansion_2d)
{
    polyset area0, area1, area2, area3;

    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(6, 0),
        make_vector<double>(6, 6), make_vector<double>(0, 6))));

    add_polygon(area2, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(1, 0),
        make_vector<double>(1, 1), make_vector<double>(0, 1))));

    structure_geometry original;
    original.slices +=
        structure_geometry_slice(1, 1, area1),
        structure_geometry_slice(2, 1, area2);

    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(original), 37., tolerance);

    structure_geometry expanded;
    expand_in_2d(&expanded, original, 1.);

    BOOST_CHECK(get_volume(expanded) > 65);
    BOOST_CHECK(get_volume(expanded) < 83);
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.9, 1, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.9, 1, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, 6.9, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, -0.9, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, -0.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.7, -0.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.7, 6.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, 6.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1.9, 1, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.9, 1, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, 1.9, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, -0.9, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, -0.7, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1.7, -0.7, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1.7, 1.7, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, 1.7, 2)));
}

BOOST_AUTO_TEST_CASE(expansion_3d)
{
    polyset area0, area1, area2, area3;

    add_polygon(area1, make_polygon2(anonymous<std::vector<vector2d> >(
        make_vector<double>(0, 0), make_vector<double>(6, 0),
        make_vector<double>(6, 6), make_vector<double>(0, 6))));

    structure_geometry original;
    original.slices +=
        structure_geometry_slice(-1, 1, polyset()),
        structure_geometry_slice( 0, 1, polyset()),
        structure_geometry_slice( 1, 1, area1),
        structure_geometry_slice( 2, 1, polyset()),
        structure_geometry_slice( 3, 1, polyset());

    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(original), 36., tolerance);

    structure_geometry expanded;
    expand_in_3d(&expanded, original, 1.);

    BOOST_CHECK(get_volume(expanded) > 136);
    BOOST_CHECK(get_volume(expanded) < 197);
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.9, 1, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.9, 1, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, 6.9, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(1, -0.9, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, -0.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.7, -0.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6.7, 6.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(-0.7, 6.7, 1)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(0, 0, 0)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6, 0, 0)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6, 6, 0)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(0, 6, 0)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(0, 0, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6, 0, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(6, 6, 2)));
    BOOST_CHECK(is_inside(expanded, make_vector<double>(0, 6, 2)));

    structure_geometry contracted;
    expand_in_3d(&contracted, original, -1.);
    CRADLE_CHECK_WITHIN_TOLERANCE(get_volume(contracted), 0., tolerance);
}

BOOST_AUTO_TEST_CASE(polygon_bounding_box_test)
{
    polygon2 poly;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(0, 0),
            make_vector<double>(-1, -1),
            make_vector<double>(-3, 0),
            make_vector<double>(0, 7),
            make_vector<double>(3, 3),
            make_vector<double>(3, 2);
        initialize(&poly.vertices, vertices);
    }
    BOOST_CHECK_EQUAL(bounding_box(poly),
        box2d(make_vector<double>(-3, -1), make_vector<double>(6, 8)));
}

BOOST_AUTO_TEST_CASE(polyset_bounding_box_test)
{
    polygon2 p;
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(-6, -3),
            make_vector<double>(-6, 3),
            make_vector<double>(-4, 3),
            make_vector<double>(-4, -3);
        initialize(&p.vertices, vertices);
    }
    polyset area;
    create_polyset(&area, p);
    {
        std::vector<vertex2> vertices;
        vertices +=
            make_vector<double>(2, -6),
            make_vector<double>(2, -2),
            make_vector<double>(4, -2),
            make_vector<double>(4, -6);
        initialize(&p.vertices, vertices);
    }
    add_polygon(area, p);

    box<2,double> bb = bounding_box(area);
    BOOST_CHECK(almost_equal(bb.corner, make_vector<double>(-6, -6), 0.00001));
    BOOST_CHECK(almost_equal(bb.size, make_vector<double>(10, 9), 0.00001));
}
