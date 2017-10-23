#include "catch.hpp"

#include <cradle/common.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/geometry/line_strip.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/geometry/multiple_source_view.hpp>
#include <cradle/geometry/project_via_render_to_texture.hpp>
#include <cradle/geometry/slice_mesh.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/imaging/inclusion_image.hpp>
#include <cradle/io/vtk_io.hpp>
#include <cradle/math/gaussian.hpp>
#include <cradle/rt/utilities.hpp>
#include <cradle/unit_tests/testing.hpp>

#include <cradle_test_utilities.hpp>

using namespace cradle;

namespace unittests
{

// Global test variables
bool isDebugGeometry = false;
image<3, double, cradle::shared> dose_override_inside_image;
image<3, double, cradle::shared> stopping_power_image;
structure_geometry image_structure;
triangle_mesh triangle_mesh_test;
optimized_triangle_mesh opt_triangle_mesh;

void
get_phantom_image()
{
    image<3, double, cradle::unique> stopping_image;
    regular_grid3d stopping_power_grid =
        make_grid_for_box(
            make_box(
                make_vector(ph_image_corner, ph_image_corner, ph_image_corner),
                make_vector(ph_image_length, ph_image_length, ph_image_length)),
            make_vector(ph_pixel_spacing, ph_pixel_spacing, ph_pixel_spacing));
    create_image_on_grid(stopping_image, stopping_power_grid);

    // Set all pixels on image to image_value;
    int pixel_count =
        stopping_image.size[0] * stopping_image.size[1] * stopping_image.size[2];
    for (int i = 0; i < pixel_count; ++i) {
        stopping_image.pixels.ptr[i] = ph_image_value;
    }

    stopping_power_image = share(stopping_image);
}

void
write_vtk_polygon(polygon2 poly, string filename)
{
    structure_polyset_list slice_list;
    slice_description_list master_slices;
    polyset pset = make_polyset(poly);
    slice_list[0.] = pset;
    master_slices.push_back(slice_description(0., 1.0));
    auto struct_geom = structure_geometry(slice_list, master_slices);

    image<3, double, cradle::shared> image_inside =
        override_image_inside_structure(
            stopping_power_image, struct_geom, override_value, 0.9f);

    write_vtk_file(filename, image_inside);
}

void
get_image_structure()
{
    auto poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner),
                make_vector(sq_length, sq_length)));
    auto hole =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));

    structure_polyset_list slice_list;
    slice_description_list master_slices;
    for (int i = 0; i < numberOfSlices; ++i)
    {
        auto pset = make_polyset(poly);
        add_hole(pset, hole);

        // Create polyset slice at position i
        double pos = sq_start_Z_slice + ((double)i * slice_thickness);
        slice_list[pos] = pset;
        master_slices.push_back(slice_description(pos, slice_thickness));
    }

    image_structure = structure_geometry(slice_list, master_slices);
    triangle_mesh_test = compute_triangle_mesh_from_structure(image_structure);
    opt_triangle_mesh = make_optimized_triangle_mesh_for_structure(image_structure);
}

void initialize()
{
    get_phantom_image();
    get_image_structure();
}

TEST_CASE("CradleGeometry compute_triangle_mesh_from_image_double test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  compute_triangle_mesh_from_image_double
    // Dependencies: requires dose_override_test to run prior to set structures and images

    initialize();

    triangle_mesh dose_override_mesh =
        compute_triangle_mesh_from_image_double(
            dose_override_inside_image, override_value - (override_value / 2));

    vertex3_array::const_iterator vertices_end = dose_override_mesh.vertices.end();
    for (
        vertex3_array::const_iterator iter = dose_override_mesh.vertices.begin();
            iter != vertices_end; ++iter)
    {
        double x = (*iter)[0];
        double y = (*iter)[1];
        double z = (*iter)[2];

        // Is the vertex within the Z bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (z < sq_start_Z_position - (ph_pixel_spacing * 1.1))
            || (z > sq_end_Z_position + (ph_pixel_spacing * 1.1))));

        // Is the vertex within the Y bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (y < sq_start_xy_position - (ph_pixel_spacing * 1.1))
            || (y > sq_end_xy_position + (ph_pixel_spacing * 1.1))));

        // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (x < sq_start_xy_position - (ph_pixel_spacing * 1.1))
            || (x > sq_end_xy_position + (ph_pixel_spacing * 1.1))));

        // Is the vertex outside the defined hole +- 10% of pixel spacing
        CHECK_FALSE((
            (y >= hole_start_xy_position)
            && (y < hole_end_xy_position - (ph_pixel_spacing * 1.1))
            && (x >= hole_start_xy_position)
            && (x < hole_end_xy_position - (ph_pixel_spacing * 1.1))));
    }

    if (isDebugGeometry)
    {
        write_vtk_file("Unit_Tests.dir/triangle_image.vtk", dose_override_inside_image);
        write_vtk_file("Unit_Tests.dir/triangle_mesh_from_image.vtk", dose_override_mesh);
    }
}

TEST_CASE("CradleGeometry mesh_contains test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  mesh_contains

    vector3d pointsIn[4];
    vector3d pointsOut[4];

    pointsOut[0] = make_vector(sq_corner, sq_corner, sq_start_Z_position);
    pointsOut[1] = make_vector(0.0, 0.0, sq_corner + 4.01);
    pointsOut[2] = make_vector(sq_corner * -1.0 + .05, 4.0, sq_corner + 4.01);
    pointsOut[3] =
        make_vector(sq_corner + 0.1, sq_corner + 0.1, sq_start_Z_position - 0.05);

    pointsIn[0] =
        make_vector(sq_corner + 0.1, sq_corner + 0.1, sq_start_Z_position + 0.5);
    pointsIn[1] = make_vector(sq_corner + 0.1, 4.0, sq_start_Z_position + 4.0);
    pointsIn[2] =
        make_vector(
            hole_start_xy_position - (ph_pixel_spacing / 2),
            0.0, sq_start_Z_position + ph_pixel_spacing);
    pointsIn[3] =
        make_vector(sq_corner + 0.1, sq_corner + 0.1, sq_start_Z_position + 0.6);

    for (size_t i = 0; i < 4; ++i)
    {
        CHECK_FALSE(mesh_contains(opt_triangle_mesh, pointsOut[i]));
        CHECK(mesh_contains(opt_triangle_mesh, pointsIn[i]));
    }
}

TEST_CASE("CradleGeometry make_triangle_mesh test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_triangle_mesh

    box<3, double> bb = bounding_box(triangle_mesh_test);
    vector<3, double> bbmin = bb.corner;
    vector<3, double> bbmax = get_high_corner(bb);

    vertex3_array::const_iterator vertices_end = triangle_mesh_test.vertices.end();
    for (
        vertex3_array::const_iterator iter = triangle_mesh_test.vertices.begin();
        iter != vertices_end; ++iter)
    {
        double x = (*iter)[0];
        double y = (*iter)[1];
        double z = (*iter)[2];

        // Is the vertex within the Z bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (z < sq_start_Z_position - (ph_pixel_spacing * 1.1))
            || (z > sq_end_Z_position + (ph_pixel_spacing * 1.1))
            || ((bbmin[2] > sq_start_Z_position + (ph_pixel_spacing * 1.1))
            && (bbmax[2] < sq_end_Z_position - (ph_pixel_spacing * 1.1)))));

        // Is the vertex within the Y bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (y < sq_start_xy_position - (ph_pixel_spacing * 1.1))
            || (y > sq_end_xy_position + (ph_pixel_spacing * 1.1))
            || ((bbmin[1] > sq_start_xy_position + (ph_pixel_spacing * 1.1))
            && (bbmax[1] < sq_end_xy_position - (ph_pixel_spacing * 1.1)))));

        // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
        CHECK_FALSE((
            (x < sq_start_xy_position - (ph_pixel_spacing * 1.1))
            || (x > sq_end_xy_position + (ph_pixel_spacing * 1.1))
            || ((bbmin[0] > sq_start_xy_position + (ph_pixel_spacing * 1.1))
            && (bbmax[0] < sq_end_xy_position - (ph_pixel_spacing * 1.1)))));

        // Is the vertex outside the defined hole +- 10% of pixel spacing
        CHECK_FALSE((
            (y >= hole_start_xy_position)
            && (y < hole_end_xy_position - (ph_pixel_spacing * 1.1))
            && (x >= hole_start_xy_position)
            && (x < hole_end_xy_position - (ph_pixel_spacing * 1.1))));
    }

    if (isDebugGeometry)
    {
        write_vtk_file("Unit_Tests.dir/triangle_mesh.vtk", triangle_mesh_test);
    }
}


TEST_CASE("CradleGeometry polygonal bounding_box test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  bounding_box(polyset)

    polygon2 holes =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    polyset polyset =
        make_polyset(
            as_polygon(
                make_box(
                    make_vector(sq_corner, sq_corner),
                    make_vector(sq_length, sq_length))));
    add_hole(polyset, holes);

    box<2, double> bb = bounding_box(polyset);
    vector<2, double> bbmin = bb.corner;
    vector<2, double> bbmax = get_high_corner(bb);

    CHECK_FALSE(bbmin[0] != sq_corner);
    CHECK_FALSE(bbmin[1] != sq_corner);
    CHECK_FALSE(bbmax[0] != sq_corner + sq_length);
    CHECK_FALSE(bbmax[1] != sq_corner + sq_length);

    //Test triangle
    std::vector<vector<2, double> > v2;

    v2.push_back(make_vector(1.0, 2.0));
    v2.push_back(make_vector(11.5, 5.0));
    v2.push_back(make_vector(5.0, 12.0));

    polygon2 poly = make_polygon2(v2);

    bb = bounding_box(poly);
    bbmin = bb.corner;
    bbmax = get_high_corner(bb);

    CHECK_FALSE(bbmin[0] != 1.0);
    CHECK_FALSE(bbmin[1] != 2.0);
    CHECK_FALSE(bbmax[0] != 11.5);
    CHECK_FALSE(bbmax[1] != 12);
}

TEST_CASE("CradleGeometry polygonal is_inside test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  is_inside(polyset, vector2d)

    polygon2 holes =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    polyset polyset =
        make_polyset(
            as_polygon(
                make_box(
                make_vector(sq_corner, sq_corner),
                make_vector(sq_length, sq_length))));
    add_hole(polyset, holes);

    vector2d pointsIn[4];
    vector2d pointsOut[4];

    pointsOut[0] = make_vector(sq_corner - 0.1, sq_corner - 0.1);
    pointsOut[1] = make_vector(0.0, 0.0);
    pointsOut[2] = make_vector(sq_corner + sq_length + .05, 4.0);
    pointsOut[3] = make_vector(sq_corner - 0.1, sq_corner + (sq_length / 2));

    pointsIn[0] = make_vector(sq_corner + 0.1, sq_corner + 0.1);
    pointsIn[1] = make_vector(sq_corner + 0.1, 4.0);
    pointsIn[2] = make_vector(hole_start_xy_position - (ph_pixel_spacing), 0.0);
    pointsIn[3] = make_vector(sq_corner + (sq_length / 2.), sq_corner + sq_length - 0.05);

    for (int i = 0; i < 4; ++i)
    {
        CHECK_FALSE(is_inside(polyset, pointsOut[i]));
        CHECK(is_inside(polyset, pointsIn[i]));
    }
}

TEST_CASE("CradleGeometry polygonal get_area_and_centroid test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_area(polygon2)
    //  get_centroid(polygon2)
    // (Indirectly)
    //  get_area_and_centroid(polygon2)

    vector2d holeShift = make_vector(-2.0, -1.0);

    polygon2 poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner), make_vector(sq_length, sq_length)));
    polygon2 holes =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner) + holeShift,
                make_vector(hole_length, hole_length)));
    polyset polyset = make_polyset(poly);
    add_hole(polyset, holes);

    double polyArea = get_area(poly);
    double holeArea = get_area(holes);
    double polysetArea = get_area(polyset);
    vector2d polyCentroid = get_centroid(poly);
    vector2d holeCentroid = get_centroid(holes);
    vector2d polysetCentroid = get_centroid(polyset);

    // Get the weighted average centroid of the polyset
    auto calculatedPolysetCentroid =
        ((polyArea * polyCentroid) - (holeArea * holeCentroid)) / (polyArea - holeArea);

    // Poly area
    CHECK_FALSE(polyArea != (sq_length * sq_length));

    // Poly centroid
    CHECK_FALSE(polyCentroid != make_vector(0.0, 0.0));

    // Hole area
    CHECK_FALSE(holeArea != (hole_length * hole_length));

    // Hole centroid
    CHECK_FALSE(holeCentroid != holeShift);

    // Polyset area
    CHECK_FALSE(polysetArea != polyArea - holeArea);

    // Polyset centroid
    CHECK_FALSE(polysetCentroid != calculatedPolysetCentroid);
}

