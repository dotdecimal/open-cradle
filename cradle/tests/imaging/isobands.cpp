#include <cradle/imaging/isobands.hpp>
#include <cradle/imaging/isolines.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/geometry/polygonal.hpp>
#include <algorithm>

#define BOOST_TEST_MODULE isobands
#include <cradle/imaging/test.hpp>

double tolerance = 0.001;

using namespace cradle;

double total_area(std::vector<triangle<2,double> > const& tris)
{
    double area = 0;
    for (std::vector<triangle<2,double> >::const_iterator
        i = tris.begin(); i != tris.end(); ++i)
    {
        area += get_area(*i);
    }
    return area;
}

bool isolines_match_isobands(
    std::vector<line_segment<2,double> > const& lines,
    std::vector<triangle<2,double> > const& tris)
{
    for (std::vector<line_segment<2,double> >::const_iterator
        i = lines.begin(); i != lines.end(); ++i)
    {
        for (std::vector<triangle<2,double> >::const_iterator
            j = tris.begin(); j != tris.end(); ++j)
        {
            for (int k = 0; k != 2; ++k)
            {
                for (int l = 0; l != 3; ++l)
                {
                    if (almost_equal((*i)[k], (*j)[l], tolerance))
                        goto found_vertex;
                }
                goto no_match;
             found_vertex:
                ;
            }
            goto found_segment;
         no_match:
            ;
        }
        return false;
      found_segment:
        ;
    }
    return true;
}

std::vector<polygon2> as_polygons(
    std::vector<triangle<2,double> > const& tris)
{
    size_t n_tris = tris.size();
    std::vector<polygon2> polys(n_tris);
    for (size_t i = 0; i != n_tris; ++i)
         polys[i] = as_polygon(tris[i]);
    return polys;
}

bool all_ccw(std::vector<triangle<2,double> > const& tris)
{
    for (std::vector<triangle<2,double> >::const_iterator
        i = tris.begin(); i != tris.end(); ++i)
    {
        if (!is_ccw(*i))
            return false;
    }
    return true;
}

BOOST_AUTO_TEST_CASE(isobands_test)
{
    // There are three significant values in the isobands algorithm:
    // - below the low level
    // - above the high level
    // - in between
    // Given a 2x2 square in the image, there are five points of interest to
    // the algorithm: the four corners and the center.
    // Thus, there are 3 ^ 5 possible 2x2 squares that might yield different
    // results from the algorithm. The following image is constructed such
    // that it has all of them.
    image<2,int8_t,unique> img;
    create_image(img, make_vector<unsigned>(18, 54));
    img.value_mapping = linear_function<double>(1, 0.5);
    set_spatial_mapping(
        img, make_vector<double>(-1, 0), make_vector<double>(2, 3));
    for (unsigned i = 0; i != 27; ++i)
    {
        unsigned index[5];
        index[4] = i / 9;
        index[3] = (i / 3) % 3;
        index[2] = i % 3;
        for (unsigned j = 0; j != 9; ++j)
        {
            index[1] = (j / 3) % 3;
            index[0] = j % 3;
            // With the value mapping above, these will translate to
            // { 1.5, 3, 4.5 }.
            // The iso levels we're interested in are 2 and 4.
            int8_t values[3] = { 1, 4, 7 };
            // Skew the average towards the level of index[4].
            switch (index[4])
            {
             case 0:
                values[0] -= 100;
                break;
             case 2:
                values[2] += 100;
                break;
            }
            // Assign the four pixel values.
            get_pixel_ref(img, make_vector(j * 2 + 0, i * 2 + 0)) =
                values[index[0]];
            get_pixel_ref(img, make_vector(j * 2 + 1, i * 2 + 0)) =
                values[index[1]];
            get_pixel_ref(img, make_vector(j * 2 + 0, i * 2 + 1)) =
                values[index[2]];
            get_pixel_ref(img, make_vector(j * 2 + 1, i * 2 + 1)) =
                values[index[3]];
        }
    }

    // Construct the isobands for values below 2.
    auto low_tris = compute_isobands(img, -100, 2);
    // Construct the isobands for values between 2 and 4.
    auto middle_tris = compute_isobands(img, 2, 4);
    // Construct the isobands for values above 4.
    auto high_tris = compute_isobands(img, 4, 100);

    // All the triangles should be CCW.
    BOOST_CHECK(all_ccw(low_tris));
    BOOST_CHECK(all_ccw(middle_tris));
    BOOST_CHECK(all_ccw(high_tris));

    // Create polysets to represent the three bands.
    polyset low_region;
    create_polyset_from_polygons(&low_region, as_polygons(low_tris));
    polyset middle_region;
    create_polyset_from_polygons(&middle_region, as_polygons(middle_tris));
    polyset high_region;
    create_polyset_from_polygons(&high_region, as_polygons(high_tris));

    // The total area of the triangles in each region should equal the area of
    // the region itself.
    // Allow a little more tolerance here because the error accumulates.
    CRADLE_CHECK_WITHIN_TOLERANCE(total_area(low_tris),
        get_area(low_region), 0.1);
    CRADLE_CHECK_WITHIN_TOLERANCE(total_area(middle_tris),
        get_area(middle_region), 0.1);
    CRADLE_CHECK_WITHIN_TOLERANCE(total_area(high_tris),
        get_area(high_region), 0.1);

    // There should be no overlap between any of the regions.
    polyset overlap;
    do_set_operation(&overlap, set_operation::INTERSECTION, low_region,
        middle_region);
    BOOST_CHECK(almost_equal(get_area(overlap), 0., tolerance));
    do_set_operation(&overlap, set_operation::INTERSECTION, low_region,
        high_region);
    BOOST_CHECK(almost_equal(get_area(overlap), 0., tolerance));
    do_set_operation(&overlap, set_operation::INTERSECTION, middle_region,
        high_region);
    BOOST_CHECK(almost_equal(get_area(overlap), 0., tolerance));

    // The union of the three regions should be the bounding box of the
    // image.
    polyset full_region;
    do_set_operation(&full_region, set_operation::UNION, low_region,
        middle_region);
    do_set_operation(&full_region, set_operation::UNION, full_region,
        high_region);
    polyset img_box;
    create_polyset(&img_box, as_polygon(get_bounding_box(img)));
    BOOST_CHECK(almost_equal(full_region, img_box, 0.001));

    // Construct the isobands for values between 2.5 and 3.5.
    // These bands should lie strictly inside the middle region.
    auto inner_tris = compute_isobands(img, 2, 4);
    polyset inner_region;
    create_polyset_from_polygons(&inner_region, as_polygons(inner_tris));
    do_set_operation(&overlap, set_operation::INTERSECTION, inner_region,
        middle_region);
    BOOST_CHECK(almost_equal(overlap, inner_region, tolerance));

    // Loop through each image point and check that it's in the proper region.
    for (unsigned i = 0; i != 54; ++i)
    {
        for (unsigned j = 0; j != 18; ++j)
        {
            vector2d p = get_pixel_center(img, make_vector(j, i));
            double value = apply(img.value_mapping,
                get_pixel_ref(img, make_vector(j, i)));
            polyset* region;
            if (value < 2)
                region = &low_region;
            else if (value < 4)
                region = &middle_region;
            else
                region = &high_region;
            BOOST_CHECK(is_inside(*region, p));
        }
    }

    // Generate the isolines at 2 and 4 and check that the isolines appear as
    // edges in the isobands.
    auto low_lines = compute_isolines(img, 2);
    BOOST_CHECK(isolines_match_isobands(low_lines, low_tris));
    BOOST_CHECK(isolines_match_isobands(low_lines, middle_tris));
    auto high_lines = compute_isolines(img, 4);
    BOOST_CHECK(isolines_match_isobands(high_lines, middle_tris));
    close_isoline_contours(high_lines, img, 4);
    BOOST_CHECK(isolines_match_isobands(high_lines, high_tris));
}