TEST_CASE("CradleGeometry get_first_last_intersection test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_first_last_intersection

    std::vector<triangle_mesh> targets;
    targets.push_back(triangle_mesh_test);
    box<3, double> bb = bounding_box(triangle_mesh_test);
    vector<3, double> bbmin = bb.corner;
    vector<3, double> bbmax = get_high_corner(bb);

    // Vertical line through structure
    vector3d v1s = make_vector(hole_end_xy_position + 1, 0.0, sq_end_Z_position + 1);
    vector3d v1e = make_vector(hole_end_xy_position + 1, 0.0, sq_start_Z_position - 1);
    vector3d pt1a, pt1b;
    double u1s = 0.0, u1e = 0.0;
    bool isIntersected =
        get_first_last_intersection(v1s, v1e, targets, pt1a, pt1b, u1s, u1e);
    // Check that returned u values are correct
    double s1t = distance(v1s, v1e);
    double s1s = distance(v1s, pt1a);
    double s1e = distance(v1s, pt1b);
    double u1a = s1s / s1t;
    double u1b = s1e / s1t;
    CHECK_FALSE(std::fabs(u1a - u1s) > tol);
    CHECK_FALSE(std::fabs(u1b - u1e) > tol);
    // Is the intersection point intersecting at the same XY values
    CHECK(isIntersected);
    CHECK_FALSE((
        (v1s[0] != pt1a[0] || v1s[1] != pt1a[1])
        || (v1s[0] != pt1b[0] || v1s[1] != pt1b[1])));
    // Is the intersection point within the Z bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE(
        (std::fabs(pt1a[2] - bbmax[2]) > tol || std::fabs(pt1b[2] - bbmin[2]) > tol));

    // Horizontal line through structure
    vector3d v2s = make_vector(sq_start_xy_position - 1, 0.0, sq_start_Z_position + 2);
    vector3d v2e = make_vector(sq_end_xy_position + 1, 0.0, sq_start_Z_position + 2);
    vector3d pt2a, pt2b;
    double u2s = 0.0, u2e = 0.0;
    isIntersected = get_first_last_intersection(v2s, v2e, targets, pt2a, pt2b, u2s, u2e);
    // Check that returned u values are correct
    double s2t = distance(v2s, v2e);
    double s2s = distance(v2s, pt2a);
    double s2e = distance(v2s, pt2b);
    double u2a = s2s / s2t;
    double u2b = s2e / s2t;
    CHECK_FALSE(std::fabs(u2a - u2s) > tol);
    CHECK_FALSE(std::fabs(u2b - u2e) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((
        (v2s[1] != pt2a[1] || v2s[2] != pt2a[2])
        || (v2s[1] != pt2b[1] || v2s[2] != pt2b[2])));
    // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE(((pt2a[0] != sq_corner) || (pt2b[0] != sq_corner + sq_length)));

    // Horizontal line through hole
    vector3d v3s = make_vector(hole_start_xy_position - 1, 0.0, sq_start_Z_position + 2);
    vector3d v3e = make_vector(hole_end_xy_position + 1, 0.0, sq_start_Z_position + 2);
    vector3d pt3a, pt3b;
    double u3s = 0.0, u3e = 0.0;
    isIntersected = get_first_last_intersection(v3s, v3e, targets, pt3a, pt3b, u3s, u3e);
    // Check that returned u values are correct
    double s3t = distance(v3s, v3e);
    double s3s = distance(v3s, pt3a);
    double s3e = distance(v3s, pt3b);
    double u3a = s3s / s3t;
    double u3b = s3e / s3t;
    CHECK_FALSE(std::fabs(u3a - u3s) > tol);
    CHECK_FALSE(std::fabs(u3b - u3e) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((
        (v3s[1] != pt3a[1] || v3s[2] != pt3a[2])
        || (v3s[1] != pt3b[1] || v3s[2] != pt3b[2])));
    // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE((
        (pt3a[0] != hole_corner) || (pt3b[0] != hole_corner + hole_length)));

    // Angled 45deg line through structure on same slice
    vector3d v4s = make_vector(7.0, 1.0, 2.0);
    vector3d v4e = make_vector(-1.0, -7.0, 2.0);
    vector3d pt4a, pt4b;
    double u4s = 0.0, u4e = 0.0;
    isIntersected = get_first_last_intersection(v4s, v4e, targets, pt4a, pt4b, u4s, u4e);
    // Check that returned u values are correct
    double s4t = distance(v4s, v4e);
    double s4s = distance(v4s, pt4a);
    double s4e = distance(v4s, pt4b);
    double u4a = s4s / s4t;
    double u4b = s4e / s4t;
    CHECK_FALSE(std::fabs(u4a - u4s) > tol);
    CHECK_FALSE(std::fabs(u4b - u4e) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((
        pt4a[0] != sq_corner + sq_length
        || pt4a[1] != 0.0 || pt4b[0] != 0.0 || pt4b[1] != sq_corner));

    // Angled 45deg line through structure through multiple slices
    vector3d v5s =
        make_vector(
            (0.5 * sq_length) + 1.0, -4.0, (0.5 * sq_length) - std::fabs(bbmin[2]) + 1.0);
    vector3d v5e = make_vector(-1.0, -4.0, bbmin[2] - 1.0);
    vector3d pt5a, pt5b;
    double u5s = 0.0, u5e = 0.0;
    isIntersected = get_first_last_intersection(v5s, v5e, targets, pt5a, pt5b, u5s, u5e);
    // Check that returned u values are correct
    double s5t = distance(v5s, v5e);
    double s5s = distance(v5s, pt5a);
    double s5e = distance(v5s, pt5b);
    double u5a = s5s / s5t;
    double u5b = s5e / s5t;
    CHECK_FALSE(std::fabs(u5a - u5s) > tol);
    CHECK_FALSE(std::fabs(u5b - u5e) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((
        (pt5a[0] != sq_corner + sq_length || std::fabs(pt5a[2] - (v5s[2] - 1)) > tol
        || std::fabs(pt5b[0]) > tol) && pt5b[2] != bbmin[2]));

    // Vertical line through origin
    vector3d v6s = make_vector(0.0, 0.0, sq_end_Z_position + 1);
    vector3d v6e = make_vector(0.0, 0.0, sq_start_Z_position - 1);
    vector3d pt6a, pt6b;
    double u6s = 0.0, u6e = 0.0;
    isIntersected = get_first_last_intersection(v6s, v6e, targets, pt6a, pt6b, u6s, u6e);
    // Is the intersection point intersecting at the same XY values
    CHECK_FALSE(isIntersected);

    double cube_length = 6.0;
    triangle_mesh cube =
        make_cube(
            make_vector(0.0, 0.0, 0.0),
            make_vector(cube_length, cube_length, cube_length));
    box<3, double> cb = bounding_box(cube);
    vector<3, double> cbmin = cb.corner;
    vector<3, double> cbmax = get_high_corner(cb);
    std::vector<triangle_mesh> cubetargets;
    cubetargets.push_back(cube);

    // Angled line through cube origin
    vector3d v7s = make_vector(cube_length / 2, cube_length / 2, cbmax[2] + 1);
    vector3d v7e = make_vector(cube_length / 2, cube_length / 2, cbmin[2] - 1);
    vector3d pt7a, pt7b;
    double u7s = 0.0, u7e = 0.0;
    isIntersected =
        get_first_last_intersection(v7s, v7e, cubetargets, pt7a, pt7b, u7s, u7e);
    // Check that returned u values are correct
    double s7t = distance(v7s, v7e);
    double s7s = distance(v7s, pt7a);
    double s7e = distance(v7s, pt7b);
    double u7a = s7s / s7t;
    double u7b = s7e / s7t;
    CHECK_FALSE(std::fabs(u7a - u7s) > tol);
    CHECK_FALSE(std::fabs(u7b - u7e) > tol);
    // Is the intersection point correct?
    CHECK(isIntersected);
    CHECK_FALSE((
        pt7a[0] != cube_length / 2 || pt7a[1] != cube_length / 2
        || pt7a[2] != cube_length));
    CHECK_FALSE((
        pt7b[0] != cube_length / 2 || pt7b[1] != cube_length / 2 || pt7b[2] != 0.0));

    // Angled line through cube corner
    vector3d v8s = make_vector(-1.0, -1.0, 8.0);
    vector3d v8e = make_vector(3.5, 3.5, -1.0);
    vector3d pt8a, pt8b;
    double u8s = 0.0, u8e = 0.0;
    isIntersected =
        get_first_last_intersection(v8s, v8e, cubetargets, pt8a, pt8b, u8s, u8e);
    // Check that returned u values are correct
    double s8t = distance(v8s, v8e);
    double s8s = distance(v8s, pt8a);
    double s8e = distance(v8s, pt8b);
    double u8a = s8s / s8t;
    double u8b = s8e / s8t;
    CHECK_FALSE(std::fabs(u8a - u8s) > tol);
    CHECK_FALSE(std::fabs(u8b - u8e) > tol);
    // Is the intersection point correct?
    CHECK(isIntersected);
    CHECK_FALSE((pt8a[0] != 0.0 || pt8a[1] != 0.0 || pt8a[2] != 6.0));
    CHECK_FALSE((pt8b[0] != 3.0 || pt8b[1] != 3.0 || pt8b[2] != 0.0));
}

TEST_CASE("CradleGeometry get_deepest_intersection test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_deepest_intersection

    std::vector<triangle_mesh> targets;
    targets.push_back(triangle_mesh_test);
    box<3, double> bb = bounding_box(triangle_mesh_test);
    vector<3, double> bbmin = bb.corner;

    // Vertical line through structure
    vector3d v1s = make_vector(hole_end_xy_position + 1, 0.0, sq_end_Z_position + 1);
    vector3d v1e = make_vector(hole_end_xy_position + 1, 0.0, sq_start_Z_position - 1);
    vector3d pt1;
    double u1 = 0.0;
    bool isIntersected = get_deepest_intersection(v1s, v1e, targets, pt1, u1);
    // Check that returned u values are correct
    double s1t = distance(v1s, v1e);
    double s1 = distance(v1s, pt1);
    double u1a = s1 / s1t;
    CHECK_FALSE(std::fabs(u1a - u1) > tol);
    // Is the intersection point intersecting at the same XY values
    CHECK(isIntersected);
    CHECK_FALSE((v1s[0] != pt1[0] || v1s[1] != pt1[1]));
    // Is the intersection point within the Z bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE(std::fabs(pt1[2] - bbmin[2]) > tol);

    // Horizontal line through structure
    vector3d v2s = make_vector(sq_start_xy_position - 1, 0.0, sq_start_Z_position + 2);
    vector3d v2e = make_vector(sq_end_xy_position + 1, 0.0, sq_start_Z_position + 2);
    vector3d pt2;
    double u2 = 0.0;
    isIntersected = get_deepest_intersection(v2s, v2e, targets, pt2, u2);
    // Check that returned u values are correct
    double s2t = distance(v2s, v2e);
    double s2 = distance(v2s, pt2);
    double u2a = s2 / s2t;
    CHECK_FALSE(std::fabs(u2a - u2) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((v2s[1] != pt2[1] || v2s[2] != pt2[2]));
    // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE(pt2[0] != sq_corner + sq_length);

    // Horizontal line through hole
    vector3d v3s = make_vector(hole_start_xy_position - 1, 0.0, sq_start_Z_position + 2);
    vector3d v3e = make_vector(hole_end_xy_position + 1, 0.0, sq_start_Z_position + 2);
    vector3d pt3;
    double u3 = 0.0;
    isIntersected = get_deepest_intersection(v3s, v3e, targets, pt3, u3);
    // Check that returned u values are correct
    double s3t = distance(v3s, v3e);
    double s3 = distance(v3s, pt3);
    double u3a = s3 / s3t;
    CHECK_FALSE(std::fabs(u3a - u3) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((v3s[1] != pt3[1] || v3s[2] != pt3[2]));
    // Is the vertex within the X bounds of the structure +- 10% of pixel spacing
    CHECK_FALSE(pt3[0] != hole_corner + hole_length);

    // Angled 45deg line through structure on same slice
    vector3d v4s = make_vector(7.0, 1.0, 2.0);
    vector3d v4e = make_vector(-1.0, -7.0, 2.0);
    vector3d pt4;
    double u4 = 0.0;
    isIntersected = get_deepest_intersection(v4s, v4e, targets, pt4, u4);
    // Check that returned u values are correct
    double s4t = distance(v4s, v4e);
    double s4 = distance(v4s, pt4);
    double u4a = s4 / s4t;
    CHECK_FALSE(std::fabs(u4a - u4) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((pt4[0] != 0.0 || pt4[1] != sq_corner));

    // Angled 45deg line through structure through multiple slices
    vector3d v5s =
        make_vector((sq_length / 2) + 1, -4.0, (sq_length / 2) - std::fabs(bbmin[2]) + 1);
    vector3d v5e = make_vector(-1.0, -4.0, bbmin[2] - 1.0);
    vector3d pt5;
    double u5 = 0.0;
    isIntersected = get_deepest_intersection(v5s, v5e, targets, pt5, u5);
    // Check that returned u values are correct
    double s5t = distance(v5s, v5e);
    double s5 = distance(v5s, pt5);
    double u5a = s5 / s5t;
    CHECK_FALSE(std::fabs(u5a - u5) > tol);
    // Is the intersection point intersecting at the same YZ values
    CHECK(isIntersected);
    CHECK_FALSE((std::fabs(pt5[0]) > tol && pt5[2] != bbmin[2]));

    // Vertical line through origin
    vector3d v6s = make_vector(0.0, 0.0, sq_end_Z_position + 1);
    vector3d v6e = make_vector(0.0, 0.0, sq_start_Z_position - 1);
    vector3d pt6;
    double u6 = 0.0;
    isIntersected = get_deepest_intersection(v6s, v6e, targets, pt6, u6);
    // Is the intersection point intersecting at the same XY values
    CHECK_FALSE(isIntersected);

    double cube_length = 6.0;
    triangle_mesh cube =
        make_cube(
            make_vector(0.0, 0.0, 0.0),
            make_vector(cube_length, cube_length, cube_length));
    box<3, double> cb = bounding_box(cube);
    vector<3, double> cbmin = cb.corner;
    vector<3, double> cbmax = get_high_corner(cb);
    std::vector<triangle_mesh> cubetargets;
    cubetargets.push_back(cube);

    // Angled line through cube origin
    vector3d v7s = make_vector(cube_length / 2, cube_length / 2, cbmax[2] + 1);
    vector3d v7e = make_vector(cube_length / 2, cube_length / 2, cbmin[2] - 1);
    vector3d pt7;
    double u7 = 0.0;
    isIntersected = get_deepest_intersection(v7s, v7e, cubetargets, pt7, u7);
    // Check that returned u values are correct
    double s7t = distance(v7s, v7e);
    double s7 = distance(v7s, pt7);
    double u7a = s7 / s7t;
    CHECK_FALSE(std::fabs(u7a - u7) > tol);
    // Is the intersection point correct?
    CHECK(isIntersected);
    CHECK_FALSE((pt7[0] != cube_length / 2 || pt7[1] != cube_length / 2 || pt7[2] != 0.0));

    // Angled line through cube corner
    vector3d v8s = make_vector(-1.0, -1.0, 8.0);
    vector3d v8e = make_vector(3.5, 3.5, -1.0);
    vector3d pt8;
    double u8 = 0.0;
    isIntersected = get_deepest_intersection(v8s, v8e, cubetargets, pt8, u8);
    // Check that returned u values are correct
    double s8t = distance(v8s, v8e);
    double s8 = distance(v8s, pt8);
    double u8a = s8 / s8t;
    CHECK_FALSE(std::fabs(u8a - u8) > tol);
    // Is the intersection point correct?
    CHECK(isIntersected);
    CHECK_FALSE((pt8[0] != 3.0 || pt8[1] != 3.0 || pt8[2] != 0.0));
}

    // This test covers:
    // (Directly)
    //  make_adaptive_grid
    //  compute_adaptive_voxels_in_structure
    //  to_image(adaptive_grid, float*)

    adaptive_grid_region region = adaptive_grid_region(opt_triangle_mesh, 1.0);
    adaptive_grid_region_list region_list;
    region_list.push_back(region);
    auto dose_box =
        add_margin_to_box(bounding_box(image_structure), make_vector(6.0, 6.0, 6.0));
    adaptive_grid dose_grid = compute_adaptive_grid(dose_box, dose_box, 3.0, region_list);

    SECTION("  compute_adaptive_voxels_in_structure test")
    {
        grid_cell_inclusion_info voxel_inclusion =
            compute_adaptive_voxels_in_structure(dose_grid, image_structure);

        for (size_t i = 0; i < voxel_inclusion.cells_inside.size(); ++i)
        {
            // Test the value of voxel 34649 (98 in list) (2.25, 6.75, 7.4375) is
            // just outside of the structure
            CHECK_FALSE(voxel_inclusion.cells_inside[i].index == 34649);

            // Test the value of voxel 34288 (200 in list) (6, 3.75, 8.8125), is
            // on the outside edge of the structure
            CHECK_FALSE(voxel_inclusion.cells_inside[i].index == 34288);

            // Test the value of voxel 33577 (610 in list) (5.25, 5.25, 4.6875), is
            // on the inside edge of the structure
            if (voxel_inclusion.cells_inside[i].index == 33577)
            {
                CHECK_FALSE((
                    !(voxel_inclusion.cells_inside[i].weight > 0)
                    && !(voxel_inclusion.cells_inside[i].weight < 1)));
            }

            // Test the value of voxel 33549 (637 in list) (3.75, 3.75, 4), is
            // smack in the middle of the structure
            if (voxel_inclusion.cells_inside[i].index == 33549)
            {
                CHECK_FALSE(voxel_inclusion.cells_inside[i].weight != 1);
            }

            // Test the value of voxel 4170 (819 in list) (0, 0, 4), is
            // smack in the middle of the hollow structure hole
            CHECK_FALSE(voxel_inclusion.cells_inside[i].index == 4170);

            if (isDebugGeometry)
            {
                // Debugging - used to find box index for location of box
                auto box1 = get_octree_box(dose_grid.extents, dose_grid.voxels[i].index);
                CHECK_FALSE((
                    box1.corner[0] > -1 && box1.corner[0] < 1 && box1.corner[1] > -1
                    && box1.corner[1] < 1 && box1.corner[2] > 1 && box1.corner[2] < 5));
            }
        }
    }

    std::vector<vector3d> points = get_points_on_adaptive_grid(dose_grid);
    size_t size = static_cast<size_t>(points.size());

    array<float> dose;
    auto dose_voxels = allocate(&dose, size);
    for (size_t i = 0; i < size; ++i)
    {
        double x = points[i][0];
        double y = points[i][1];
        double z = points[i][2];

        dose_voxels[i] = (float)(2.0 * x + 3.0 * y + 4.0 * z);
    }
    image3 adaptive_image = to_image(dose_grid, dose_voxels);

    if (isDebugGeometry)
    {
        // Output points list to use to determine which points to test below
        std::ofstream fs;
        fs.open("Unit_Tests.dir/points.txt");

        for (auto point : points)
        {
            fs << point << "\n";
        }
        fs.close();
    }

    unsigned ii = 0;
    auto img_is = as_const_view(cast_variant<float>(adaptive_image));
    for (unsigned k = 0; k < adaptive_image.size[2]; ++k) // Z level
    {
        double z = img_is.origin[2] + (k + 0.5) * img_is.axes[2][2];
        //std::cout << z << std::endl;
        for (unsigned j = 0; j < adaptive_image.size[1]; ++j) // Y position
        {
            double y = img_is.origin[1] + (j + 0.5) * img_is.axes[1][1];

            for (unsigned i = 0; i < adaptive_image.size[0]; ++i) // X position
            {
                double x = img_is.origin[0] + (i + 0.5) * img_is.axes[0][0];

                if (z == -6.65625)
                {
                    // Point 1: Large grid size outside structure
                    double x1 = -10.5;
                    double y1 = -10.5;
                    double v1 = -75.0;
                    if (std::fabs(x - x1) < 0.376 && std::fabs(y - y1) < 0.376)
                    {
                        double actualValue = img_is.pixels[ii];
                        CHECK_FALSE(std::fabs(actualValue - v1) > tol);
                    }
                }
                if (z == -0.46875)
                {
                    // Point 2: Small grid size inside structure
                    double x2 = -3.375;
                    double y2 = -3.375;
                    double v2 = -18.75;
                    if (std::fabs(x - x2) < 0.376 && std::fabs(y - y2) < 0.376)
                    {
                        double actualValue = img_is.pixels[ii];
                        CHECK_FALSE(std::fabs(actualValue - v2) > tol);
                    }
                    // Point 3: Medium grid size inside structure hole
                    double x3 = 1.875;
                    double y3 = -0.375;
                    double v3 = 0.75;
                    if (std::fabs(x - x3) < 0.376 && std::fabs(y - y3) < 0.376)
                    {
                        double actualValue = img_is.pixels[ii];
                        CHECK_FALSE(std::fabs(actualValue - v3) > tol);
                    }
                }

                ++ii;
            }
        }
    }

    if (isDebugGeometry)
    {
        write_vtk_file("Unit_Tests.dir/adaptive_grid.vtk", dose_grid);
        write_vtk_file("Unit_Tests.dir/adaptive_grid_points.vtk", points);
        write_vtk_file("Unit_Tests.dir/image_structure.vtk", triangle_mesh_test);
        write_vtk_file("Unit_Tests.dir/image3.vtk", adaptive_image, string("float"));
    }

    // TODO: Check corners created in adaptive grid on cubes
}

TEST_CASE("CradleGeometry point_in_polygon test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  point_in_polygon
    //  point_in_polyset

    vector2d pointsIn[4];
    vector2d pointsOut[4];

    polygon2 poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner), make_vector(sq_length, sq_length)));
    polyset polyset = make_polyset(poly);
    polygon2 hole =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    add_hole(polyset, hole);

    pointsOut[0] = make_vector(sq_corner - 0.01, sq_corner - 0.01);
    pointsOut[1] = make_vector(sq_corner - 0.01, (sq_corner / 2) - 0.01);
    pointsOut[2] = make_vector(sq_corner + sq_length + .05, sq_corner + 0.8 * sq_length);
    pointsOut[3] =
        make_vector(sq_corner + sq_length + 0.01, sq_corner + sq_length + 0.01);

    pointsIn[0] = make_vector(sq_corner + 0.1, sq_corner + 0.1);
    pointsIn[1] = make_vector(sq_corner + 0.1, sq_corner + 0.8 * sq_length);
    pointsIn[2] = make_vector(sq_corner + sq_length / 4., sq_corner + sq_length / 4.);
    pointsIn[3] = make_vector(sq_corner + 0.8 * sq_length, sq_corner + 0.1);

    // Check points_in_polygon
    for (int i = 0; i < 4; ++i)
    {
        CHECK_FALSE(point_in_polygon(pointsOut[i], poly));
        CHECK(point_in_polygon(pointsIn[i], poly));
    }

    // Check points_in_polyset
    for (int i = 0; i < 4; ++i)
    {
        CHECK_FALSE(point_in_polyset(pointsOut[i], polyset));
        CHECK(point_in_polyset(pointsIn[i], polyset));
    }
    // Check point in hole is outside polyset
    CHECK_FALSE((
        point_in_polyset(make_vector(0.0, 0.0), polyset)
        || point_in_polyset(
            make_vector(hole_corner + hole_length / 4., hole_corner + hole_length / 4.),
            polyset)));
}

TEST_CASE("CradleGeometry bounding_box test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  bounding_box(std::vector<vector<N,T> > const& points)
    //  bounding_box(structure_geometry const& structure)

    SECTION("  Test 2d bounding_box(std::vector<vector<N,T> > const& points)")
    {
        std::vector<vector<2, double>> v2;

        v2.push_back(make_vector(0.0, 0.0));
        v2.push_back(make_vector(11.75, 12.0));
        v2.push_back(make_vector(-11.75, -12.0));
        v2.push_back(make_vector(5.0, 5.0));

        box2d bb2 = bounding_box(v2);

        CHECK_FALSE(bb2.corner[0] != -11.75);
        CHECK_FALSE(bb2.corner[1] != -12);
        CHECK_FALSE(bb2.size[0] != 23.5);
        CHECK_FALSE(bb2.size[1] != 24);
    }

    SECTION("  Test 2d bounding_box(std::vector<vector<N,T> > const& points) "
        "w/o crossing origin")
    {
        std::vector<vector<2, double>> v2;

        v2.push_back(make_vector(1.0, 2.0));
        v2.push_back(make_vector(11.5, 5.0));
        v2.push_back(make_vector(5.0, 12.0));

        box2d bb2 = bounding_box(v2);

        CHECK_FALSE(bb2.corner[0] != 1.0);
        CHECK_FALSE(bb2.corner[1] != 2.0);
        CHECK_FALSE(bb2.size[0] != 10.5);
        CHECK_FALSE(bb2.size[1] != 10.0);
    }

    SECTION("  Test 3d bounding_box(std::vector<vector<N,T> > const& points)")
    {
        std::vector<vector<3, double>> v3;

        v3.push_back(make_vector(0.0, 0.0, 0.0));
        v3.push_back(make_vector(11.75, 12.0, -6.0));
        v3.push_back(make_vector(-11.75, -12.0, 6.0));
        v3.push_back(make_vector(5.0, 5.0, 3.0));

        box3d bb3 = bounding_box(v3);

        CHECK_FALSE(bb3.corner[0] != -11.75);
        CHECK_FALSE(bb3.corner[1] != -12);
        CHECK_FALSE(bb3.corner[2] != -6);
        CHECK_FALSE(bb3.size[0] != 23.5);
        CHECK_FALSE(bb3.size[1] != 24);
        CHECK_FALSE(bb3.size[2] != 12);
    }

    SECTION("  Test 3d bounding_box(structure_geometry const& structure)")
    {
        auto bb3 = bounding_box(image_structure);

        CHECK_FALSE(bb3.corner[0] != sq_corner);
        CHECK_FALSE(bb3.corner[1] != sq_corner);
        CHECK_FALSE(bb3.corner[2] != sq_start_Z_slice - 0.5 * slice_thickness);
        CHECK_FALSE(bb3.size[0] != sq_length);
        CHECK_FALSE(bb3.size[1] != sq_length);
        CHECK_FALSE(bb3.size[2] != numberOfSlices * slice_thickness);
    }
}

TEST_CASE("CradleGeometry make_cube test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_cube
    //  make_sliced_box

    auto box = make_box(make_vector(2., 3., 1.), make_vector(5., 3., 4.));

    // make_cube test
    auto cube = make_cube(make_vector(2., 3., 1.), make_vector(7., 6., 5.));

    CHECK_FALSE(cube.faces.n_elements != 12);

    auto bb = bounding_box(cube);

    CHECK_FALSE(bb != box);

    // make_sliced_box test
    double slice_spacing = 0.01;
    cradle::structure_geometry sliced_box = make_sliced_box(box, 2, slice_spacing);
    auto sliced_bb = bounding_box(sliced_box);

    CHECK(are_equal(sliced_bb.corner[0], box.corner[0], tol));
    CHECK(are_equal(sliced_bb.corner[1], box.corner[1], tol));

    // This isn't half a slice spacing because of roundoff
    CHECK(are_equal(sliced_bb.corner[2], box.corner[2], slice_spacing / 1.99));
    CHECK(are_equal(sliced_bb.size[0], box.size[0], tol));
    CHECK(are_equal(sliced_bb.size[1], box.size[1], tol));

    // This isn't double a slice spacing because of roundoff
    CHECK(are_equal(sliced_bb.size[2], box.size[2], slice_spacing * 2.01));

    if (isDebugGeometry)
    {
        auto tri = compute_triangle_mesh_from_structure(sliced_box);
        write_vtk_file("Unit_Tests.dir/sliced_box.vtk", tri);
    }
}

TEST_CASE("CradleGeometry volume/centroid test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_volume(structure_geometry)
    //  get_centroid(structure_geometry)
    // Uses a cube 'donut' structure

    auto volume = get_volume(image_structure);
    double volBox = sq_length * sq_length * (numberOfSlices * slice_thickness);
    double volHole = hole_length * hole_length * (numberOfSlices * slice_thickness);
    double expected_volume = volBox - volHole;

    CHECK(are_equal(std::fabs(volume), std::fabs(expected_volume), tol));

    auto centroid = get_centroid(image_structure);
    vector3d expected_centroid =
        make_vector(
            sq_corner + (sq_length / 2), sq_corner + (sq_length / 2),
            (sq_start_Z_slice - (slice_thickness * 0.5)) + (numberOfSlices / 2));

    CHECK(are_equal(centroid[0], expected_centroid[0]));
    CHECK(are_equal(centroid[1], expected_centroid[1]));
    CHECK(are_equal(centroid[2], expected_centroid[2]));
}

TEST_CASE("CradleGeometry make_cylinder test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_cylinder
    // (Indirectly)
    //  get_area(triangle<N,T>)

    int num_of_faces = 128;
    double radius = 10.;
    double height = 40.;

    triangle_mesh cylinder =
        make_cylinder(make_vector(0., 0., -1.0), radius, height, num_of_faces, 2);

    CHECK_FALSE(boost::numeric_cast<int>(cylinder.faces.n_elements) != num_of_faces * 4);

    // Get the total area of the end cap of the cylinder
    double area = 0;
    for (int i = 0; i < num_of_faces; i++)
    {
        area += get_area(get_triangle(cylinder, i));
    }

    // See if area matches within 1%
    double expected_area = pi * radius * radius;
    CHECK_FALSE(std::fabs((expected_area - area) / expected_area) > 0.01);

    // Check the total surface area is within 1%
    double expected_surface_area = 2. * pi * radius * height + 2. * expected_area;
    double surface_area = 0.;
    for (size_t i = 0; i < cylinder.faces.n_elements; i++)
    {
        surface_area += get_area(get_triangle(cylinder, i));
    }
    CHECK_FALSE(
        std::fabs((expected_surface_area - surface_area) / expected_surface_area) > 0.01);

    // Check bounding box of cylinder
    auto bb = bounding_box(cylinder);

    CHECK_FALSE(bb.corner[0] != -radius);
    CHECK_FALSE(bb.corner[1] != -radius);
    CHECK_FALSE(bb.corner[2] != -1.0);
    CHECK_FALSE(bb.size[0] != radius*2.0);
    CHECK_FALSE(bb.size[1] != radius*2.0);
    CHECK_FALSE(bb.size[2] != height);
}

TEST_CASE("CradleGeometry make_sphere test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_sphere
    // (Indirectly)
    //  get_area(triangle<N,T>)

    int num_of_edges = 32;
    int num_of_levels = 64;
    double radius = 10.;
    double sphere_tol = 0.005;

    triangle_mesh sphere =
        make_sphere(make_vector(0., 0., -1.), radius, num_of_edges, num_of_levels);
    if (isDebugGeometry)
    {
        write_vtk_file("Unit_Tests.dir/Sphere_Test.vtk", sphere);
    }

    // Check number of triangle faces is correct
    int expected_num_of_faces = (num_of_levels - 2) * num_of_edges * 2;
    CHECK_FALSE(
        boost::numeric_cast<int>(sphere.faces.n_elements) != expected_num_of_faces);

    // Check surface area is within 1%
    double expected_surface_area = 4.0 * pi * (radius * radius);
    double surface_area = 0.;
    for (size_t i = 0; i < sphere.faces.n_elements; i++)
    {
        surface_area += get_area(get_triangle(sphere, i));
    }
    CHECK_FALSE(
        std::fabs((expected_surface_area - surface_area) / expected_surface_area) > 0.01);

    // Check bounding box of sphere
    auto bb = bounding_box(sphere);

    CHECK(are_equal(bb.corner[0], -radius, sphere_tol));
    CHECK(are_equal(bb.corner[1], -radius, sphere_tol));
    CHECK(are_equal(bb.corner[2], -1.0 - radius, sphere_tol));
    CHECK(are_equal(bb.size[0], radius * 2.0, sphere_tol * 2));
    CHECK(are_equal(bb.size[1], radius * 2.0, sphere_tol * 2));
    CHECK(are_equal(bb.size[2], radius * 2.0, sphere_tol * 2));
}

TEST_CASE("CradleGeometry make_pyramid test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_pyramid
    // (Indirectly)
    //  get_area(triangle<N,T>)

    double width = 15.;
    double length = 12.;
    double height = 20.;

    triangle_mesh pyramid =
        make_pyramid(make_vector(-1., 0., 0.), width, length, height, 2);
    if (isDebugGeometry)
    {
        write_vtk_file("Unit_Tests.dir/Pyramid_Test.vtk", pyramid);
    }

    // Check number of triangle faces is correct
    CHECK_FALSE(pyramid.faces.n_elements != 6);

    std::vector<triangle3d> faces;
    for (size_t i = 0; i < pyramid.faces.n_elements; i++)
    {
        faces.push_back(get_triangle(pyramid, i));
    }

    // First 2 faces are bottom of pyramid
    for (unsigned i = 0; i < 2; i++)
    {
        CHECK_FALSE(get_area(faces[i]) != (width*length) / 2.);
    }

    // Front and back faces of pyramid
    double triangle_height = sqrt((length / 2.) * (length / 2.) + (height * height));
    double expected_area = (triangle_height * width) / 2.;
    for (unsigned i = 2; i < 6; i += 2)
    {
        CHECK(are_equal(get_area(faces[i]), expected_area, tol));
    }

    // Left and right faces of the pyramid
    triangle_height = sqrt((width / 2.) * (width / 2.) + (height * height));
    expected_area = (triangle_height * length) / 2.;
    for (int i = 3; i < 6; i += 2)
    {
        CHECK(are_equal(get_area(faces[i]), expected_area, tol));
    }

    // Check bounding box of pyramid
    auto bb = bounding_box(pyramid);

    CHECK_FALSE(bb.corner[0] != -1. - width / 2.);
    CHECK_FALSE(bb.corner[1] != -length / 2.);
    CHECK_FALSE(bb.corner[2] != 0.);
    CHECK_FALSE(bb.size[0] != width);
    CHECK_FALSE(bb.size[1] != length);
    CHECK_FALSE(bb.size[2] != height);
}

TEST_CASE("CradleGeometry point_in_structure test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  point_in_structure

    vector3d pointsIn[6];
    vector3d pointsOut[6];

    pointsOut[0] = make_vector(sq_corner - 0.01, sq_corner - 0.01, 0.);
    pointsOut[1] = make_vector(sq_corner - 0.01, (sq_corner / 2.) - 0.01, 0.);
    pointsOut[2] =
        make_vector(sq_corner + sq_length + .05, sq_corner + 0.8 * sq_length, 0.);
    pointsOut[3] =
        make_vector(sq_corner + sq_length + 0.01, sq_corner + sq_length + 0.01, 0.);
    pointsOut[4] = make_vector(sq_corner + 0.1, sq_corner + 0.1, -1.5);
    pointsOut[5] = make_vector(sq_corner + 0.1, sq_corner + 0.1, 10.75);

    pointsIn[0] = make_vector(sq_corner + 0.1, sq_corner + 0.1, 0.);
    pointsIn[1] = make_vector(sq_corner + 0.1, sq_corner + 0.8 * sq_length, 0.);
    pointsIn[2] = make_vector(sq_corner + sq_length / 4., sq_corner + sq_length / 4., 1.);
    pointsIn[3] = make_vector(sq_corner + 0.8 * sq_length, sq_corner + 0.1, 0.);
    pointsIn[4] = make_vector(sq_corner + 0.1, sq_corner + 0.1, -0.4);
    pointsIn[5] = make_vector(sq_corner + 0.1, sq_corner + 0.1, 8.9);

    // Check points_in_structure
    for (int i = 0; i < 6; ++i)
    {
        CHECK_FALSE(point_in_structure(pointsOut[i], image_structure));
        CHECK(point_in_structure(pointsIn[i], image_structure));
    }

    // Check point in hole is outside structure
    CHECK_FALSE((
        point_in_structure(make_vector(0.0, 0.0, 0.0), image_structure)
        || point_in_structure(
            make_vector(hole_corner + hole_length / 4., hole_corner + hole_length / 4., 1.0),
            image_structure)));
}

TEST_CASE("CradleGeometry scale_polygon/polyset test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  scale_polygon
    //  scale_polyset
    //  polyset_expansion

    double scale_factor = 1.5;
    double expand_distance = 2.0;

    polygon2 poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner), make_vector(sq_length, sq_length)));
    polyset polyset = make_polyset(poly);
    polygon2 hole =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    add_hole(polyset, hole);

    polygon2 scaled_poly = scale(poly, scale_factor);
    cradle::polyset scaled_polyset = scale(polyset, scale_factor);
    cradle::polyset expanded_polyset = polyset_expansion(polyset, expand_distance);
    cradle::polyset expanded_polyset_neg = polyset_expansion(polyset, -expand_distance);

    auto bb_poly = bounding_box(poly);
    auto bb_scaled_poly = bounding_box(scaled_poly);
    auto bb_polyset = bounding_box(polyset);
    auto bb_scaled_polyset = bounding_box(scaled_polyset);
    auto bb_hole = bounding_box(hole);
    auto bb_scaled_hole = bounding_box(scaled_polyset.holes[0]);
    auto bb_expanded = bounding_box(expanded_polyset);
    auto bb_expanded_neg = bounding_box(expanded_polyset_neg);

    // Scaled Polygon bounding box
    CHECK_FALSE(bb_poly.corner[0] != bb_scaled_poly.corner[0] / scale_factor);
    CHECK_FALSE(bb_poly.corner[1] != bb_scaled_poly.corner[1] / scale_factor);
    CHECK_FALSE(bb_poly.size[0] != bb_scaled_poly.size[0] / scale_factor);
    CHECK_FALSE(bb_poly.size[1] != bb_scaled_poly.size[1] / scale_factor);

    // Scaled Polyset bounding box
    CHECK_FALSE(bb_polyset.corner[0] != bb_scaled_polyset.corner[0] / scale_factor);
    CHECK_FALSE(bb_polyset.corner[1] != bb_scaled_polyset.corner[1] / scale_factor);
    CHECK_FALSE(bb_polyset.size[0] != bb_scaled_polyset.size[0] / scale_factor);
    CHECK_FALSE(bb_polyset.size[1] != bb_scaled_polyset.size[1] / scale_factor);

    // Scaled Polyset Hole bounding box
    CHECK_FALSE(bb_hole.corner[0] != bb_scaled_hole.corner[0] / scale_factor);
    CHECK_FALSE(bb_hole.corner[1] != bb_scaled_hole.corner[1] / scale_factor);
    CHECK_FALSE(bb_hole.size[0] != bb_scaled_hole.size[0] / scale_factor);
    CHECK_FALSE(bb_hole.size[1] != bb_scaled_hole.size[1] / scale_factor);

    // Expanded Polyset bounding box
    CHECK_FALSE(bb_polyset.corner[0] != bb_expanded.corner[0] + expand_distance);
    CHECK_FALSE(bb_polyset.corner[1] != bb_expanded.corner[1] + expand_distance);
    CHECK_FALSE(bb_polyset.size[0] != bb_expanded.size[0] - expand_distance * 2);
    CHECK_FALSE(bb_polyset.size[1] != bb_expanded.size[1] - expand_distance * 2);
    // Check for hole in expanded polyset (currently holes are removed)
    CHECK_FALSE(expanded_polyset.holes.size() != 0);

    // Negative Expanded Polyset bounding box
    CHECK_FALSE(bb_polyset.corner[0] != bb_expanded_neg.corner[0] - expand_distance);
    CHECK_FALSE(bb_polyset.corner[1] != bb_expanded_neg.corner[1] - expand_distance);
    CHECK_FALSE(bb_polyset.size[0] != bb_expanded_neg.size[0] + expand_distance * 2);
    CHECK_FALSE(bb_polyset.size[1] != bb_expanded_neg.size[1] + expand_distance * 2);
    // Check for hole in expanded polyset (currently holes are removed)
    CHECK_FALSE(expanded_polyset_neg.holes.size() != 0);
}

TEST_CASE("CradleGeometry distance_to_polyset test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  distance_to_polyset

    polygon2 poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner), make_vector(sq_length, sq_length)));
    polyset polyset = make_polyset(poly);
    polygon2 hole =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    add_hole(polyset, hole);

    // points inside of polyset (points inside polyset should have a negative distance)
    auto pt1 = make_vector(hole_corner - .1, 0.);
    auto pt2 = make_vector(-sq_corner, 0.);
    auto pt3 = make_vector(hole_corner - 1., -0.125);

    // points outside of polyset
    // (points outside the polyset should have a positive distance)
    auto pt4 = make_vector(0., 0.);
    auto pt5 = make_vector(0.1, 0.);
    auto pt6 = make_vector(sq_corner - 0.5, 0.);
    auto pt7 = make_vector(sq_corner - 0.5, sq_corner - 0.5);
    auto pt8 = make_vector(-sq_corner + 1.5, -sq_corner + 1.25);
    auto pt9 = make_vector(-sq_corner + 0.001, 2.);

    auto dist1 = distance_to_polyset(pt1, polyset);
    auto dist2 = distance_to_polyset(pt2, polyset);
    auto dist3 = distance_to_polyset(pt3, polyset);
    auto dist4 = distance_to_polyset(pt4, polyset);
    auto dist5 = distance_to_polyset(pt5, polyset);
    auto dist6 = distance_to_polyset(pt6, polyset);
    auto dist7 = distance_to_polyset(pt7, polyset);
    auto dist8 = distance_to_polyset(pt8, polyset);
    auto dist9 = distance_to_polyset(pt9, polyset);

    CHECK(are_equal(dist1, -0.1, tol));
    CHECK(are_equal(dist2, 0., tol));
    CHECK(are_equal(dist3, -1., tol));
    CHECK(are_equal(dist4, 2., tol));
    CHECK(are_equal(dist5, 1.9, tol));
    CHECK(are_equal(dist6, 0.5, tol));
    CHECK(are_equal(dist7, sqrt((.5*.5) + (.5*.5)), tol));
    CHECK(are_equal(dist8, sqrt((1.25*1.25) + (1.5*1.5)), tol));
    CHECK(are_equal(dist9, 0.001, 0.00001));
}

TEST_CASE("CradleGeometry polyset_combination test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  polyset_combination

    // polygon 1
    polygon2 poly =
        as_polygon(
            make_box(
                make_vector(sq_corner, sq_corner), make_vector(sq_length, sq_length)));
    polyset polyset = make_polyset(poly);
    polygon2 hole =
        as_polygon(
            make_box(
                make_vector(hole_corner, hole_corner),
                make_vector(hole_length, hole_length)));
    add_hole(polyset, hole);

    // polygon 2
    polygon2 poly2 =
        as_polygon(
            make_box(
                make_vector(sq_corner * 2, sq_corner * 2),
                make_vector(sq_length, sq_length)));

    // create polyset list
    std::vector<cradle::polyset> poly_list;
    poly_list.push_back(polyset);
    poly_list.push_back(make_polyset(poly2));

    SECTION("  Test Union")
    {
        cradle::polyset poly_union =
            polyset_combination(cradle::set_operation::UNION, poly_list);

        CHECK_FALSE(poly_union.holes.size() != 1);
        CHECK_FALSE(poly_union.polygons.size() != 1);
        CHECK_FALSE(poly_union.polygons[0].vertices.size() != 8);
        double area = get_area(poly_union);
        double expected_area =
            (((2 * sq_corner) * (2 * sq_corner)) * 2)   // poly area x 2
            - (sq_corner * sq_corner)                   // intersecting area
            - ((hole_corner * 2) * (hole_corner * 2))   // hole area
            + (hole_corner * hole_corner);              // poly2 overlap inside hole
        CHECK_FALSE(area != expected_area);
        CHECK_FALSE(
            bounding_box(poly_union)
            != make_box(make_vector(sq_corner * 2, sq_corner * 2),
                make_vector(1.5 * sq_length, 1.5 * sq_length)));
    }

    SECTION("  Test Intersection")
    {
        cradle::polyset poly_intersection =
            polyset_combination(cradle::set_operation::INTERSECTION, poly_list);

        CHECK_FALSE(poly_intersection.holes.size() != 0);
        CHECK_FALSE(poly_intersection.polygons.size() != 1);
        CHECK_FALSE(poly_intersection.polygons[0].vertices.size() != 6);
        double area = get_area(poly_intersection);
        // Area explanation    |  1/4 of poly area   | - |     1/4 hole area       |
        double expected_area = (sq_corner * sq_corner) - (hole_corner * hole_corner);
        CHECK_FALSE(area != expected_area);
        CHECK_FALSE(
            bounding_box(poly_intersection)
            != make_box(make_vector(sq_corner, sq_corner),
                make_vector(-sq_corner, -sq_corner)));
    }

    SECTION("  Test Difference")
    {
        cradle::polyset poly_difference =
            polyset_combination(cradle::set_operation::DIFFERENCE, poly_list);

        CHECK_FALSE(poly_difference.holes.size() != 0);
        CHECK_FALSE(poly_difference.polygons.size() != 1);
        CHECK_FALSE(poly_difference.polygons[0].vertices.size() != 10);
        double area = get_area(poly_difference);
        double expected_area =
            ((2 * sq_corner) * (2 * sq_corner))                 // poly area
            - (sq_corner * sq_corner)                           // intersecting area
            - ((hole_corner * 2) * (hole_corner * 2) * 0.75);   // 3/4  hole area
        CHECK_FALSE(area != expected_area);
        CHECK_FALSE(bounding_box(poly_difference) != bounding_box(polyset));

        if (isDebugGeometry)
        {
            // debug output of polygon shape
            for (auto vert : poly_difference.polygons[0].vertices)
            {
                std::cout << vert << std::endl;
            }
        }
    }

    SECTION("  Test XOR")
    {
        cradle::polyset poly_xor =
            polyset_combination(cradle::set_operation::XOR, poly_list);

        CHECK_FALSE(poly_xor.holes.size() != 1);
        CHECK_FALSE(poly_xor.polygons.size() != 2);
        CHECK_FALSE(poly_xor.polygons[0].vertices.size() != 8);
        CHECK_FALSE(poly_xor.polygons[1].vertices.size() != 6);
        CHECK_FALSE(poly_xor.holes[0].vertices.size() != 6);
        double area = get_area(poly_xor);
        double expected_area =
            ((2 * sq_corner) * (2 * sq_corner))             // poly area
            - (sq_corner * sq_corner)                       // intersecting area
            - ((hole_corner * 2) * (hole_corner * 2) * 0.5) // 1/2  hole area
            + (((2 * sq_corner) * (2 * sq_corner)) * 0.75); // 3/4 poly2 area
        CHECK_FALSE(area != expected_area);
        CHECK_FALSE(
            bounding_box(poly_xor)
            != make_box(
                make_vector(sq_corner * 2, sq_corner * 2),
                make_vector(-sq_corner * 3, -sq_corner * 3)));

        if (isDebugGeometry)
        {
            // debug output of polygon shape
            for (size_t j = 0; j < poly_xor.polygons.size(); j++)
            {
                std::cout << "Polyline " << j << std::endl << std::endl;
                for (size_t i = 0; i < poly_xor.polygons[j].vertices.size(); i++)
                {
                    std::cout << poly_xor.polygons[j].vertices[i][0] << " "
                        << poly_xor.polygons[j].vertices[i][1] << std::endl;
                }
            }
            // debug output of hole shape
            for (size_t j = 0; j < poly_xor.holes.size(); j++)
            {
                std::cout << "holes " << j << std::endl << std::endl;
                for (size_t i = 0; i < poly_xor.holes[j].vertices.size(); i++)
                {
                    std::cout << poly_xor.holes[j].vertices[i][0] << " "
                        << poly_xor.holes[j].vertices[i][1] << std::endl;
                }
            }
        }
    }
}

TEST_CASE("CradleGeometry make_sliced_box test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_sliced_box

    double slice_spacing = 1.;
    double side_length = 50.;
    auto box =
        make_box(
            make_vector(-25., -25., -25.),
            make_vector(side_length, side_length, side_length));
    auto sliced_b = make_sliced_box(box, 2, slice_spacing);

    CHECK_FALSE(sliced_b.slices.size() != (50. / slice_spacing) + 1);

    auto poly = get_slice(sliced_b, get_center(box)[2]);
    auto expected_area = side_length * side_length;

    CHECK(are_equal(expected_area, get_area(poly), tol));

    auto bb = bounding_box(sliced_b);

    CHECK(are_equal(bb.corner[0], box.corner[0], tol));
    CHECK(are_equal(bb.corner[1], box.corner[1], tol));
    // This isn't half a slice spacing because of 1% shift in slice function
    CHECK(
        are_equal(
            box.corner[2] - bb.corner[2], slice_spacing / 2.0, 0.015 * slice_spacing));
    CHECK(are_equal(bb.size[0], box.size[0], tol));
    CHECK(are_equal(bb.size[1], box.size[1], tol));
    // This can be up to 1/2 slice spacing off due to the slicing method used
    CHECK_FALSE(std::fabs(bb.size[2] - box.size[2]) > 0.515 * slice_spacing);
}

TEST_CASE("CradleGeometry make_sliced_sphere/cylinder test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_sliced_sphere
    //  make_sliced_cylinder

    double slice_spacing = 1.0;
    int num_of_faces = 128;
    double radius = 10.;
    double height = radius * 2;

    auto box =
        make_box(
            make_vector(-radius, -radius, 0.),
            make_vector(radius * 2, radius * 2, height));

    // Test cylinder
    auto sliced_c =
        make_sliced_cylinder(
            make_vector(0., 0., 0.), radius, height, num_of_faces, 2, 2, 1);
    auto poly_c = get_slice(sliced_c, height / 2.);
    auto expected_area = pi * (radius * radius);
    auto expected_vol_c = expected_area * height;
    auto bb_c = bounding_box(sliced_c);

    CHECK_FALSE(sliced_c.slices.size() != (height / slice_spacing) + 1);

    // area is within 1/2%
    CHECK(are_equal(expected_area, get_area(poly_c), expected_area * 0.005));

    // volume is within 1/2%
    CHECK(are_equal(expected_vol_c, get_volume(sliced_c), expected_vol_c * 0.005));

    // Begin bounding box check
    CHECK(are_equal(bb_c.corner[0], box.corner[0], tol));
    CHECK(are_equal(bb_c.corner[1], box.corner[1], tol));
    CHECK(
        are_equal(
            box.corner[2] - bb_c.corner[2],
            slice_spacing / 2.0,
            0.015 * slice_spacing)); // This isn't half a slice spacing because of 1% shift in slice function
    CHECK(are_equal(bb_c.size[0], box.size[0], tol));
    CHECK(are_equal(bb_c.size[1], box.size[1], tol));
    CHECK_FALSE(
        std::fabs(bb_c.size[2] - box.size[2])
        > 0.515 * slice_spacing); // This can be up to 1/2 slice spacing off due to the slicing method used


    // Test sphere
    auto sliced_s =
        make_sliced_sphere(make_vector(0., 0., height / 2), radius, num_of_faces, 2, 1);
    auto poly_s = get_slice(sliced_s, height / 2.);
    auto expected_vol_s = expected_area * 4 / 3 * radius;
    auto bb_s = bounding_box(sliced_s);

    CHECK_FALSE(sliced_s.slices.size() != (height / slice_spacing) + 1);

    // area is within 1/2%
    CHECK(are_equal(expected_area, get_area(poly_s), expected_area * 0.005));

    // volume is within 1/2%
    CHECK(are_equal(expected_vol_s, get_volume(sliced_s), expected_vol_s * 0.005));

    // Begin bounding box check
    CHECK(are_equal(bb_s.corner[0], box.corner[0], tol));
    CHECK(are_equal(bb_s.corner[1], box.corner[1], tol));
    CHECK(
        are_equal(
            box.corner[2] - bb_s.corner[2],
            slice_spacing / 2.0, 0.015 * slice_spacing)); // This isn't half a slice spacing because of 1% shift in slice function
    CHECK(are_equal(bb_s.size[0], box.size[0], 0.0016));
    CHECK(are_equal(bb_s.size[1], box.size[1], 0.0016));
    CHECK_FALSE(
        std::fabs(bb_s.size[2] - box.size[2])
        > 0.515 * slice_spacing); // This can be up to 1/2 slice spacing off due to the slicing method used
}

TEST_CASE("CradleGeometry make_sliced_pyramid test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_sliced_pyramid

    double slice_spacing = 1.0;
    double width = 15.;
    double length = 12.;
    double height = 20.;

    auto box =
        make_box(
            make_vector(-length / 2, -width / 2, 0.), make_vector(length, width, height));

    auto sliced_p =
        make_sliced_pyramid(make_vector(0., 0., 0.), length, width, height, 2, 2, 1.);
    auto poly_p = get_slice(sliced_p, height / 2);
    auto expected_area = 0.25 * (length * width);

    // Added half a slice thickness to height for expected volume calc
    auto expected_vol_p = (length * width * (height + 0.5)) / 3;
    auto bb_p = bounding_box(sliced_p);

    if (isDebugGeometry)
    {
        auto mesh = compute_triangle_mesh_from_structure(sliced_p);
        write_vtk_file("Unit_Tests.dir/pyramid.vtk", mesh);
    }

    // Pyramid as a structure is "stair cased". Tolerance are loosened to account for this.
    double pyramid_tol = 0.008;
    CHECK_FALSE(sliced_p.slices.size() != (height / slice_spacing) + 1);

    // area is within 1/2%
    CHECK(are_equal(expected_area, get_area(poly_p), expected_area * 0.005));

    // volume is within 5%
    CHECK(are_equal(expected_vol_p, get_volume(sliced_p), expected_vol_p * 0.05));

    // Begin bounding box check
    CHECK(are_equal(bb_p.corner[0], box.corner[0], pyramid_tol));
    CHECK(are_equal(bb_p.corner[1], box.corner[1], pyramid_tol));
    CHECK(
        are_equal(
            box.corner[2] - bb_p.corner[2],
            slice_spacing / 2.0,
            0.015 * slice_spacing)); // This isn't half a slice spacing because of 1% shift in slice function
    CHECK(are_equal(bb_p.size[0], box.size[0], pyramid_tol));
    CHECK(are_equal(bb_p.size[1], box.size[1], pyramid_tol));
    CHECK_FALSE(
        std::fabs(bb_p.size[2] - box.size[2])
        > 0.515 * slice_spacing); // This can be up to 1/2 slice spacing off due to the slicing method used
}

TEST_CASE("CradleGeometry make_sliced_parallelepiped test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  make_sliced_parallelepiped

    double width = 10.;
    double height = 10.;

    double slice_spacing = 1.0;
    auto corner = make_vector(0., 0., 0.);
    auto a = make_vector(0., width, 0.);
    auto b = make_vector(width, 0., 0.);
    auto c = make_vector(width / 2, 0., height);
    auto u = corner - c;
    auto v = corner - b;
    auto w = corner - a;

    auto sliced_p = make_sliced_parallelepiped(corner, a, b, c, 2, slice_spacing);
    auto box = make_box(corner, make_vector(width + (width / 2), width, height));

    // Find area
    double expected_area = width * width; // Roughly
    auto poly_pp = get_slice(sliced_p, height / 2.);
    double area_pp = get_area(poly_pp);

    // Find volume
    auto expected_vol_p = std::fabs(dot(u, cross(v, w))); // U.(V x W)
    auto volum = get_volume(sliced_p);

    auto bb_p = bounding_box(sliced_p);

    if (isDebugGeometry)
    {
        auto mesh = compute_triangle_mesh_from_structure(sliced_p);
        write_vtk_file("Unit_Tests.dir/parallelepiped1.vtk", mesh);
    }

    CHECK_FALSE(sliced_p.slices.size() != (height / slice_spacing) + 1);
    CHECK(are_equal(expected_area, area_pp, tol));
    CHECK(are_equal(expected_vol_p, volum, tol));

    // Begin bounding box check
    CHECK(are_equal(bb_p.corner[0], box.corner[0], 0.005));
    CHECK(are_equal(bb_p.corner[1], box.corner[1], tol));
    CHECK(
        are_equal(
            box.corner[2] - bb_p.corner[2],
            slice_spacing / 2.0, 0.015 * slice_spacing)); // This isn't half a slice spacing because of 1% shift in slice function
    CHECK(are_equal(bb_p.size[0], box.size[0], 0.5));
    CHECK(are_equal(bb_p.size[1], box.size[1], tol));
    CHECK_FALSE(
        std::fabs(bb_p.size[2] - box.size[2])
        > 0.515 * slice_spacing); // This can be up to 1/2 slice spacing off due to the slicing method used
}

TEST_CASE("CradleGeometry get_points_on_grid test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_points_on_grid

    double xsize = 10.;
    double ysize = 20.;
    double zsize = 12.;
    double xspacing = 0.5;
    double yspacing = 1.0;
    double zspacing = 0.6;

    // 2d grid
    auto grid =
        make_grid_for_box(
            make_box(make_vector(0., 0.), make_vector(xsize, ysize)),
            make_vector(xspacing, yspacing));
    auto pts = get_points_on_grid(grid);

    double expected_point_count = (xsize / xspacing) * (ysize / yspacing);

    CHECK(are_equal(expected_point_count, (double)pts.size(), 0.));

    int ii = 0;
    for (int i = 0; i < ysize / yspacing; i++)
    {
        for (int j = 0; j < xsize / xspacing; j++)
        {
            double px = (j * xspacing) + (xspacing / 2);
            double py = (i * yspacing) + (yspacing / 2);
            CHECK(are_equal(pts[ii], make_vector(px, py), tol));
            ++ii;
        }
    }

    // 3d grid
    auto grid_z =
        make_grid_for_box(
            make_box(make_vector(0., 0., 0.), make_vector(xsize, ysize, zsize)),
            make_vector(xspacing, yspacing, zspacing));
    auto pts_z = get_points_on_grid(grid_z);

    expected_point_count = (xsize / xspacing) * (ysize / yspacing) * (zsize / zspacing);

    CHECK(are_equal(expected_point_count, (double)pts_z.size(), 0.));

    ii = 0;
    for (int k = 0; k < zsize / zspacing; k++)
    {
        for (int i = 0; i < ysize / yspacing; i++)
        {
            for (int j = 0; j < xsize / xspacing; j++)
            {
                double px = (j + 0.5) * xspacing;
                double py = (i + 0.5) * yspacing;
                double pz = (k + 0.5) * zspacing;
                CHECK(are_equal(pts_z[ii], make_vector(px, py, pz), tol));
                ++ii;
            }
        }
    }
}

TEST_CASE("CradleGeometry matrix_product test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  matrix_product
    // (Indirectly)
    //  make_matrix

    // matrix 1
    matrix<3, 3, double> mat1 = make_matrix(1., 2., 3.,
        4., 5., 6.,
        7., 8., 9.);

    // matrix 2
    matrix<3, 3, double> mat2 = make_matrix(7., 9., 8.,
        6., 4., 5.,
        2., 1., 3.);
    // array matrix 1
    double array_mat1[3][3] = { 1., 2., 3.,
        4., 5., 6.,
        7., 8., 9. };
    // array matrix 2
    double array_mat2[3][3] = { 7., 9., 8.,
        6., 4., 5.,
        2., 1., 3. };

    matrix<3, 3, double> product = matrix_product(mat1, mat2);

    // Create the expected product by multiplying the array matrices
    double expected_product[3][3];
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            double sum = 0.;
            for (int k = 0; k < 3; k++)
            {
                sum += array_mat1[i][k] * array_mat2[k][j];
            }
            expected_product[i][j] = sum;
        }
    }

    // Compare matrix product to the expected product
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            CHECK(product(i, j) == expected_product[i][j]);
        }
    }
}

TEST_CASE("CradleGeometry matrix_inverse test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  matrix_inverse
    // (Indirectly)
    //  make_matrix

    // matrix
    matrix<3, 3, double> mat = make_matrix(7., 9., 8.,
        6., 4., 5.,
        2., 1., 3.);
    // array matrix
    double array_mat[3][3] = { 7., 9., 8.,
        6., 4., 5.,
        2., 1., 3. };

    matrix<3, 3, double> inverse = matrix_inverse(mat);

    // find determinant of array matrix
    double det =
        array_mat[0][0] * (array_mat[1][1] * array_mat[2][2] - array_mat[1][2] * array_mat[2][1])
        - array_mat[0][1] * (array_mat[1][0] * array_mat[2][2] - array_mat[2][0] * array_mat[1][2])
        + array_mat[0][2] * (array_mat[1][0] * array_mat[2][1] - array_mat[2][0] * array_mat[1][1]);

    // find cofactor of array matrix
    double cofactor[3][3] =
        { array_mat[1][1] * array_mat[2][2] - array_mat[1][2] * array_mat[2][1],
        -1 * (array_mat[1][0] * array_mat[2][2] - array_mat[2][0] * array_mat[1][2]),
        array_mat[1][0] * array_mat[2][1] - array_mat[2][0] * array_mat[1][1],
        -1 * (array_mat[0][1] * array_mat[2][2] - array_mat[2][1] * array_mat[0][2]),
        array_mat[0][0] * array_mat[2][2] - array_mat[2][0] * array_mat[0][2],
        -1 * (array_mat[0][0] * array_mat[2][1] - array_mat[2][0] * array_mat[0][1]),
        array_mat[0][1] * array_mat[1][2] - array_mat[1][1] * array_mat[0][2],
        -1 * (array_mat[0][0] * array_mat[1][2] - array_mat[1][0] * array_mat[0][2]),
        array_mat[0][0] * array_mat[1][1] - array_mat[1][0] * array_mat[0][1] };

    // transpose the cofactor and divide by determinant to find inverse of array matrix
    double expected_inverse[3][3];
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            expected_inverse[i][j] = cofactor[j][i] / det;
        }
    }

    // compare matrix inverse to expected inverse
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            CHECK(inverse(i, j) == expected_inverse[i][j]);
        }
    }
}

TEST_CASE("CradleGeometry structure_combination test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  structure_combination

    double side_length = 5.;
    double expected_area1 = side_length * side_length;
    double expected_area2 = 2. * side_length * side_length - 6.;
    double expected_area3 = 6.;
    double expected_area4 = side_length * side_length - 6.;
    double expected_area5 = 2. * side_length * side_length - 12.;

    // Structure 1
    auto poly1 =
        as_polygon(
            make_box(make_vector(1., 1.), make_vector(side_length, side_length)));
    polyset polyset1 = make_polyset(poly1);

    slice_description_list master_slices;
    for (double i = 0.; i < 5 + side_length; i++)
    {
        master_slices.push_back(slice_description(i, 1.));
    }

    structure_polyset_list slice_list1;
    for (double i = 0.; i < side_length; i++)
    {
        slice_list1[i] = polyset1;
    }
    auto structure1 = structure_geometry(slice_list1, master_slices);

    // Structure 2
    polygon2 poly2 =
        as_polygon(
            make_box(make_vector(3., 4.), make_vector(side_length, side_length)));
    polyset polyset2 = make_polyset(poly2);

    structure_polyset_list slice_list2;
    for (double i = 0.; i < side_length; i++)
    {
        slice_list2[3. + i] = polyset2;
    }
    auto structure2 = structure_geometry(slice_list2, master_slices);

    // Structure list
    std::vector<structure_geometry> structures;
    structures.push_back(structure1);
    structures.push_back(structure2);

    // Test Union
    auto structure_union = structure_combination(set_operation::UNION, structures);

    for (size_t i = 0; i < structure_union.slices.size(); i++)
    {
        double pos = double(i);
        CHECK_FALSE((
            structure_union.slices[pos].polygons.size() != 1 ||
            structure_union.slices[pos].holes.size() != 0));
        if (pos != 3. && pos != 4.)
        {
            CHECK_FALSE(structure_union.slices[pos].polygons[0].vertices.size() != 4);
            CHECK(
                are_equal(
                    get_area(structure_union.slices[pos]),
                    expected_area1, 0.001 * expected_area1));
        }
        else if ((pos == 3. || pos == 4.) &&
            structure_union.slices[pos].polygons[0].vertices.size() != 8)
        {
            CHECK_FALSE(structure_union.slices[pos].polygons[0].vertices.size() != 8);
            CHECK(
                are_equal(
                    get_area(structure_union.slices[pos]),
                    expected_area2, 0.001 * expected_area2));
        }
    }

    // Test Intersection
    auto structure_intersection =
        structure_combination(set_operation::INTERSECTION, structures);

    for (size_t i = 0; i < structure_intersection.slices.size(); i++)
    {
        double pos = double(3 + i);
        CHECK_FALSE((
            structure_intersection.slices[pos].polygons.size() != 1 ||
            structure_intersection.slices[pos].holes.size() != 0));
        CHECK_FALSE(structure_intersection.slices[pos].polygons[0].vertices.size() != 4);
        CHECK(
            are_equal(
                get_area(structure_intersection.slices[pos]),
                expected_area3, 0.001 * expected_area3));
    }

    // Test Difference
    auto structure_difference =
        structure_combination(set_operation::DIFFERENCE, structures);

    for (size_t i = 0; i < structure_difference.slices.size(); i++)
    {
        double pos = double(i);
        CHECK_FALSE((
            structure_difference.slices[pos].polygons.size() != 1 ||
            structure_difference.slices[pos].holes.size() != 0));
        CHECK_FALSE((
            (pos != 3. && pos != 4.) &&
            structure_difference.slices[pos].polygons[0].vertices.size() != 4));
        CHECK_FALSE((
            (pos == 3. || pos == 4.) &&
            structure_difference.slices[pos].polygons[0].vertices.size() != 6));
        CHECK_FALSE((
            (pos != 3. && pos != 4.) &&
            !are_equal(
                get_area(structure_difference.slices[pos]),
                expected_area1, 0.001 * expected_area1)));
        CHECK_FALSE(((pos == 3. || pos == 4.) &&
            !are_equal(
                get_area(structure_difference.slices[pos]),
                expected_area4, 0.001 * expected_area4)));
    }

    // Test XOR
    auto structure_xor = structure_combination(set_operation::XOR, structures);

    for (auto & item : structure_xor.slices)
    {
        if (item.first == 3. || item.first == 4.)
        {
            CHECK_FALSE((
                item.second.polygons.size() != 2 || item.second.holes.size() != 0));
            CHECK_FALSE((
                item.second.polygons[0].vertices.size() +
                item.second.polygons[1].vertices.size()) != 12);
            CHECK(
                are_equal(get_area(item.second), expected_area5, 0.001 * expected_area5));
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0));
            CHECK_FALSE(item.second.polygons[0].vertices.size() != 4);
            CHECK(
                are_equal(get_area(item.second), expected_area1, 0.001 * expected_area1));
        }
    }
}

TEST_CASE("CradleGeometry structure_expansion test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  structure_2d_expansion
    //  structure_3d_expansion
    // (Indirectly)
    //  distance_to_polyset

    double rect_corner = 3.;
    double rect_lengthx = 5.;
    double rect_lengthy = 4.;

    // Expand a rectangular box in 2d
    polygon2 poly1 =
        as_polygon(
            make_box(
                make_vector(rect_corner, rect_corner),
                make_vector(rect_lengthx, rect_lengthy)));
    polyset polyset1 = make_polyset(poly1);

    slice_description_list master_slices;
    for (double i = -3.; i < 8.; i++)
    {
        master_slices.push_back(slice_description(i, 1.));
    }

    structure_polyset_list slice_list1;
    for (double i = 0.; i < 5.; i++)
    {
        slice_list1[i] = polyset1;
    }
    auto structure1 = structure_geometry(slice_list1, master_slices);

    double amount = 2.;
    auto expansion1 = structure_2d_expansion(structure1, amount);
    CHECK_FALSE(expansion1.slices.size() < 5);

    for (auto const& item : expansion1.slices)
    {
        if (item.first < 0 || item.first >= 5)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));

            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure1.slices[1.]),
                        amount, tol));
            }
        }
    }

    // Contract a rectangular box in 2d
    double amount2 = -1.0;
    auto contraction1 = structure_2d_expansion(structure1, amount2);
    for (auto const& item : contraction1.slices)
    {
        if (item.first < 0 || item.first >= 5)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure1.slices[1.]),
                        amount2, tol));
            }
        }
    }

    // Expand a concave polygon in 2d
    std::vector<vector<2, double> > v2;
    v2.push_back(make_vector(15., 3.));
    v2.push_back(make_vector(18., 3.));
    v2.push_back(make_vector(18., 8.));
    v2.push_back(make_vector(15., 8.));
    v2.push_back(make_vector(15., 6.));
    v2.push_back(make_vector(17., 6.));
    v2.push_back(make_vector(17., 5.));
    v2.push_back(make_vector(15., 5.));
    polygon2 poly2 = make_polygon2(v2);
    polyset polyset2 = make_polyset(poly2);

    structure_polyset_list slice_list2;
    for (double i = 0.; i < 5.; i++)
    {
        slice_list2[i] = polyset2;
    }
    auto structure2 = structure_geometry(slice_list2, master_slices);

    double amount3 = 0.75;
    auto expansion2 = structure_2d_expansion(structure2, amount3);

    for (auto const& item : expansion2.slices)
    {
        if (item.first < 0 || item.first >= 5)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure2.slices[1.]),
                        amount3, tol));
            }
        }
    }

    // This expansion result is more concave
    double amount3_2 = 0.25;
    auto expansion2_2 = structure_2d_expansion(structure2, amount3_2);

    for (auto const& item : expansion2_2.slices)
    {
        if (item.first < 0 || item.first >= 5)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure2.slices[1.]),
                        amount3_2, tol));
            }
        }
    }

    // Contract a concave polygon in 2d
    double amount4 = -0.25;
    auto contraction2 = structure_2d_expansion(structure2, amount4);
    for (auto const& item : contraction2.slices)
    {
        if (item.first < 0 || item.first >= 5)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure2.slices[1.]),
                        amount4, tol));
            }
        }
    }

    // Expand a rectangular box in 3d
    double amount5 = 2.1;
    auto expansion3 = structure_3d_expansion(structure1, amount5);

    for (auto const& item : expansion3.slices)
    {
        if (item.first < -2.1 || item.first >= 7.1)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                if ((item.first >= 0. && item.first < 5.))
                {
                    CHECK(
                        are_equal(
                            distance_to_polyset(
                                item.second.polygons[0].vertices[j], structure1.slices[1.]),
                            amount5, tol));
                }
                else
                {
                    CHECK_FALSE(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure1.slices[1.])
                        > amount5);
                }
            }
        }
    }

    // Contract a rectangular box in 3d
    double amount6 = -1.4;
    auto contraction3 = structure_3d_expansion(structure1, amount6);

    for (auto const& item : contraction3.slices)
    {
        if (item.first < 1 || item.first >= 4)
        {
            CHECK_FALSE(item.second.polygons.size() != 0);
        }
        else
        {
            CHECK_FALSE((
                item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
                item.second.polygons[0].vertices.size() == 0));
            for (size_t j = 0; j < item.second.polygons[0].vertices.size(); j++)
            {
                CHECK(
                    are_equal(
                        distance_to_polyset(
                            item.second.polygons[0].vertices[j], structure1.slices[1.]),
                        amount6, tol));
            }
        }
    }
}

TEST_CASE("CradleGeometry triangulate_polyset test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  triangulate_polyset
    // (Indirectly)
    //  get_area(triangle<2,T>)

    // Triangulate a concave rectangular polyset
    std::vector<vector<2, double> > v;
    v.push_back(make_vector(2., 2.));
    v.push_back(make_vector(5., 2.));
    v.push_back(make_vector(5., 7.));
    v.push_back(make_vector(2., 7.));
    v.push_back(make_vector(2., 5.));
    v.push_back(make_vector(4., 5.));
    v.push_back(make_vector(4., 4.));
    v.push_back(make_vector(2., 4.));
    polygon2 poly1 = make_polygon2(v);
    polyset polyset1 = make_polyset(poly1);

    auto tri1 = triangulate_polyset(polyset1);

    auto expected_area1 = get_area(polyset1);
    double area1 = 0.;
    for (size_t i = 0; i < tri1.size(); i++)
    {
        area1 += get_area(tri1[i]);
    }

    CHECK(are_equal(area1, expected_area1, tol * expected_area1));

    // Triangulate a circle
    auto cir = circle<double>(make_vector(10., 4.), 3.);
    polygon2 poly2 = as_polygon(cir, 128);
    polyset polyset2 = make_polyset(poly2);

    auto tri2 = triangulate_polyset(polyset2);

    auto expected_area2 = get_area(polyset2);
    double area2 = 0.;
    for (size_t i = 0; i < tri2.size(); i++)
    {
        area2 += get_area(tri2[i]);
    }

    CHECK(are_equal(area2, expected_area2, tol * expected_area2));

    // Triangulate a circle with a square hole
    auto cir2 = circle<double>(make_vector(20., 4.), 3.);
    polygon2 poly3 = as_polygon(cir2, 128);
    polyset polyset3 = make_polyset(poly3);
    polygon2 hole = as_polygon(make_box(make_vector(19., 3.), make_vector(2., 2.)));
    add_hole(polyset3, hole);

    auto tri3 = triangulate_polyset(polyset3);

    auto expected_area3 = get_area(polyset3);
    double area3 = 0.;
    for (size_t i = 0; i < tri3.size(); i++)
    {
        area3 += get_area(tri3[i]);
    }

    CHECK(are_equal(area3, expected_area3, tol * expected_area3));
}

TEST_CASE("CradleGeometry mesh_as_structure test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  mesh_as_structure
    // (Indirectly)
    //  get_area(polyset)
    //  make_cube
    //  make_sphere
    //  make_pyramid

    // Mesh cube as structure
    double cu_origin = 2.;
    double cu_extent = 7.;
    double expected_area1 = (cu_extent - cu_origin) * (cu_extent - cu_origin);
    auto mesh_cube =
        make_cube(
            make_vector(cu_origin, cu_origin, cu_origin - 0.0001),
            make_vector(cu_extent, cu_extent, cu_extent + 0.0001));

    slice_description_list master_slices;
    for (double i = 2.; i < 7.; i++)
    {
        master_slices.push_back(slice_description(i, 1.));
    }

    auto struct_cube = mesh_as_structure(mesh_cube, 2, master_slices);

    for (auto const& item : struct_cube.slices)
    {
        CHECK_FALSE((
            item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
            item.second.polygons[0].vertices.size() == 0));
        CHECK(are_equal(get_area(item.second), expected_area1, tol * expected_area1));
    }

    // Mesh sphere as structure
    int num_of_edges = 256;
    int num_of_levels = 256;
    double radius = 4.01; // Avoid slicing right end start/end points

    auto mesh_sphere =
        make_sphere(make_vector(15., 5., 5.), radius, num_of_edges, num_of_levels);

    slice_description_list master_slices2;
    for (double i = 1.; i < 10.; i++)
    {
        master_slices2.push_back(slice_description(i, 1.));
    }
    auto struct_sphere = mesh_as_structure(mesh_sphere, 2, master_slices2);

    for (auto const& item : struct_sphere.slices)
    {
        CHECK_FALSE((
            item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
            item.second.polygons[0].vertices.size() == 0));
        double r = sqrt((radius * radius) - (item.first - 5.) * (item.first - 5.));
        double expected_area2 = pi * r * r;
        CHECK(are_equal(get_area(item.second), expected_area2, 0.01 * expected_area2));
    }

    // Mesh pyramid as structure
    double width = 6.;
    double length = 4.;
    double height = 6.003;
    double base_position = 0.998;

    auto mesh_pyramid =
        make_pyramid(make_vector(25., 5., base_position), width, length, height, 2);

    slice_description_list master_slices3;
    for (double i = 1.; i < 8.; i++)
    {
        master_slices3.push_back(slice_description(i, 1.));
    }
    auto struct_pyramid = mesh_as_structure(mesh_pyramid, 2, master_slices3);

    for (auto const& item : struct_pyramid.slices)
    {
        CHECK_FALSE((
            item.second.polygons.size() != 1 || item.second.holes.size() != 0 ||
            item.second.polygons[0].vertices.size() == 0));
        double e_width =
            width - (width / height) * (item.first - base_position);
        double e_length = length - (length / height) * (item.first - base_position);
        double expected_area3 = e_width * e_length;
        CHECK(are_equal(get_area(item.second), expected_area3, tol * expected_area3));
    }
}

TEST_CASE("CradleGeometry connect_line_segments test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  connect_line_segments

    // Connect 2 line segments
    auto line1 = make_line_segment(make_vector(1., 1.), make_vector(4., 3.));
    auto line2 = make_line_segment(make_vector(4., 3.), make_vector(8., 2.));
    auto line3 = make_line_segment(make_vector(9., 3.), make_vector(10., 5.));

    std::vector<line_segment<2, double> > lines;
    lines.push_back(line1);
    lines.push_back(line2);
    lines.push_back(line3);

    auto connection = connect_line_segments(lines, tol);
    CHECK_FALSE(connection.size() != 2);
    CHECK_FALSE(connection[0].vertices.size() != 3);

    // Connect line segments using a higher tolerance
    auto v1 = make_vector(1., 4.);
    auto v2 = make_vector(3., 7.);
    auto v3 = make_vector(1., 3.99);
    auto v4 = make_vector(5., 5.);
    auto line4 = make_line_segment(v1, v2);
    auto line5 = make_line_segment(v3, v4);
    auto line6 = make_line_segment(make_vector(3., 7.2), make_vector(1., 8.));

    std::vector<line_segment<2, double> > lines2;
    lines2.push_back(line4);
    lines2.push_back(line5);
    lines2.push_back(line6);

    auto connection2 = connect_line_segments(lines2, 0.1);
    CHECK_FALSE(connection2.size() != 2);
    CHECK_FALSE(connection2[0].vertices.size() != 3);
    for (size_t i = 0; i < connection2[0].vertices.size(); i++)
    {
        CHECK_FALSE((
            connection2[0].vertices[i] != v1 && connection2[0].vertices[i] != v2
            && connection2[0].vertices[i] != v3 && connection2[0].vertices[i] != v4));
    }
}

TEST_CASE("CradleGeometry sample(interpolated_function) test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  sample(interpolated_function const& f, double x)

    // Find samples of a straight line function
    array<function_sample> samples1;
    size_t size = 10;
    auto sample1_values = allocate(&samples1, size);
    for (size_t i = 0; i < size; ++i)
    {
        double val = 2. * i - 1.;
        sample1_values[i] = function_sample(val, 2.);
    }

    auto func1 =
        interpolated_function(0., 1., samples1, outside_domain_policy::ALWAYS_ZERO);
    auto s1 = sample(func1, 1.);
    auto s2 = sample(func1, 2.5);
    auto s3 = sample(func1, 7.25);
    auto s4 = sample(func1, 12.); //outside domain policy should set this to 0

    CHECK_FALSE(s1 != (2. * 1. - 1.));
    CHECK_FALSE(s2 != (2. * 2.5 - 1.));
    CHECK_FALSE(s3 != (2. * 7.25 - 1.));
    CHECK_FALSE(s4 != 0.);

    // Find samples of a sine function
    array<function_sample> samples2;
    auto sample2_values = allocate(&samples2, size);
    for (size_t i = 0; i < size; ++i)
    {
        double val2 = sin(i);
        sample2_values[i] = function_sample(val2, sin(i + 1) - val2);
    }

    auto func2 =
        interpolated_function(0., 1., samples2, outside_domain_policy::EXTEND_WITH_COPIES);
    auto s5 = sample(func2, 1.);
    auto s6 = sample(func2, 5.5);
    auto s7 = sample(func2, 8.75);
    auto s8 = sample(func2, 13.);   // outside domain policy should set this to
                                    // last sample value + last sample delta

    CHECK(are_equal(s5, sin(1.), tol));
    CHECK(are_equal(s6, sin(5) + 0.5 * (sin(6) - sin(5)), tol));
    CHECK(are_equal(s7, sin(8) + 0.75 * (sin(9) - sin(8)), tol));
    CHECK(are_equal(s8, sin(10.), tol));
}


TEST_CASE("CradleGeometry project_meshes_via_render_to_texture test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  project_meshes_via_render_to_texture

    // Test a single mesh cube
    auto cube1 =
        make_cube(
            make_vector(-5., -5., -5.),
            make_vector(5., 5., 5.));
    auto display_surface1 = slice(bounding_box(cube1), 2);
    auto center1 = make_vector(0., 0., 0.);
    auto direction = make_vector(0., 0., -1.);
    auto sad = make_vector(2000., 2000.);
    auto up = make_vector(0., 1., 0.);
    auto cube1_bb = bounding_box(cube1);

    auto msv =
        multiple_source_view(
            center1,
            display_surface1,
            direction,
            sad,
            up);

    std::vector<triangle_mesh> meshes;
    meshes.push_back(cube1);

    auto projections =
        project_meshes_via_render_to_texture(
            cube1_bb, meshes, msv, 0.);

    CHECK_FALSE(projections.size() != 1);

    auto projection_bb = bounding_box(projections[0]);

    CHECK_FALSE((
        !are_equal(projection_bb.corner[0], display_surface1.corner[0], 0.01) ||
        !are_equal(projection_bb.corner[1], display_surface1.corner[1], 0.01)));

    CHECK_FALSE((
        !are_equal(projection_bb.size[0], display_surface1.size[0], 0.01) ||
        !are_equal(projection_bb.size[1], display_surface1.size[1], 0.01)));

    auto projection_area = get_area(projections[0]);
    auto display_area = product(display_surface1.size);

    CHECK(are_equal(projection_area, display_area, tol * display_area));

    // Test two adjacent mesh cubes
    auto cube2 =
        make_cube(
            make_vector(-5., 5., -5.),
            make_vector(10., 7., 5.));
    auto cube2_bb = bounding_box(cube2);
    meshes.push_back(cube2);

    auto bounds2 =
        make_box(
            cube1_bb.corner,
            get_high_corner(cube2_bb) - cube1_bb.corner);
    auto display_surface2 =
        make_box(
            display_surface1.corner,
            get_high_corner(slice(cube2_bb, 2)) - display_surface1.corner);

    auto msv2 =
        multiple_source_view(
            center1,
            display_surface2,
            direction,
            sad,
            up);

    auto projections2 =
        project_meshes_via_render_to_texture(
            bounds2, meshes, msv2, 0.);

    CHECK_FALSE(projections2.size() != 2);

    auto combined_projections2 =
        polyset_combination(set_operation::UNION, projections2);
    auto projections2_bb = bounding_box(combined_projections2);

    auto point_out1 =
        make_vector(get_high_corner(display_surface1)[0] + 1.,
            slice(cube2_bb, 2).corner[1] - 1.);

    auto point_in1 =
        make_vector(get_high_corner(display_surface1)[0] - 1.,
            slice(cube2_bb, 2).corner[1] + 1.);

    CHECK_FALSE(is_inside(combined_projections2, point_out1));

    CHECK(is_inside(combined_projections2, point_in1));

    CHECK_FALSE((
        !are_equal(projections2_bb.corner[0], display_surface2.corner[0], 0.01) ||
        !are_equal(projections2_bb.corner[1], display_surface2.corner[1], 0.01)));

    CHECK_FALSE((
        !are_equal(projections2_bb.size[0], display_surface2.size[0], 0.01) ||
        !are_equal(projections2_bb.size[1], display_surface2.size[1], 0.01)));

    auto projections2_area =
        get_area(projections2[0]) + get_area(projections2[1]);
    auto display_area2 = display_area + product(slice(cube2_bb, 2).size);

    CHECK(are_equal(projections2_area, display_area2, tol * display_area2));

    // Test two overlapping mesh cubes
    auto cube3 =
        make_cube(
            make_vector(-10., -5., 5.),
            make_vector(10., -2., 15.));
    auto cube3_bb = bounding_box(cube3);

    std::vector<triangle_mesh> meshes2;
    meshes2.push_back(cube1);
    meshes2.push_back(cube3);

    auto bounds3 =
        make_box(
            make_vector(cube3_bb.corner[0], cube1_bb.corner[1],
                cube1_bb.corner[2]),
            make_vector(cube3_bb.size[0], cube1_bb.size[1],
                cube1_bb.size[2] + cube3_bb.size[2]));

    auto display_surface3 = slice(bounds3, 2);

    auto msv3 =
        multiple_source_view(
            center1,
            display_surface3,
            direction,
            sad,
            up);

    auto projections3 =
        project_meshes_via_render_to_texture(
            bounds3, meshes2, msv3, 0.);

    CHECK_FALSE(projections3.size() != 2);

    auto combined_projections3 =
        polyset_combination(set_operation::UNION, projections3);
    auto projections3_bb = bounding_box(combined_projections3);

    auto point_out2 =
        make_vector(display_surface1.corner[0] - 1.,
            get_high_corner(slice(cube3_bb, 2))[1] + 1.);

    auto point_in2 =
        make_vector(display_surface1.corner[0] + 1.,
            get_high_corner(slice(cube3_bb, 2))[1] - 1.);

    CHECK_FALSE(is_inside(combined_projections3, point_out2));

    CHECK(is_inside(combined_projections3, point_in2));

    CHECK_FALSE((
        !are_equal(projections3_bb.corner[0], display_surface3.corner[0], 0.01) ||
        !are_equal(projections3_bb.corner[1], display_surface3.corner[1], 0.01)));

    CHECK_FALSE((
        !are_equal(projections3_bb.size[0], display_surface3.size[0], 0.01) ||
        !are_equal(projections3_bb.size[1], display_surface3.size[1], 0.01)));

    auto projections3_area =
        get_area(projections3[0]) + get_area(projections3[1]);
    auto display_area3 = display_area + product(slice(cube3_bb, 2).size)
        - (display_surface1.size[0] * slice(cube3_bb, 2).size[1]);

    CHECK(are_equal(projections3_area, display_area3, tol * display_area3));

    // Test from different direction with scaled projection plane
    auto cube4 =
        make_cube(
            make_vector(0., 0., 0.),
            make_vector(5., 5., 5.));
    auto display_surface4 =
        make_box(
            make_vector(0., 0.), // corner won't shift
            make_vector(5. * 3. / 2., 5 * 4. / 3.)); // size grows with SAD
    auto center4 = make_vector(0., 0., 0.);
    auto direction4 = make_vector(-1., 0., 0.);
    auto sad4 = make_vector(200., 300.);
    auto up4 = make_vector(0., 0., 1.);
    auto cube4_bb = bounding_box(cube4);

    auto msv4 =
        multiple_source_view(
            center4,
            display_surface4,
            direction4,
            sad4,
            up4);

    std::vector<triangle_mesh> meshes4;
    meshes4.push_back(cube4);

    auto projections4 =
        project_meshes_via_render_to_texture(
            cube4_bb, meshes4, msv4, -100.);

    CHECK_FALSE(projections4.size() != 1);

    auto projection_bb4 = bounding_box(projections4[0]);

    CHECK_FALSE((
        !are_equal(projection_bb4.corner[0], display_surface4.corner[0], 0.01) ||
        !are_equal(projection_bb4.corner[1], display_surface4.corner[1], 0.01)));

    CHECK_FALSE((
        !are_equal(projection_bb4.size[0], display_surface4.size[0], 0.01) ||
        !are_equal(projection_bb4.size[1], display_surface4.size[1], 0.01)));

    auto projection_area4 = get_area(projections4[0]);
    auto display_area4 = product(display_surface4.size);

    CHECK(are_equal(projection_area4, display_area4, tol * display_area4));
}

TEST_CASE("CradleGeometry get_structure_slices test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  get_structure_slices

    // Create a structure with polygons only on middle five slices
    polygon2 poly1 = as_polygon(make_box(make_vector(2., 2.), make_vector(4., 3.)));
    polyset polyset1 = make_polyset(poly1);

    structure_polyset_list slice_list1;
    slice_description_list master_slices1;
    for (int i = 0; i < 15.; i++)
    {
        if (i >= 5 && i < 10)
        {
            slice_list1[2. + i] = polyset1;
        }
        master_slices1.push_back(slice_description(2. + i, 1.));
    }

    auto structure1 = structure_geometry(slice_list1, master_slices1);

    // Limits should return nothing
    auto slices1 = get_structure_slices(structure1, -5., 1.);
    CHECK(slices1.size() == 0);

    // Limits should return everything
    auto slices2 = get_structure_slices(structure1, 0., 15.9);
    CHECK(slices2.size() == 15);

    // Limits should return middle 4 slices
    auto slices3 = get_structure_slices(structure1, 7.9, 11.4);
    CHECK(slices3.size() == 4);

    for (size_t i = 0; i < slices3.size(); ++i)
    {
        CHECK(slices3[i].position == (8. + i));
        CHECK(slices3[i].region.polygons.size() == 1);
    }
}

TEST_CASE("CradleGeometry overlapping test", "[cradle][geometry]")
{
    // This test covers:
    // (Directly)
    //  overlapping(box3d, structure_geometry, unsigned, optional<box3d>)

    // Make a concave structure
    std::vector<vector<2, double> > v2;
    v2.push_back(make_vector(8., 2.));
    v2.push_back(make_vector(10., 2.));
    v2.push_back(make_vector(10., 4.));
    v2.push_back(make_vector(12., 4.));
    v2.push_back(make_vector(12., 2.));
    v2.push_back(make_vector(14., 2.));
    v2.push_back(make_vector(14., 6.));
    v2.push_back(make_vector(8., 6.));
    polygon2 poly2 = make_polygon2(v2);
    polyset polyset2 = make_polyset(poly2);

    structure_polyset_list slice_list2;
    slice_description_list master_slices2;
    auto structure_height2 = 5.;
    for (double i = 2.; i < structure_height2 + 2.; i++)
    {
        slice_list2[i] = polyset2;
        master_slices2.push_back(slice_description(i, 1.));
    }

    auto structure2 = structure_geometry(slice_list2, master_slices2);
    auto st_bounds = bounding_box(structure2);

    // Check a box with partial overlap
    auto box_with_overlap =
        make_box(
            make_vector(7., 3., master_slices2[1].position),
            make_vector(2., 2., 2.));

    // Check using sg_bounds and not using sg_bounds
    CHECK(overlapping(box_with_overlap, structure2, 2u, none));
    CHECK(overlapping(box_with_overlap, structure2, 2u, st_bounds));

    // Check a box that doesn't overlap (z)
    auto box_without_overlap1 =
        make_box(
            make_vector(7., 3., master_slices2[0].position - 10.),
            make_vector(2., 2., 2.));

    // Check using sg_bounds and not using sg_bounds
    CHECK_FALSE(overlapping(box_without_overlap1, structure2, 2u, none));
    CHECK_FALSE(overlapping(box_without_overlap1, structure2, 2u, st_bounds));

    // Check a box that doesn't overlap (x/y)
    auto box_without_overlap2 =
        make_box(
            make_vector(5., -1., master_slices2[1].position),
            make_vector(2., 2., 2.));

    // Check using sg_bounds and not using sg_bounds
    CHECK_FALSE(overlapping(box_without_overlap2, structure2, 2u, none));
    CHECK_FALSE(overlapping(box_without_overlap2, structure2, 2u, st_bounds));

    // Check a box that's fully inside
    auto contained_box =
        make_box(
            make_vector(9., 4.5, master_slices2[2].position),
            make_vector(2., 1., 2.));

    // Check using sg_bounds and not using sg_bounds
    CHECK(overlapping(contained_box, structure2, 2u, none));
    CHECK(overlapping(contained_box, structure2, 2u, st_bounds));

    // Check a box that contains the whole structure
    auto containing_box =
        make_box(
            make_vector(7., 1., master_slices2[0].position - 1.),
            make_vector(12., 8., structure_height2 + 3.));

    // Check using sg_bounds and not using sg_bounds
    CHECK(overlapping(containing_box, structure2, 2u, none));
    CHECK(overlapping(containing_box, structure2, 2u, st_bounds));

    // Check a box that doesn't overlap due to concavity
    auto box_without_overlap3 =
        make_box(
            make_vector(10.5, 1., master_slices2[3].position),
            make_vector(1., 2., 3.));

    // Check using sg_bounds and not using sg_bounds
    CHECK_FALSE(overlapping(box_without_overlap3, structure2, 2u, none));
    CHECK_FALSE(overlapping(box_without_overlap3, structure2, 2u, st_bounds));
}

    // This test covers:
    // (Directly)
    //  slice_structure_along_different_axis

    // Slice a structure box along the x axis
    auto poly_corner1 = make_vector(2., 2.);
    auto poly_size1 = make_vector(4., 6.);
    polygon2 poly1 = as_polygon(make_box(poly_corner1, poly_size1));
    polyset polyset1 = make_polyset(poly1);

    structure_polyset_list slice_list1;
    slice_description_list master_slices1;
    auto structure_height1 = 5.;
    for (double i = 0; i < structure_height1; ++i)
    {
        slice_list1[2. + i] = polyset1;
        master_slices1.push_back(slice_description(2. + i, 1.));
    }

    auto structure1 = structure_geometry(slice_list1, master_slices1);

    std::vector<double> slice_positions1;
    slice_positions1.push_back(2.5);
    slice_positions1.push_back(3.5);
    slice_positions1.push_back(4.5);
    slice_positions1.push_back(5.5);

    auto new_structure1 =
        slice_structure_along_different_axis(structure1, 0, slice_positions1);

    auto expected_area1 = poly_size1[1] * structure_height1;
    for (auto slice : new_structure1.slices)
    {
        CHECK(get_area(slice.second) == expected_area1);
    }

    CHECK(get_volume(structure1) == get_volume(new_structure1));

    // Slice an L-shaped structure along the y axis
    auto poly_corner2 = make_vector(2., 8.);
    auto poly_size2 = make_vector(8., 4.);
    polygon2 poly2 = as_polygon(make_box(poly_corner2, poly_size2));

    std::vector<cradle::polyset> poly_list;
    poly_list.push_back(polyset1);
    poly_list.push_back(make_polyset(poly2));
    auto polyset2 = polyset_combination(cradle::set_operation::UNION, poly_list);

    structure_polyset_list slice_list2;
    slice_description_list master_slices2;
    auto structure_height2 = 10.;
    for (double i = 0; i < structure_height2; ++i)
    {
        slice_list2[2. + i] = polyset2;
        master_slices2.push_back(slice_description(2. + i, 1.));
    }

    auto structure2 = structure_geometry(slice_list2, master_slices2);

    std::vector<double> slice_positions2;
    slice_positions2.push_back(3.);
    slice_positions2.push_back(5.);
    slice_positions2.push_back(7.);
    slice_positions2.push_back(9.);
    slice_positions2.push_back(11.);
    slice_positions2.push_back(13.);// Test a position that is outside the bounds

    auto new_structure2 =
        slice_structure_along_different_axis(structure2, 1, slice_positions2);

    CHECK(new_structure2.slices.size() == 5);

    auto expected_area2_1 = poly_size1[0] * structure_height2;
    auto expected_area2_2 = poly_size2[0] * structure_height2;
    unsigned ii = 0;
    for (auto slice : new_structure2.slices)
    {
        if (ii < 3)
        {
            CHECK(get_area(slice.second) == expected_area2_1);
        }
        else
        {
            CHECK(get_area(slice.second) == expected_area2_2);
        }
        ++ii;
    }

    // Slice a concave structure along y axis and check for holes in new structure
    std::vector<vector<2, double> > v3;
    v3.push_back(make_vector(8., 2.));
    v3.push_back(make_vector(10., 2.));
    v3.push_back(make_vector(10., 4.));
    v3.push_back(make_vector(12., 4.));
    v3.push_back(make_vector(12., 2.));
    v3.push_back(make_vector(14., 2.));
    v3.push_back(make_vector(14., 6.));
    v3.push_back(make_vector(8., 6.));
    polygon2 poly3 = make_polygon2(v3);
    polyset polyset3 = make_polyset(poly3);

    structure_polyset_list slice_list3;
    slice_description_list master_slices3;
    auto structure_height3 = 8.;
    for (double i = 0.; i < structure_height3; ++i)
    {
        slice_list3[2. + i] = polyset3;
        master_slices3.push_back(slice_description(2. + i, 1.));
    }

    auto structure3 = structure_geometry(slice_list3, master_slices3);

    std::vector<double> slice_positions3;
    slice_positions3.push_back(2.5);
    slice_positions3.push_back(3.5);
    slice_positions3.push_back(4.5);
    slice_positions3.push_back(5.5);

    auto new_structure3 =
        slice_structure_along_different_axis(structure3, 1, slice_positions3);

    auto expected_area3 = structure_height3 * 6.;
    unsigned jj = 0;
    for (auto slice : new_structure3.slices)
    {
        if (jj < 2)
        {
            CHECK(slice.second.holes.size() == 1);
            CHECK(get_area(slice.second) == (2. / 3.) * expected_area3);
        }
        else
        {
            CHECK(slice.second.holes.size() == 0);
            CHECK(get_area(slice.second) == expected_area3);
        }
        ++jj;
    }
}


}