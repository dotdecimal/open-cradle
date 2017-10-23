#include "catch.hpp"

#include <cradle/common.hpp>
#include <cradle/geometry/line_strip.hpp>
#include <cradle/geometry/slice_mesh.hpp>
#include <cradle/imaging/api.hpp>
#include <cradle/imaging/inclusion_image.hpp>
#include <cradle/imaging/projection.hpp>
#include <cradle/imaging/statistics.hpp>
#include <cradle/imaging/isolines.hpp>
#include <cradle/imaging/isobands.hpp>
#include <cradle/imaging/binary_ops.hpp>
#include <cradle/io/vtk_io.hpp>
#include <cradle/unit_tests/testing.hpp>

#include <cradle_test_utilities.hpp>

using namespace cradle;

namespace unittests
{

bool isDebugImaging = false;
cradle::image<3, double, cradle::shared> hollow_pantom_image;
image<3, double, cradle::shared> imaging_dose_override_inside_image;
image<3, double, cradle::shared> imaging_stopping_power_image;
structure_geometry imaging_structure;

cradle::image<3, double, cradle::shared> get_hollow_phantom_image()
{
    regular_grid3d grid =
        make_grid_for_box(
            make_box(
                make_vector(ph_image_corner, ph_image_corner, ph_image_corner),
                make_vector(ph_image_length, ph_image_length, ph_image_length)),
            make_vector(ph_pixel_spacing, ph_pixel_spacing, ph_pixel_spacing));
    unsigned count = grid.n_points[0] * grid.n_points[1] * grid.n_points[2];
    double *data = new double[count];
    int ii = 0;
    for (unsigned k = 0; k < grid.n_points[2]; ++k)
    {
        for (unsigned j = 0; j < grid.n_points[1]; ++j)
        {
            for (unsigned i = 0; i < grid.n_points[0]; ++i)
            {
                // Set value to 0 when inside hole
                if ((k >= 12 && k < ph_image_length - 4) &&
                    (j >= 8 && j < 12) &&
                    (i >= 8 && i < 12))
                {
                    data[ii++] = 0;
                }
                // Set value to 1 when inside structure
                else if ((k > 8 && k < 19) &&
                    (j > 3 && j < 16) &&
                    (i > 3 && i < 16))
                {
                    data[ii++] = 1;
                }
                // Set value to 0 when outside structure
                else
                {
                    data[ii++] = 0;
                }
            }
        }
    }
    image<3, double, unique> data_image;
    create_image(data_image, grid.n_points, data);
    set_value_mapping(data_image, 0, 1, millimeters);
    set_spatial_mapping(data_image, grid.p0 - 0.5 * grid.spacing, grid.spacing);
    auto shared_image = share(data_image);
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/hollow_image.vtk", shared_image);
    }
    return shared_image;
}

cradle::image<3, double, cradle::shared> get_projection_image()
{
    regular_grid3d grid =
        make_grid_for_box(
            make_box(
                make_vector(ph_image_corner, ph_image_corner, ph_image_corner),
                make_vector(ph_image_length, ph_image_length, ph_image_length)),
            make_vector(ph_pixel_spacing, ph_pixel_spacing, ph_pixel_spacing));
    unsigned count = grid.n_points[0] * grid.n_points[1] * grid.n_points[2];
    double *data = new double[count];
    int ii = 0;
    for (unsigned k = 0; k < grid.n_points[2]; ++k)
    {
        for (unsigned j = 0; j < grid.n_points[1]; ++j)
        {
            for (unsigned i = 0; i < grid.n_points[0]; ++i)
            {
                // Set value to 0 when inside hole
                if ((k >= 12 && k < ph_image_length - 4) &&
                    (j >= 8 && j < 12) &&
                    (i >= 8 && i < 12))
                {
                    data[ii++] = 0.5;
                }
                // Set value to 1 when inside structure
                else if ((k > 8 && k < 19) &&
                    (j > 3 && j < 16) &&
                    (i > 3 && i < 16))
                {
                    data[ii++] = 1;
                }
                // Set value to 0 when outside structure
                else
                {
                    data[ii++] = 0;
                }
            }
        }
    }
    image<3, double, unique> data_image;
    create_image(data_image, grid.n_points, data);
    set_value_mapping(data_image, 0, 1, millimeters);
    set_spatial_mapping(data_image, grid.p0 - 0.5 * grid.spacing, grid.spacing);
    auto shared_image = share(data_image);
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/proj_image.vtk", shared_image);
    }
    return shared_image;
}

void initialize_imaging()
{
    hollow_pantom_image = get_hollow_phantom_image();
}

TEST_CASE("CradleImaging density_override test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  override_image_inside_structure
    //  override_image_outside_structure

    initialize_imaging();

    image<3, double, cradle::shared> image_inside =
        override_image_inside_structure(
            imaging_stopping_power_image, imaging_structure, override_value, 0.9f);
    image<3, double, cradle::shared> image_outside =
        override_image_outside_structure(
            imaging_stopping_power_image, imaging_structure, override_value, 0.9f);
    imaging_dose_override_inside_image = image_inside;
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/Override_image_inside.vtk", image_inside);
        write_vtk_file("Unit_Tests.dir/Override_image_outside.vtk", image_outside);
        write_vtk_file(
            "Unit_Tests.dir/Override_image_orig_struct.vtk",
            compute_triangle_mesh_from_structure(imaging_structure));
    }

    auto img_is = as_const_view(image_inside);
    auto img_os = as_const_view(image_outside);

    // Test override_image_inside
    unsigned ii = 0;
    for (unsigned k = 0; k < image_inside.size[2]; ++k) // Z level
    {
        for (unsigned j = 0; j < image_inside.size[1]; ++j) // Y position
        {
            for (unsigned i = 0; i < image_inside.size[0]; ++i) // X position
            {
                // Z level: Loop steps inside if the current level is one where the
                // structured was defined to be on
                // otherwise make sure the pixel value is unchanged
                double z = (k + 0.5) * ph_pixel_spacing + ph_image_corner;
                if ((z >= sq_start_Z_position) && (z <= sq_end_Z_position))
                {
                    // J column: loop steps inside if the current column is where the
                    // structure was defined to be in
                    // otherwise make sure the pixel value is unchanged
                    double y = (j + 0.5) * ph_pixel_spacing + ph_image_corner;
                    if ((y >= sq_start_xy_position) && (y < sq_end_xy_position))
                    {
                        // I row: loop steps inside if the current row is where the
                        // structure was defined to be in
                        // otherwise make sure the pixel value is unchanged
                        double x = (i + 0.5) * ph_pixel_spacing + ph_image_corner;
                        if ((x >= sq_start_xy_position) && (x < sq_end_xy_position))
                        {
                            // Is the current point inside the defined hole
                            if ((y >= hole_start_xy_position) && (y < hole_end_xy_position) &&
                                (x >= hole_start_xy_position) && (x < hole_end_xy_position))
                            {
                                // Pixel should be unchanged
                                if (img_is.pixels[ii] != ph_image_value)
                                {
                                    CHECK_FALSE(img_is.pixels[ii] != ph_image_value);
                                }
                            }
                            // Current point is within the defined structure,
                            // should have been changed to override value
                            else if (img_is.pixels[ii] != override_value)
                            {
                                CHECK_FALSE(img_is.pixels[ii] != override_value);
                            }
                        }
                        else if (img_is.pixels[ii] != ph_image_value)
                        {
                            CHECK_FALSE(img_is.pixels[ii] != ph_image_value);
                        }
                    }
                    else if (img_is.pixels[ii] != ph_image_value)
                    {
                        CHECK_FALSE(img_is.pixels[ii] != ph_image_value);
                    }
                }
                else if (img_is.pixels[ii] != ph_image_value)
                {
                    CHECK_FALSE(img_is.pixels[ii] != ph_image_value);
                }

                // Increment to the next pixel
                ++ii;
            }
        }
    }

    // Test override_image_outside
    ii = 0;
    for (unsigned k = 0; k < image_inside.size[2]; ++k) // Z level
    {
        for (unsigned j = 0; j < image_inside.size[1]; ++j) // Y position
        {
            for (unsigned i = 0; i < image_inside.size[0]; ++i) // X position
            {
                // Z level: Loop steps inside if the current level is one where the
                // structured was defined to be on
                // otherwise make sure the pixel value is unchanged
                double z = (k + 0.5) * ph_pixel_spacing + ph_image_corner;
                if ((z >= sq_start_Z_position) && (z <= sq_end_Z_position))
                {
                    // J column: loop steps inside if the current column is where the
                    // structure was defined to be in
                    // otherwise make sure the pixel value is unchanged
                    double y = (j + 0.5) * ph_pixel_spacing + ph_image_corner;
                    if ((y >= sq_start_xy_position) && (y < sq_end_xy_position))
                    {
                        // I row: loop steps inside if the current row is where the
                        // structure was defined to be in
                        // otherwise make sure the pixel value is unchanged
                        double x = (i + 0.5) * ph_pixel_spacing + ph_image_corner;
                        if ((x >= sq_start_xy_position) && (x < sq_end_xy_position))
                        {
                            // Is the current point inside the defined hole
                            if ((y >= hole_start_xy_position) && (y < hole_end_xy_position) &&
                                (x >= hole_start_xy_position) && (x < hole_end_xy_position))
                            {
                                // Pixel should be changed
                                if (img_os.pixels[ii] != override_value)
                                {
                                    CHECK_FALSE(img_os.pixels[ii] != override_value);
                                }
                            }
                            // Current point is within the defined structure, should not
                            // have been changed to override value
                            else if (img_os.pixels[ii] != ph_image_value)
                            {
                                CHECK_FALSE(img_os.pixels[ii] != ph_image_value);
                            }
                        }
                        else if (img_os.pixels[ii] != override_value)
                        {
                            CHECK_FALSE(img_os.pixels[ii] != override_value);
                        }
                    }
                    else if (img_os.pixels[ii] != override_value)
                    {
                        CHECK_FALSE(img_os.pixels[ii] != override_value);
                    }
                }
                else if (img_os.pixels[ii] != override_value)
                {
                    CHECK_FALSE(img_os.pixels[ii] != override_value);
                }

                // Increment to the next pixel
                ++ii;
            }
        }
    }
}

TEST_CASE("CradleImaging image_min_max test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  image_min_max
    //  image_list_min_max

    // Test image_min_max
    cradle::min_max<double> expected_min_max = min_max<double>(0., 1.);
    auto min_max = get(image_min_max(hollow_pantom_image));

    CHECK(are_equal(min_max.min, expected_min_max.min, tol));
    CHECK(are_equal(min_max.max, expected_min_max.max, tol));

    // Scale the image too
    expected_min_max = cradle::min_max<double>(0., 10.);
    hollow_pantom_image.value_mapping.slope *= 10.;

    // Test image_list_min_max
    std::vector<image<3, double, cradle::shared>> image_list;
    image_list.push_back(imaging_stopping_power_image);
    image_list.push_back(hollow_pantom_image);

    min_max = get(image_list_min_max(image_list));

    CHECK(are_equal(min_max.min, expected_min_max.min, tol));
    CHECK(are_equal(min_max.max, expected_min_max.max, tol));

    // Return image back to original state
    hollow_pantom_image.value_mapping.slope *= 0.1;
}

TEST_CASE("CradleImaging image_bounding_box test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  image_bounding_box

    hollow_pantom_image.origin[0] += -1.0;
    hollow_pantom_image.origin[1] += -2.0;
    auto image_bb = image_bounding_box(as_variant(hollow_pantom_image));
    auto expected_bb =
        make_box(
            make_vector(ph_image_corner - 1.0, ph_image_corner - 2.0, ph_image_corner),
            make_vector(ph_image_length, ph_image_length, ph_image_length));

    CHECK(image_bb == expected_bb);

    // Return image to original state
    hollow_pantom_image.origin[0] += 1.0;
    hollow_pantom_image.origin[1] += 2.0;
}

TEST_CASE("CradleImaging image_histogram test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  image_histogram

    int num_of_values = 12;
    int bin_size = 1;
    unsigned num_of_bins = 10;

    image<1, double, unique> imagedvh;
    double *data = new double[num_of_values];
    for (int i = 0; i < num_of_values - 1; ++i) // Add data values 0 - 9
    {
        data[i] = i;
    }
    data[10] = 2.5; // Add an extra data point to skew the data to not be completely flat
    data[11] = 9.5;

    regular_grid1d grid =
        make_grid_for_box(
            make_box(make_vector(1.), make_vector((double)num_of_values)), make_vector(1.));
    create_image(imagedvh, grid.n_points, data);

    auto image_hist = image_histogram(as_variant(share(imagedvh)), 0, 9, bin_size);
    auto image_hist_is = as_const_view(cast_variant<unsigned>(image_hist));

    CHECK_FALSE(image_hist_is.size[0] != num_of_bins);
    for (unsigned i = 0; i < image_hist_is.size[0]; ++i)
    {
        // Check that all bins are holding 1 each except for bins 2 and 9 have 2 each
        if (i == 2 || i == 9)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != 2);
        }
        else if (image_hist_is.pixels[i] != 1)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != 1);
        }
    }
}

TEST_CASE("CradleImaging combine_images test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  combine_images
    // Testing for 1d is non-required, but listed for completeness, as all functions rely
    // on the same templated implementation
    //  create_uniform_image_on_grid_1d
    //  create_uniform_image_on_grid_2d
    //  create_uniform_image_on_grid_3d

    // 2D A
    auto img2_1 =
        cast_variant<double>(
            create_uniform_image_on_grid<2>(
                make_grid_for_box(
                    make_box(make_vector(0.0, 0.0), make_vector(10.0, 20.0)),
                    make_vector(1.0, 1.0)), 1.0, "mm"));
    auto img2_2 =
        cast_variant<double>(
            create_uniform_image_on_grid<2>(
                make_grid_for_box(
                    make_box(make_vector(5.0, 10.0), make_vector(10.0, 20.0)),
                    make_vector(1.0, 1.0)), 2.0, "mm"));
    std::vector<image<2, double, shared> > imgs2;
    imgs2.push_back(img2_1);
    imgs2.push_back(img2_2);

    auto img2_3 = combine_images(imgs2);
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/Combine_Image2.vtk", img2_3);
    }

    auto expect_size2 = make_vector<unsigned>(15, 30);
    CHECK_FALSE(img2_3.size != expect_size2);
    CHECK_FALSE(img2_3.origin != img2_1.origin);
    CHECK_FALSE(img2_3.axes != img2_1.axes);

    auto image_const_view = as_const_view(img2_3);
    double val1 =
        (double)image_const_view.pixels[0] * img2_3.value_mapping.slope
        + img2_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 1.0);
    val1 =
        (double)image_const_view.pixels[13] * img2_3.value_mapping.slope
        + img2_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 1.0);
    val1 =
        (double)image_const_view.pixels[172] * img2_3.value_mapping.slope
        + img2_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 3.0);
    val1 =
        (double)image_const_view.pixels[445] * img2_3.value_mapping.slope
        + img2_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 2.0);

    // 2D B (switch order in combine list)
    std::vector<image<2, double, shared> > imgs2b;
    imgs2b.push_back(img2_2);
    imgs2b.push_back(img2_1);

    auto img2_3b = combine_images(imgs2b);
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/Combine_Image2b.vtk", img2_3b);
    }

    CHECK_FALSE(img2_3b.size != expect_size2);
    CHECK_FALSE(img2_3b.origin != img2_1.origin);
    CHECK_FALSE(img2_3b.axes != img2_1.axes);

    auto image_const_view_b = as_const_view(img2_3b);
    val1 =
        (double)image_const_view_b.pixels[0] * img2_3b.value_mapping.slope
        + img2_3b.value_mapping.intercept;
    CHECK_FALSE(val1 != 1.0);
    val1 =
        (double)image_const_view_b.pixels[13] * img2_3b.value_mapping.slope
        + img2_3b.value_mapping.intercept;
    CHECK_FALSE(val1 != 1.0);
    val1 =
        (double)image_const_view_b.pixels[172] * img2_3b.value_mapping.slope
        + img2_3b.value_mapping.intercept;
    CHECK_FALSE(val1 != 3.0);
    val1 =
        (double)image_const_view_b.pixels[445] * img2_3b.value_mapping.slope
        + img2_3b.value_mapping.intercept;
    CHECK_FALSE(val1 != 2.0);

    // 3D
    auto img3_1 =
        cast_variant<double>(
            create_uniform_image_on_grid<3>(
                make_grid_for_box(
                    make_box(make_vector(0., 0., 0.), make_vector(10., 15., 20.)),
                    make_vector(1., 1., 1.)), 1.0, "mm"));
    auto img3_2 =
        cast_variant<double>(
            create_uniform_image_on_grid<3>(
                make_grid_for_box(
                    make_box(make_vector(5., 0., 0.), make_vector(10., 15., 20.)),
                    make_vector(1., 1., 1.)), 2.0, "mm"));
    std::vector<image<3, double, shared> > imgs3;
    imgs3.push_back(img3_1);
    imgs3.push_back(img3_2);

    auto img3_3 = combine_images(imgs3);
    if (isDebugImaging)
    {
        write_vtk_file("Unit_Tests.dir/Combine_Image3.vtk", img3_3);
    }

    auto expect_size3 = make_vector<unsigned>(15, 15, 20);
    CHECK_FALSE(img3_3.size != expect_size3);
    CHECK_FALSE(img3_3.origin != img3_1.origin);
    CHECK_FALSE(img3_3.axes != img3_1.axes);

    auto image_const_view3 = as_const_view(img3_3);

    val1 =
        (double)image_const_view3.pixels[122] * img3_3.value_mapping.slope
        + img3_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 1.0);
    val1 =
        (double)image_const_view3.pixels[127] * img3_3.value_mapping.slope
        + img3_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 3.0);
    val1 =
        (double)image_const_view3.pixels[163] * img3_3.value_mapping.slope
        + img3_3.value_mapping.intercept;
    CHECK_FALSE(val1 != 2.0);
}


TEST_CASE("CradleImaging compute_grid_cells_in_polyset test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_grid_cells_in_polyset

    // make a grid
    auto grid =
        make_regular_grid(
            make_vector(1.0, 1.0), make_vector(1.0, 1.0), make_vector(7u, 7u));

    // polygon 1
    polygon2 poly = as_polygon(make_box(make_vector(3.5, 2.5), make_vector(3.0, 2.0)));
    polyset polyset = make_polyset(poly);

    grid_cell_inclusion_info list = compute_grid_cells_in_polyset(grid, polyset);

    // Check list size
    CHECK_FALSE(list.cells_inside.size() < 6);

    // Test the weight values of cells in polyset
    for (size_t i = 0; i < list.cells_inside.size(); ++i)
    {
        switch (list.cells_inside[i].index)
        {
            // All cases here have same expected result
            case 17:
            case 18:
            case 19:
            case 24:
            case 25:
            case 26:
                CHECK_FALSE(list.cells_inside[i].weight != 1.0);
                break;
            default:
                break;
        }
    }

    // polygon 2
    polygon2 poly2 = as_polygon(make_box(make_vector(3.0, 2.0), make_vector(2.0, 3.0)));
    polyset = make_polyset(poly2);

    grid_cell_inclusion_info list2 = compute_grid_cells_in_polyset(grid, polyset);

    // Check list size
    CHECK_FALSE(list2.cells_inside.size() < 12);

    for (size_t i = 0; i < list2.cells_inside.size(); ++i)
    {
        switch (list2.cells_inside[i].index)
        {
            // Cases must be either inside the edge of the polyset or completely inside
            // the polyset
            case 9:
            case 11:
            case 30:
            case 32:
                CHECK_FALSE(list2.cells_inside[i].weight != 0.25);
                break;
            case 10:
            case 16:
            case 18:
            case 23:
            case 25:
            case 31:
                CHECK_FALSE(list2.cells_inside[i].weight != 0.5);
                break;
            case 17:
            case 24:
                CHECK_FALSE(list2.cells_inside[i].weight != 1.0);
                break;
            default:
                break;
        }
    }
}

TEST_CASE("CradleImaging compute_grid_cells_in_circle test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_grid_cells_in_cicle

    // make a grid
    auto grid =
        make_regular_grid(
            make_vector(1.0, 1.0), make_vector(1.0, 1.0), make_vector(7u, 7u));

    // make a circle
    circle<double> circ = circle<double>(make_vector(4.0, 4.0), 2.0);

    grid_cell_inclusion_info list = compute_grid_cells_in_circle(grid, circ);

    // Check list size
    CHECK_FALSE(list.cells_inside.size() < 21);

    // Test the weight values of cells in circle
    for (size_t i = 0; i < list.cells_inside.size(); ++i)
    {
        switch (list.cells_inside[i].index)
        {
            // Cases must be either inside the edge of the circle or completely inside
            // the circle
            case 9:
            case 10:
            case 11:
            case 15:
            case 16:
                CHECK_FALSE((
                    list.cells_inside[i].weight == 0.0
                    || list.cells_inside[i].weight == 1.0));
                break;
            case 17:
                CHECK_FALSE(list.cells_inside[i].weight != 1.0);
                break;
            case 18:
            case 19:
            case 22:
                CHECK_FALSE((
                    list.cells_inside[i].weight == 0.0
                    || list.cells_inside[i].weight == 1.0));
                break;
            case 23:
            case 24:
            case 25:
                CHECK_FALSE(list.cells_inside[i].weight != 1.0);
                break;
            case 26:
            case 29:
            case 30:
                CHECK_FALSE((
                    list.cells_inside[i].weight == 0.0
                    || list.cells_inside[i].weight == 1.0));
                break;
            case 31:
                CHECK_FALSE(list.cells_inside[i].weight != 1.0);
                break;
            case 32:
                CHECK_FALSE((
                    list.cells_inside[i].weight == 0.0
                    || list.cells_inside[i].weight == 1.0));
                break;
            case 33:
            case 37:
            case 38:
            case 39:
                CHECK_FALSE((
                    list.cells_inside[i].weight == 0.0
                    || list.cells_inside[i].weight == 1.0));
                break;
            default:
                break;
        }
    }
}

TEST_CASE("CradleImaging structure imaging test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_grid_cells_in_structure
    //  compute_structure_inclusion_image

    // Make a 3d grid
    unsigned count = 7u;
    auto spacing = make_vector(1.0, 1.0, 1.0);
    auto grid =
        make_regular_grid(
            make_vector(1.0, 1.0, 1.0), spacing, make_vector(count, count, count));

    // Test box
    auto box1 = make_box(make_vector(3.5, 2.5, 0.5), make_vector(3., 2., 2.));
    auto sliced_b1 = make_sliced_box(box1, 2, 1.);

    grid_cell_inclusion_info list = compute_grid_cells_in_structure(grid, sliced_b1);

    // Check list size
    CHECK_FALSE(list.cells_inside.size() < 12);

    auto img = compute_structure_inclusion_image(grid, sliced_b1);
    auto image_const_view = as_const_view(img);

    // Test the weight values of cells in structure and compare to pixel weight
    for (size_t i = 0; i < list.cells_inside.size(); ++i)
    {
        switch (list.cells_inside[i].index)
        {
            // All cases here have same expected result
            case 17:
            case 18:
            case 19:
            case 24:
            case 25:
            case 26:
            case 66:
            case 67:
            case 68:
            case 73:
            case 74:
            case 75:
                CHECK_FALSE(list.cells_inside[i].weight != 1.0);
                CHECK_FALSE(
                    list.cells_inside[i].weight
                    != image_const_view.pixels[list.cells_inside[i].index]);
                break;
                // Default looks for weight values outside of box
            default:
                CHECK_FALSE(list.cells_inside[i].weight > 0.0001);
                break;
        }
    }

    // Test sphere
    auto sphere_center = make_vector(4., 3., 3.);
    structure_geometry sliced_s = make_sliced_sphere(sphere_center, 2., 128, 2, 1.);

    grid_cell_inclusion_info list2 = compute_grid_cells_in_structure(grid, sliced_s);

    // Check list size (dropped from 64 to allow for failures at ends due to small voxel
    // assumption
    CHECK(list2.cells_inside.size() >= 60);

    auto img2 = compute_structure_inclusion_image(grid, sliced_s);
    auto image_const_view2 = as_const_view(img2);

    // Test the weight values of cells in structure and compare to pixel weight
    int countsq = count * count;
    for (size_t i = 0; i < list2.cells_inside.size(); ++i)
    {
        // Find the distance between current cell and center of sphere
        auto z = boost::numeric_cast<int>(list2.cells_inside[i].index / countsq);
        auto y =
            boost::numeric_cast<int>(
                (list2.cells_inside[i].index - countsq * z) / (int)count);
        auto x =
            boost::numeric_cast<int>(
                list2.cells_inside[i].index - countsq * z - (int)count * y);
        double dx = sphere_center[0] - (x * spacing[0] + grid.p0[0]);
        double dy = sphere_center[1] - (y * spacing[1] + grid.p0[1]);
        double dz = sphere_center[2] - (z * spacing[2] + grid.p0[2]);
        double distance = sqrt(dx * dx + dy * dy);

        CHECK_FALSE((
            distance <= 1. && std::abs(dz) <= 1.0 && list2.cells_inside[i].weight != 1.));
        CHECK_FALSE((
            ((1. < distance) && (distance <= 2.) && std::abs(dz) < 2.0)
            && !((0. < list2.cells_inside[i].weight)
                && (list2.cells_inside[i].weight <= 1.))));
        CHECK_FALSE(((2.5 < distance) && (0.0001 < list2.cells_inside[i].weight)));
        CHECK_FALSE(
            list2.cells_inside[i].weight
            != image_const_view2.pixels[list2.cells_inside[i].index]);
    }
}

TEST_CASE("CradleImaging image integral test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_image_integral_over_line_segment
    //  compute_image_integral_over_line_segment_min_max
    //  compute_inverse_image_integral_over_ray

    // Make a uniform box
    auto box1 = make_box(make_vector(2., 2.), make_vector(4., 3.));
    auto img1 = create_uniform_image(box1, 1., "mm");

    // Make two line segments with equivalent integrals and compare
    auto line1 = make_line_segment(make_vector(2.5, 2.), make_vector(2.5, 5.));
    auto line2 = make_line_segment(make_vector(4.5, 1.), make_vector(4.5, 6.));

    auto integral1 = compute_image_integral_over_line_segment(img1, line1);
    auto integral2 = compute_image_integral_over_line_segment(img1, line2);

    CHECK_FALSE(integral1 != integral2);

    // Make line segments completely inside box and compare integral to length
    double dx = 3.;
    double dy = 2.;
    auto line3 = make_line_segment(make_vector(3., 2.), make_vector(3. + dx, 2. + dy));
    double length3 = sqrt(dx * dx + dy * dy);

    auto integral3 = compute_image_integral_over_line_segment(img1, line3);

    CHECK_FALSE(integral3 != length3);

    double dx2 = 0.;
    double dy2 = 1.;
    auto line3_2 =
        make_line_segment(make_vector(3.5, 3.5), make_vector(3.5 + dx2, 3.5 + dy2));
    double length3_2 = sqrt(dx2 * dx2 + dy2 * dy2);

    auto integral3_2 = compute_image_integral_over_line_segment(img1, line3_2);

    CHECK_FALSE(integral3_2 != length3_2);

    // Make a non-uniform box by combining two uniform boxes of different intensities
    auto img2_1 =
        cast_variant<double>(
            create_uniform_image_on_grid<2>(
                make_grid_for_box(
                    make_box(make_vector(7., 7.), make_vector(2., 3.)),
                    make_vector(1., 1.)), 1., "mm"));
    auto img2_2 =
        cast_variant<double>(
            create_uniform_image_on_grid<2>(
                make_grid_for_box(
                    make_box(make_vector(9., 7.), make_vector(2., 3.)),
                    make_vector(1., 1.)), .5, "mm"));
    std::vector<image<2, double, shared> > imgs2;
    imgs2.push_back(img2_1);
    imgs2.push_back(img2_2);

    auto img2 = combine_images(imgs2);

    // Make two line segments with different integrals and compare
    auto line4 = make_line_segment(make_vector(7.5, 6.), make_vector(7.5, 11.));
    auto line5 = make_line_segment(make_vector(10.5, 6.), make_vector(10.5, 11.));

    auto integral4 = compute_image_integral_over_line_segment(img2, line4);
    auto integral5 = compute_image_integral_over_line_segment(img2, line5);

    CHECK_FALSE(integral4 != 2.0 * integral5);

    // Make a line run through both halves of non-uniform box
    auto line6 = make_line_segment(make_vector(6., 8.5), make_vector(12., 8.5));
    auto integral6 = compute_image_integral_over_line_segment(img2, line6);

    CHECK_FALSE(integral6 != 3.0);

    // Make a min/max integral of Line 6 that will accept small intensities
    // and compare to equivalent integral
    auto integral6_2 =
        compute_image_integral_over_line_segment_min_max(img2, line6, 0., 0.55, 0.);
    auto line7 = make_line_segment(make_vector(9., 8.5), make_vector(12., 8.5));
    auto integral7 = compute_image_integral_over_line_segment(img2, line7);

    CHECK_FALSE(integral6_2 != integral7);

    // Make a min/max integral of Line 6 that will accept large intensities
    // and compare to equivalent integral
    auto integral6_3 =
        compute_image_integral_over_line_segment_min_max(img2, line6, 0.75, 1.25, 0.);
    auto line8 = make_line_segment(make_vector(7.0, 8.5), make_vector(9.0, 8.5));
    auto integral8 = compute_image_integral_over_line_segment(img2, line8);

    CHECK_FALSE(integral6_3 != integral8);

    // Make a ray run through both halves of non-uniform box
    // and compare inverse integral function result to equivalent length
    auto ray1 = ray<2, double>(make_vector(6., 8.5), make_vector(1., 0.));
    auto rayintegral1 = compute_image_integral_over_ray(img2, ray1);
    auto inv_rayintegral1 =
        compute_inverse_image_integral_over_ray(img2, ray1, rayintegral1);

    double length9 = 5.0; // distance from right side of img2 to start point of ray1

    CHECK_FALSE(inv_rayintegral1 != length9);

    // Make a ray inside uniform half of box
    // and compare inverse integral function result to equivalent length
    auto ray2 = ray<2, double>(make_vector(7.5, 7.5), make_vector(0., 1.));
    auto inv_rayintegral2 = compute_inverse_image_integral_over_ray(img2, ray2, 2.);

    double length10 = 2.0; // Image values are 1.0 in this region,
    // so line length should match integral length

    CHECK_FALSE(inv_rayintegral2 != length10);
}

TEST_CASE("CradleImaging set_data_for_mesh_double test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  set_data_for_mesh_double
    // (Indirectly)
    //  mesh_contains

    // Combine two 3d images with different image values
    auto spacing = make_vector(1., 1., 1.);
    auto grid1 =
        make_grid_for_box(
            make_box(make_vector(1., 1., 1.), make_vector(3., 7., 7.)), spacing);
    auto grid2 =
        make_grid_for_box(
            make_box(make_vector(4., 1., 1.), make_vector(4., 7., 7.)), spacing);
    auto img1_1 = cast_variant<double>(create_uniform_image_on_grid<3>(grid1, .5, "mm"));
    auto img1_2 = cast_variant<double>(create_uniform_image_on_grid<3>(grid2, 1., "mm"));
    std::vector<image<3, double, shared> > imgs1;
    imgs1.push_back(img1_1);
    imgs1.push_back(img1_2);

    auto img1 = combine_images(imgs1);

    // Mesh cube
    // Threshold value above 1 will set pixels inside mesh to value of 1
    auto mesh_c = make_cube(make_vector(2., 2., 2.), make_vector(5., 4., 4.));
    auto mesh_cube =
        optimized_triangle_mesh(mesh_c, make_bin_collection_from_mesh(mesh_c));

    std::vector<optimized_triangle_mesh> mesh_list1;
    mesh_list1.push_back(mesh_cube);
    auto data_img1 = set_data_for_mesh_double(img1, mesh_list1, 1.5, true);

    auto image_const_view1 = as_const_view(data_img1);
    int ii = 0;
    for (unsigned k = 0; k < image_const_view1.size[2]; k++)
    {
        for (unsigned j = 0; j < image_const_view1.size[1]; j++)
        {
            for (unsigned i = 0; i < image_const_view1.size[0]; i++)
            {
                switch (ii)
                {
                    // All cases here have same expected result
                    case 57:
                    case 58:
                    case 59:
                    case 64:
                    case 65:
                    case 66:
                    case 106:
                    case 107:
                    case 108:
                    case 113:
                    case 114:
                    case 115:
                        CHECK_FALSE(image_const_view1.pixels[ii] != 1);
                        break;
                    // Default looks for pixel values outside of mesh
                    default:
                        CHECK_FALSE(image_const_view1.pixels[ii] != 0);
                        break;
                }
                ii++;
            }
        }
    }

    // Mesh sphere
    auto sphere_center = make_vector(4.5, 4.5, 3.5);
    double radius = 2.;
    auto mesh_s = make_sphere(sphere_center, radius, 256, 256);
    auto mesh_sphere =
        optimized_triangle_mesh(mesh_s, make_bin_collection_from_mesh(mesh_s));

    std::vector<optimized_triangle_mesh> mesh_list2;
    mesh_list2.push_back(mesh_sphere);
    auto data_img2 = set_data_for_mesh_double(img1, mesh_list2, 1.5, true);

    auto image_const_view2 = as_const_view(data_img2);
    int jj = 0;
    for (unsigned k = 0; k < image_const_view2.size[2]; k++)
    {
        for (unsigned j = 0; j < image_const_view2.size[1]; j++)
        {
            for (unsigned i = 0; i < image_const_view2.size[0]; i++)
            {
                int z = jj / 49;
                int y = (jj - 49 * z) / 7;
                int x = jj - 49 * z - 7 * y;
                double dx = sphere_center[0] - (x * spacing[0] + grid1.p0[0]);
                double dy = sphere_center[1] - (y * spacing[1] + grid1.p0[1]);
                double dz = sphere_center[2] - (z * spacing[2] + grid1.p0[2]);
                double distance = sqrt(dx * dx + dy * dy + dz * dz);
                CHECK_FALSE((distance < radius && image_const_view2.pixels[jj] != 1));
                CHECK_FALSE((distance >= radius && image_const_view2.pixels[jj] != 0));
                jj++;
            }
        }
    }

    // Threshold between 0.5 and 1; will set pixels inside mesh
    // and in the 0.5 half of image to value of 1
    auto data_img2_2 = set_data_for_mesh_double(img1, mesh_list2, 0.75, true);

    auto image_const_view2_2 = as_const_view(data_img2_2);
    int kk = 0;
    for (unsigned k = 0; k < image_const_view2_2.size[2]; k++)
    {
        for (unsigned j = 0; j < image_const_view2_2.size[1]; j++)
        {
            for (unsigned i = 0; i < image_const_view2_2.size[0]; i++)
            {
                int z = kk / 49;
                int y = (kk - 49 * z) / 7;
                int x = kk - 49 * z - 7 * y;
                double dx = sphere_center[0] - (x * spacing[0] + grid1.p0[0]);
                double dy = sphere_center[1] - (y * spacing[1] + grid1.p0[1]);
                double dz = sphere_center[2] - (z * spacing[2] + grid1.p0[2]);
                double distance = sqrt(dx * dx + dy * dy + dz * dz);
                CHECK_FALSE((
                    distance < radius && x < 3 && image_const_view2_2.pixels[kk] != 1));
                CHECK_FALSE((
                    (distance >= radius || x >= 3) && image_const_view2_2.pixels[kk] != 0));
                kk++;
            }
        }
    }

    // set_data_inside set to false; pixels outside the mesh will be set to 1
    auto data_img2_3 = set_data_for_mesh_double(img1, mesh_list2, 1.5, false);

    auto image_const_view2_3 = as_const_view(data_img2_3);
    int mm = 0;
    for (unsigned k = 0; k < image_const_view2_3.size[2]; k++)
    {
        for (unsigned j = 0; j < image_const_view2_3.size[1]; j++)
        {
            for (unsigned i = 0; i < image_const_view2_3.size[0]; i++)
            {
                int z = mm / 49;
                int y = (mm - 49 * z) / 7;
                int x = mm - 49 * z - 7 * y;
                double dx = sphere_center[0] - (x * spacing[0] + grid1.p0[0]);
                double dy = sphere_center[1] - (y * spacing[1] + grid1.p0[1]);
                double dz = sphere_center[2] - (z * spacing[2] + grid1.p0[2]);
                double distance = sqrt(dx * dx + dy * dy + dz * dz);
                CHECK_FALSE((distance < radius && image_const_view2_3.pixels[mm] != 0));
                CHECK_FALSE((distance >= radius && image_const_view2_3.pixels[mm] != 1));
                mm++;
            }
        }
    }
}

TEST_CASE("CradleImaging slice_mesh test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  slice_mesh

    auto cube_origin = make_vector(3., 2., 1.);
    auto cube_extent = make_vector(7., 5., 3.);
    auto mesh_c = make_cube(cube_origin, cube_extent);

    // Slice box along the z axis
    plane<double> plane1 =
        plane<double>(make_vector(0., 0., 2.), make_vector(0., 0., 1.));
    auto up1 = make_vector(0., 1., 0.);
    matrix<4, 4, double> mat1 = make_matrix(1., 0., 0., 0.,
        0., 1., 0., 0.,
        0., 0., 1., 0.,
        0., 0., 0., 1.);

    auto slice1 = slice_mesh(plane1, up1, mesh_c, mat1);

    CHECK_FALSE((
        slice1.polygons.size() != 1 || slice1.holes.size() != 0
        || slice1.polygons[0].vertices.size() == 0));
    double expected_area1 =
        (cube_extent[0] - cube_origin[0]) * (cube_extent[1] - cube_origin[1]);
    CHECK(are_equal(get_area(slice1), expected_area1, tol * expected_area1));
    auto expected_bb1 = make_box(make_vector(cube_origin[0], cube_origin[1]),
        make_vector(cube_extent[0] - cube_origin[0], cube_extent[1] - cube_origin[1]));
    CHECK(are_equal(bounding_box(slice1).corner, expected_bb1.corner, tol));
    CHECK(are_equal(bounding_box(slice1).size, expected_bb1.size, tol));

    // Slice box along the x axis
    plane<double> plane2 = plane<double>(make_vector(4., 0., 0.), make_vector(1., 0., 0.));
    auto up2 = make_vector(0., 0., 1.);

    auto slice2 = slice_mesh(plane2, up2, mesh_c, mat1);

    CHECK_FALSE((
        slice2.polygons.size() != 1 || slice2.holes.size() != 0
        || slice2.polygons[0].vertices.size() == 0));
    double expected_area2 =
        (cube_extent[1] - cube_origin[1]) * (cube_extent[2] - cube_origin[2]);
    CHECK(are_equal(get_area(slice2), expected_area2, tol * expected_area2));
    auto expected_bb2 = make_box(make_vector(cube_origin[1], cube_origin[2]),
        make_vector(cube_extent[1] - cube_origin[1], cube_extent[2] - cube_origin[2]));
    CHECK(are_equal(bounding_box(slice2).corner, expected_bb2.corner, tol));
    CHECK(are_equal(bounding_box(slice2).size, expected_bb2.size, tol));

    // Rotate box 90 degrees about z and slice along z
    auto deg = angle<double, degrees>(90.);
    auto mat2 = rotation_about_z(deg);

    auto slice3 = slice_mesh(plane1, up1, mesh_c, mat2);

    CHECK_FALSE((
        slice3.polygons.size() != 1 || slice3.holes.size() != 0
        || slice3.polygons[0].vertices.size() == 0));
    CHECK(are_equal(get_area(slice3), expected_area1, tol * expected_area1));
    auto expected_bb3 = make_box(make_vector(-cube_extent[1], cube_origin[0]),
        make_vector(cube_extent[1] - cube_origin[1], cube_extent[0] - cube_origin[0]));
    CHECK(are_equal(bounding_box(slice3).corner, expected_bb3.corner, tol));
    CHECK(are_equal(bounding_box(slice3).size, expected_bb3.size, tol));

    // Slice through sphere center using oblique planes
    auto sphere_center = make_vector(4., 4., 4.);
    double radius = 3.;
    auto mesh_s = make_sphere(sphere_center, radius, 256, 256);
    auto expected_area3 = pi * radius * radius;

    auto normal1 = make_vector(1., 1., 0.);
    plane<double> plane3 = plane<double>(sphere_center, normal1 / length(normal1));
    auto up3 = make_vector(0., 0., 1.);
    auto slice4 = slice_mesh(plane3, up3, mesh_s, mat1);

    CHECK(are_equal(get_area(slice4), expected_area3, tol * expected_area3));

    auto normal2 = make_vector(2., 0., 3.);
    plane<double> plane4 = plane<double>(sphere_center, normal2 / length(normal2));
    auto up4 = make_vector(0., 1., 0.);
    auto slice5 = slice_mesh(plane4, up4, mesh_s, mat1);

    CHECK(are_equal(get_area(slice5), expected_area3, tol * expected_area3));

    auto normal3 = make_vector(0., 5., 2.);
    plane<double> plane5 = plane<double>(sphere_center, normal3 / length(normal3));
    auto up5 = make_vector(1., 0., 0.);
    auto slice6 = slice_mesh(plane5, up5, mesh_s, mat1);

    CHECK(are_equal(get_area(slice6), expected_area3, tol * expected_area3));
}

TEST_CASE("CradleImaging compute_polyset_inclusion_image test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_polyset_inclusion_image

    unsigned count = 7u;
    auto spacing = make_vector(1., 1.);
    auto grid =
        make_regular_grid(make_vector(1.5, 1.5), spacing, make_vector(count, count));

    // Test box
    polygon2 poly1 = as_polygon(make_box(make_vector(2., 2.), make_vector(3., 2.)));
    polyset polyset1 = make_polyset(poly1);

    auto img1 = compute_polyset_inclusion_image(grid, polyset1);
    auto img_const_view1 = as_const_view(img1);

    int ii = 0;
    for (unsigned j = 0; j < img_const_view1.size[1]; ++j)
    {
        for (unsigned i = 0; i < img_const_view1.size[0]; ++i)
        {
            switch (ii)
            {
                // Cases for pixels inside the box
                case 8:
                case 9:
                case 10:
                case 15:
                case 16:
                case 17:
                    CHECK_FALSE(img_const_view1.pixels[ii] != 1.);
                    break;
                // Default looks for pixel values outside of box
                default:
                    CHECK_FALSE(img_const_view1.pixels[ii] != 0.);
                    break;
            }
            ii++;
        }
    }

    // Test circle
    auto center = make_vector(4., 4.);
    double radius = 2.;
    auto cir = circle<double>(center, radius);
    polygon2 poly2 = as_polygon(cir, 128);
    polyset polyset2 = make_polyset(poly2);

    auto img2 = compute_polyset_inclusion_image(grid, polyset2);
    auto img_const_view2 = as_const_view(img2);

    int jj = 0;
    for (unsigned j = 0; j < img_const_view2.size[1]; ++j)
    {
        for (unsigned i = 0; i < img_const_view2.size[0]; ++i)
        {
            switch (jj)
            {
                // Cases for pixels partially inside the circle
                case 8:
                case 9:
                case 10:
                case 11:
                case 15:
                case 18:
                case 22:
                case 25:
                case 29:
                case 30:
                case 31:
                case 32:
                    CHECK_FALSE((
                        img_const_view2.pixels[jj] == 0.
                        || img_const_view2.pixels[jj] == 1.));
                    break;
                // Cases for pixels completely inside the circle
                case 16:
                case 17:
                case 23:
                case 24:
                    CHECK_FALSE(img_const_view2.pixels[jj] != 1.);
                    break;
                // Default looks for pixel values outside of circle
                default:
                    CHECK_FALSE(img_const_view2.pixels[jj] != 0.);
                    break;
            }
            jj++;
        }
    }
}

TEST_CASE("CradleImaging partial_image_histogram test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  partial_image_histogram

    // Create a 1D image
    int num_of_values = 10;
    double bin_size = 1.;

    image<1, double, unique> imagedvh;
    double *data = new double[num_of_values];
    for (int i = 0; i < num_of_values; ++i)
    {
        data[i] = i;
    }

    regular_grid1d grid =
        make_grid_for_box(
            make_box(make_vector(1.), make_vector((double)num_of_values)),
            make_vector(1.));
    create_image(imagedvh, grid.n_points, data);

    // Find the weighted partial histogram using a list of weighted grid indices
    float w1 = 2.;
    float w2 = 3.;
    float w3 = 4.;
    std::vector<weighted_grid_index> indices;
    indices.push_back(weighted_grid_index((size_t)1, w1));
    indices.push_back(weighted_grid_index((size_t)2, w2));
    indices.push_back(weighted_grid_index((size_t)2.5, w3));
    indices.push_back(weighted_grid_index((size_t)7.2, w1));
    indices.push_back(weighted_grid_index((size_t)7.4, w1));
    indices.push_back(weighted_grid_index((size_t)7.9, w3));

    auto var_img = as_variant(share(imagedvh));
    auto image_hist = partial_image_histogram(var_img, indices, 0., 9., bin_size);
    auto image_hist_is = as_const_view(cast_variant<float>(image_hist));

    CHECK_FALSE(boost::numeric_cast<int>(image_hist_is.size[0]) != num_of_values);
    for (unsigned i = 0; i < image_hist_is.size[0]; ++i)
    {
        // Check that all bins are holding 0 each except for bins 1, 2, and 7
        if (i == 1)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != w1);
        }
        else if (i == 2)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != w2 + w3);
        }
        else if (i == 7)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != 2. * w1 + w3);
        }
        else if (image_hist_is.pixels[i] != 0.)
        {
            CHECK_FALSE(image_hist_is.pixels[i] != 0.);
        }
    }
}

TEST_CASE("CradleImaging compute isolines/isobands test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  compute_isolines
    //  compute_isobands
    // (Indirectly)
    //  connect_line_segments
    //  as_polyset(vector<line_strip> const& strips, double tolerance)

    // Create an image which will result in a single horizontal isoline
    image<2, double, cradle::unique> img1;
    auto spacing = make_vector(1., 1.);
    auto size = make_vector(7., 7.);
    auto img1_grid = make_grid_for_box(make_box(make_vector(1., 1.), size), spacing);
    create_image_on_grid(img1, img1_grid);

    vector2i index;
    unsigned ii = 0;
    for (unsigned i = 0; i < img1.size[0]; ++i)
    {
        index[0] = i;
        for (unsigned j = 0; j < img1.size[1]; ++j)
        {
            index[1] = j;
            if (index[0] < 3)
            {
                img1.pixels.ptr[ii] = 1.;
            }
            else
            {
                img1.pixels.ptr[ii] = 0.5;
            }
            ++ii;
        }
    }

    image<2, double, cradle::shared> shared_img1 = share(img1);
    auto var_img1 = as_variant(shared_img1);

    // Confirm image was properly constructed
    auto img_const_view1 = as_const_view(shared_img1);
    int ii2 = 0;
    for (unsigned j = 0; j < img_const_view1.size[1]; ++j)
    {
        for (unsigned i = 0; i < img_const_view1.size[0]; ++i)
        {
            int index_y = ii2 / 7;
            double y = index_y * spacing[1] + img1_grid.p0[1];

            CHECK_FALSE((y < 4. && img_const_view1.pixels[ii2] != 1.));

            CHECK_FALSE((y > 4. && img_const_view1.pixels[ii2] != 0.5));
            ii2++;
        }
    }

    // Compute isolines
    auto isolines1 = compute_isolines(var_img1, 0.75);
    auto connected_isolines1 = connect_line_segments(isolines1, tol);

    // Check size of isoline set
    CHECK_FALSE(connected_isolines1.size() != 1);
    // Check position of isolines
    for (size_t i = 0; i < connected_isolines1[0].vertices.size(); i++)
    {
        CHECK_FALSE(connected_isolines1[0].vertices[i][1] != 4.);
    }

    // Check using a level that will result in zero isolines
    auto isolines1_2 = compute_isolines(var_img1, 1.5);
    auto connected_isolines1_2 = connect_line_segments(isolines1_2, tol);

    CHECK_FALSE(connected_isolines1_2.size() != 0);

    // Check the isobands for the region of value 1 pixels
    auto isobands1 = compute_isobands(var_img1, 0.75, 1.25);

    double expected_area1 = size[0] * 3.;
    double area1 = 0.;
    for (size_t i = 0; i < isobands1.size(); i++)
    {
        area1 += get_area(isobands1[i]);
    }

    CHECK(are_equal(area1, expected_area1, tol * expected_area1));

    for (size_t i = 0; i < isobands1.size(); i++)
    {
        for (size_t j = 0; j < isobands1[i].size(); j++)
        {
            CHECK_FALSE((
                isobands1[i][j][0] < 1. || isobands1[i][j][0] > 8.
                || isobands1[i][j][1] < 1. || isobands1[i][j][1] > 4.));
        }
    }

    // Make a ring of pixels in image with values of 1
    image<2, double, cradle::unique> img2;
    auto img2_grid = make_grid_for_box(make_box(make_vector(10., 1.), size), spacing);
    create_image_on_grid(img2, img2_grid);

    vector2i index2;
    unsigned jj = 0;
    for (unsigned i = 0; i < img2.size[0]; ++i)
    {
        index2[0] = i;
        for (unsigned j = 0; j < img2.size[1]; ++j)
        {
            index2[1] = j;
            if ((index2[1] == 1 || index2[1] == 5) && index2[0] >= 1 && index2[0] <= 5)
            {
                img2.pixels.ptr[jj] = 1.;
            }
            else if (
                (index2[0] == 1 || index2[0] == 5) && index2[1] >= 2 && index2[1] <= 4)
            {
                img2.pixels.ptr[jj] = 1.;
            }
            else
            {
                img2.pixels.ptr[jj] = 0.5;
            }
            ++jj;
        }
    }

    image<2, double, cradle::shared> shared_img2 = share(img2);
    auto var_img2 = as_variant(shared_img2);

    auto isolines2 = compute_isolines(var_img2, 0.75);
    auto connected_isolines2 = connect_line_segments(isolines2, tol);

    // Create a polyset out of resulting isolines and check for the pixels
    // inside the polyset
    auto opt_polyset2 = as_polyset(connected_isolines2, tol);
    auto polyset2 = opt_polyset2.get();

    // Confirm image was properly constructed and isolines properly form the polyset shape
    auto img_const_view2 = as_const_view(shared_img2);
    int jj2 = 0;
    for (size_t j = 0; j < img_const_view2.size[1]; ++j)
    {
        for (size_t i = 0; i < img_const_view2.size[0]; ++i)
        {
            int index_y = jj2 / 7;
            int index_x = jj2 - (7 * index_y);
            double x = index_x * spacing[0] + img2_grid.p0[0];
            double y = index_y * spacing[1] + img2_grid.p0[1];
            auto point = make_vector(x, y);

            CHECK_FALSE((
                is_inside(polyset2, point) && img_const_view2.pixels[jj2] != 1.));

            CHECK_FALSE((
                !is_inside(polyset2, point) && img_const_view2.pixels[jj2] != 0.5));
            jj2++;
        }
    }

    // Check the area of the polyset made from the isolines
    double expected_area2 =
        (size[0] - 2.) * (size[0] - 2.) - (size[0] - 4.) * (size[0] - 4.);
    double polyset2_area = get_area(polyset2);

    CHECK(are_equal(polyset2_area, expected_area2, tol * expected_area2));

    // Check the isobands for the region of value 1 pixels
    auto isobands2 = compute_isobands(var_img2, 0.75, 1.25);

    double area2 = 0.;
    for (size_t i = 0; i < isobands2.size(); i++)
    {
        area2 += get_area(isobands2[i]);
    }

    CHECK(are_equal(area2, expected_area2, tol * expected_area2));

    for (size_t i = 0; i < isobands2.size(); i++)
    {
        for (size_t j = 0; j < isobands2[i].size(); j++)
        {
            // Check if outside outer band
            CHECK_FALSE((
                isobands2[i][j][0] < 11. || isobands2[i][j][0] > 16.
                || isobands2[i][j][1] < 2. || isobands2[i][j][1] > 7.));
            // Check if inside inner band (split in two because of corner chamfers)
            CHECK_FALSE((
                (isobands2[i][j][0] > 12. && isobands2[i][j][0] < 15.)
                && (isobands2[i][j][1] > 3.5 && isobands2[i][j][1] < 5.5)));
            CHECK_FALSE((
                (isobands2[i][j][0] > 12.5 && isobands2[i][j][0] < 14.5)
                && (isobands2[i][j][1] > 3. && isobands2[i][j][1] < 6.)));
        }
    }
}

TEST_CASE("CradleImaging image_sample test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  image_sample
    //  interpolated_image_sample
    //  image_sample_over_box

    // Make an image with two regions of different values
    image<2, double, cradle::unique> img1;
    auto spacing = make_vector(1., 1.);
    auto size = make_vector(7., 7.);
    auto img1_grid = make_grid_for_box(make_box(make_vector(1., 1.), size), spacing);
    create_image_on_grid(img1, img1_grid);

    vector2i index;
    unsigned ii = 0;
    double value1 = 1.;
    double value2 = 0.5;
    for (unsigned i = 0; i < img1.size[0]; ++i)
    {
        index[0] = i;
        for (unsigned j = 0; j < img1.size[1]; ++j)
        {
            index[1] = j;
            if (index[0] < 3)
            {
                img1.pixels.ptr[ii] = value1;
            }
            else
            {
                img1.pixels.ptr[ii] = value2;
            }
            ++ii;
        }
    }

    image<2, double, cradle::shared> shared_img1 = share(img1);

    // *** Image Sample Tests ***

    // Find the image sample of a point inside the "value1" region of image
    auto point1 = make_vector(3., 3.);
    auto opt_sample1 = image_sample(shared_img1, point1);
    auto sample1 = opt_sample1.get();

    CHECK_FALSE(sample1 != value1);

    // Use a point that lies between two pixels of different values
    // but closer to value1 region
    auto point2 = make_vector(6., 3.5);
    auto opt_sample2 = image_sample(shared_img1, point2);
    auto sample2 = opt_sample2.get();

    CHECK_FALSE(sample2 != value1);

    // Use a point that lies outside the image
    auto point3 = make_vector(10., 4.);
    auto opt_sample3 = image_sample(shared_img1, point3);

    CHECK(!opt_sample3);

    // *** Interpolated Image Sample Tests ***

    // Find the interpolated image sample of a point equidistant
    // from 2 pixels of different values
    auto point4 = make_vector(6., 4.);
    auto opt_sample4 = interpolated_image_sample(shared_img1, point4);
    auto sample4 = opt_sample4.get();

    CHECK(are_equal(sample4, (value1 + value2) / 2., tol));

    // Find the interpolated image sample of a point between pixels of different values
    auto point5 = make_vector(6., 4.25);
    auto opt_sample5 = interpolated_image_sample(shared_img1, point5);
    auto sample5 = opt_sample5.get();

    CHECK(are_equal(sample5, value1 + 0.75 * (value2 - value1), tol));

    // Use another point that lies outside the image
    auto point6 = make_vector(0., 4.);
    auto opt_sample6 = interpolated_image_sample(shared_img1, point6);

    CHECK(!opt_sample6);

    // *** Image Sample Over Box Tests ***

    // Find a uniform image sample over a box
    auto box1 = make_box(make_vector(2., 5.), make_vector(2., 3.));
    auto opt_sample7 = image_sample_over_box(shared_img1, box1);
    auto sample7 = opt_sample7.get();

    CHECK(are_equal(sample7, value2, tol));

    // Find an image sample over a box covering pixels of different values
    // (2 pixels @ value1, 4 pixels @ value2)
    auto box2 = make_box(make_vector(3., 3.), make_vector(2., 3.));
    auto opt_sample8 = image_sample_over_box(shared_img1, box2);
    auto sample8 = opt_sample8.get();

    CHECK(are_equal(sample8, (2. * value1 + 4. * value2) / 6., tol));

    // Find an image sample over a box that is partially inside the image
    auto box3 = make_box(make_vector(7., 1.), make_vector(3., 2.));
    auto opt_sample9 = image_sample_over_box(shared_img1, box3);
    auto sample9 = opt_sample9.get();

    CHECK(are_equal(sample9, value1, tol));

    // Find an image sample over a box that is complete outside the image
    auto box4 = make_box(make_vector(-1., 3.), make_vector(1., 1.));
    auto opt_sample10 = image_sample_over_box(shared_img1, box4);

    CHECK_FALSE(opt_sample10);
}

TEST_CASE("CradleImaging image_statistics test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  image_statistics
    //  partial_image_statistics
    //  weighted_partial_image_statistics

    // Make a 2D image with two regions of different values and take the statistics
    image<2, double, cradle::unique> img1;
    auto spacing = make_vector(1., 1.);
    auto size = make_vector(7., 7.);
    auto img1_grid = make_grid_for_box(make_box(make_vector(1., 1.), size), spacing);
    create_image_on_grid(img1, img1_grid);

    vector2i index;
    unsigned ii = 0;
    double value1 = 1.;
    double value2 = 0.5;
    for (unsigned j = 0; j < img1.size[1]; ++j)
    {
        index[1] = j;
        for (unsigned i = 0; i < img1.size[0]; ++i)
        {
            index[0] = i;
            if (index[1] < 3)
            {
                img1.pixels.ptr[ii] = value1;
            }
            else
            {
                img1.pixels.ptr[ii] = value2;
            }
            ++ii;
        }
    }

    image<2, double, cradle::shared> shared_img1 = share(img1);
    auto stat1 = image_statistics(shared_img1);

    CHECK_FALSE(stat1.max.get() != value1);
    CHECK_FALSE(stat1.min.get() != value2);
    double expected_mean1 = (21. * value1 + 28. * value2) / 49.;
    CHECK(are_equal(stat1.mean.get(), expected_mean1, tol));
    CHECK_FALSE(stat1.n_samples != size[0] * size[1]);

    // Take the partial image statistics using a list of specified pixels
    std::vector<size_t> indices1;
    indices1.push_back(0); // value1
    indices1.push_back(15); // value1
    indices1.push_back(21); // value2
    auto partial_stat1 = partial_image_statistics(shared_img1, indices1);

    CHECK_FALSE(partial_stat1.max.get() != value1);
    CHECK_FALSE(partial_stat1.min.get() != value2);
    double partial_expected_mean1 = (2. * value1 + value2) / 3.;
    CHECK(are_equal(partial_stat1.mean.get(), partial_expected_mean1, tol));
    CHECK_FALSE(partial_stat1.n_samples != indices1.size());

    // Take the weighted partial image statistics using a list of pixels
    // with specified weights
    std::vector<weighted_grid_index> w_indices1;
    w_indices1.push_back(weighted_grid_index(1, 0.5f)); // value1
    w_indices1.push_back(weighted_grid_index(3, 1.f)); // value1
    w_indices1.push_back(weighted_grid_index(12, 0.2f)); // value1
    w_indices1.push_back(weighted_grid_index(21, 1.f)); // value2
    w_indices1.push_back(weighted_grid_index(33, 0.75f)); // value2
    w_indices1.push_back(weighted_grid_index(42, 1.3f)); // value2
    auto w_partial_stat1 = weighted_partial_image_statistics(shared_img1, w_indices1);

    CHECK_FALSE(w_partial_stat1.max.get() != value1);
    CHECK_FALSE(w_partial_stat1.min.get() != value2);

    double weight_sum1 = 0.;
    for (size_t i = 0; i < w_indices1.size(); i++)
    {
        weight_sum1 += w_indices1[i].weight;
    }
    double w_partial_expected_mean1 = 0.;
    for (size_t i = 0; i < w_indices1.size(); i++)
    {
        if (w_indices1[i].index < 21)
        {
            w_partial_expected_mean1 += (value1 * w_indices1[i].weight / weight_sum1);
        }
        else
        {
            w_partial_expected_mean1 += (value2 * w_indices1[i].weight / weight_sum1);
        }
    }

    CHECK(are_equal(w_partial_stat1.mean.get(), w_partial_expected_mean1, tol));
    CHECK(are_equal(w_partial_stat1.n_samples, weight_sum1, tol));

    // Make a 3D image with three different pixel values and take the statistics
    image<3, double, cradle::unique> img2;
    auto spacing2 = make_vector(1., 1., 1.);
    auto size2 = make_vector(3., 4., 3.);
    auto img2_grid =
        make_grid_for_box(make_box(make_vector(10., 1., 1.), size2), spacing2);
    create_image_on_grid(img2, img2_grid);

    vector2i index2;
    unsigned jj = 0;
    double val1 = 0.2;
    double val2 = 0.3;
    double val3 = 0.5;
    for (unsigned k = 0; k < img2.size[2]; ++k)
    {
        index2[2] = k;
        for (unsigned j = 0; j < img2.size[1]; ++j)
        {
            index2[1] = j;
            for (unsigned i = 0; i < img2.size[0]; ++i)
            {
                index2[0] = i;
                if (index2[2] == 0)
                {
                    img2.pixels.ptr[jj] = val1;
                }
                else if (index2[2] == 1)
                {
                    img2.pixels.ptr[jj] = val2;
                }
                else
                {
                    img2.pixels.ptr[jj] = val3;
                }
                ++jj;
            }
        }
    }

    image<3, double, cradle::shared> shared_img2 = share(img2);
    auto stat2 = image_statistics(shared_img2);

    CHECK_FALSE(stat2.max.get() != val3);
    CHECK_FALSE(stat2.min.get() != val1);
    double expected_mean2 = (val1 + val2 + val3) / 3.;
    CHECK(are_equal(stat2.mean.get(), expected_mean2, tol));
    CHECK_FALSE(stat2.n_samples != size2[0] * size2[1] * size2[2]);

    // Take the partial image statistics using a list of specified pixels
    std::vector<size_t> indices2;
    indices2.push_back(16); // value2
    indices2.push_back(18); // value2
    indices2.push_back(20); // value2
    indices2.push_back(31); // value3
    auto partial_stat2 = partial_image_statistics(shared_img2, indices2);

    CHECK_FALSE(partial_stat2.max.get() != val3);
    CHECK_FALSE(partial_stat2.min.get() != val2);
    double partial_expected_mean2 = (3. * val2 + val3) / 4.;
    CHECK(are_equal(partial_stat2.mean.get(), partial_expected_mean2, tol));
    CHECK_FALSE(partial_stat2.n_samples != indices2.size());

    // Take the weighted partial image statistics using a list of pixels
    // with specified weights
    std::vector<weighted_grid_index> w_indices2;
    w_indices2.push_back(weighted_grid_index(1, 0.6f)); // value1
    w_indices2.push_back(weighted_grid_index(9, 2.f)); // value1
    w_indices2.push_back(weighted_grid_index(12, 0.2f)); // value2
    w_indices2.push_back(weighted_grid_index(19, 5.1f)); // value2
    w_indices2.push_back(weighted_grid_index(28, 0.75f)); // value3
    w_indices2.push_back(weighted_grid_index(32, 2.7f)); // value3
    auto w_partial_stat2 = weighted_partial_image_statistics(shared_img2, w_indices2);

    CHECK_FALSE(w_partial_stat2.max.get() != val3);
    CHECK_FALSE(w_partial_stat2.min.get() != val1);

    double weight_sum2 = 0.;
    for (size_t i = 0; i < w_indices2.size(); i++)
    {
        weight_sum2 += w_indices2[i].weight;
    }
    double w_partial_expected_mean2 = 0.;
    for (size_t i = 0; i < w_indices2.size(); i++)
    {
        if (w_indices1[i].index < 12)
        {
            w_partial_expected_mean2 += (val1 * w_indices2[i].weight / weight_sum2);
        }
        else if (w_indices1[i].index < 24)
        {
            w_partial_expected_mean2 += (val2 * w_indices2[i].weight / weight_sum2);
        }
        else
        {
            w_partial_expected_mean2 += (val3 * w_indices2[i].weight / weight_sum2);
        }
    }

    CHECK(are_equal(w_partial_stat2.mean.get(), w_partial_expected_mean2, tol));
    CHECK(are_equal(w_partial_stat2.n_samples, weight_sum2, tol));
}

TEST_CASE("CradleImaging merge_statistics test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  merge_statistics
    //  image_list_statistics

    // Make a 2D image with two regions of different values and take the statistics
    image<2, double, cradle::unique> img1;
    auto spacing = make_vector(1., 1.);
    auto size1 = make_vector(7., 7.);
    auto img1_grid = make_grid_for_box(make_box(make_vector(1., 1.), size1), spacing);
    create_image_on_grid(img1, img1_grid);

    vector2i index;
    unsigned ii = 0;
    double value1 = 1.;
    double value2 = 0.5;
    for (unsigned j = 0; j < img1.size[1]; ++j)
    {
        index[1] = j;
        for (unsigned i = 0; i < img1.size[0]; ++i)
        {
            index[0] = i;
            if (index[1] < 3)
            {
                img1.pixels.ptr[ii] = value1;
            }
            else
            {
                img1.pixels.ptr[ii] = value2;
            }
            ++ii;
        }
    }

    image<2, double, cradle::shared> shared_img1 = share(img1);
    auto stat1 = image_statistics(shared_img1);

    // Make a second 2D image and take the statistics
    image<2, double, cradle::unique> img2;
    auto size2 = make_vector(8., 3.);
    auto img2_grid = make_grid_for_box(make_box(make_vector(12., 3.), size2), spacing);
    create_image_on_grid(img2, img2_grid);

    vector2i index2;
    unsigned jj = 0;
    double value3 = 0.2;
    double value4 = 0.6;
    double value5 = 0.8;
    for (unsigned j = 0; j < img2.size[1]; ++j)
    {
        index2[1] = j;
        for (unsigned i = 0; i < img2.size[0]; ++i)
        {
            index2[0] = i;
            if (index2[0] < 3)
            {
                img2.pixels.ptr[jj] = value3;
            }
            else if (index2[0] < 6)
            {
                img2.pixels.ptr[jj] = value4;
            }
            else
            {
                img2.pixels.ptr[jj] = value5;
            }
            ++jj;
        }
    }

    image<2, double, cradle::shared> shared_img2 = share(img2);
    auto stat2 = image_statistics(shared_img2);

    // Merge statistics and test
    std::vector<statistics<double> > stats;
    stats.push_back(stat1);
    stats.push_back(stat2);
    auto merge = merge_statistics(stats);

    CHECK_FALSE(merge.max.get() != value1);
    CHECK_FALSE(merge.min.get() != value3);
    double expected_mean1 =
        (21. * value1 + 28. * value2 + 9. * value3 + 9. * value4 + 6. * value5)
        / (49. + 24.);
    CHECK(are_equal(merge.mean.get(), expected_mean1, tol));
    double pixel_sum = size1[0] * size1[1] + size2[0] * size2[1];
    CHECK_FALSE(merge.n_samples != pixel_sum);

    // Get the statistics of a list of the images
    std::vector<image<2, double, cradle::shared> > images;
    images.push_back(shared_img1);
    images.push_back(shared_img2);
    auto list_stat = image_list_statistics(images);

    CHECK_FALSE(list_stat.max.get() != value1);
    CHECK_FALSE(list_stat.min.get() != value3);
    CHECK(are_equal(list_stat.mean.get(), expected_mean1, tol));
    CHECK_FALSE(list_stat.n_samples != pixel_sum);
}

TEST_CASE("CradleImaging sum_image_list test", "[cradle][imaging]")
{
    // This test covers:
    // (Directly)
    //  sum_image_list

    // Make two 2D images of equal size and different value distributions
    image<2, double, cradle::unique> img1;
    auto spacing = make_vector(1., 1.);
    auto size = make_vector(7., 7.);
    auto img1_grid = make_grid_for_box(make_box(make_vector(1., 1.), size), spacing);
    create_image_on_grid(img1, img1_grid);

    vector2i index;
    unsigned ii = 0;
    double value1 = 1.;
    double value2 = 0.5;
    for (unsigned j = 0; j < img1.size[1]; ++j)
    {
        index[1] = j;
        for (unsigned i = 0; i < img1.size[0]; ++i)
        {
            index[0] = i;
            if (index[1] < 3)
            {
                img1.pixels.ptr[ii] = value1;
            }
            else
            {
                img1.pixels.ptr[ii] = value2;
            }
            ++ii;
        }
    }
    image<2, double, cradle::shared> shared_img1 = share(img1);
    auto var_img1 = as_variant(shared_img1);

    image<2, double, cradle::unique> img2;
    auto img2_grid = make_grid_for_box(make_box(make_vector(1., 1.), size), spacing);
    create_image_on_grid(img2, img2_grid);

    vector2i index2;
    unsigned jj = 0;
    double value3 = 0.2;
    double value4 = 0.6;
    double value5 = 0.8;
    for (unsigned j = 0; j < img2.size[1]; ++j)
    {
        index2[1] = j;
        for (unsigned i = 0; i < img2.size[0]; ++i)
        {
            index2[0] = i;
            if (index2[0] < 2)
            {
                img2.pixels.ptr[jj] = value3;
            }
            else if (index2[0] < 5)
            {
                img2.pixels.ptr[jj] = value4;
            }
            else
            {
                img2.pixels.ptr[jj] = value5;
            }
            ++jj;
        }
    }
    image<2, double, cradle::shared> shared_img2 = share(img2);
    auto var_img2 = as_variant(shared_img2);

    // Sum the images in a list and check the image statistics
    std::vector<image<2, cradle::variant, cradle::shared> > images;
    images.push_back(var_img1);
    images.push_back(var_img2);
    auto sum = sum_image_list(images);
    auto sum_stat = image_statistics(sum);

    CHECK_FALSE(sum_stat.max.get() != value1 + value5);
    CHECK_FALSE(sum_stat.min.get() != value2 + value3);
    double expected_mean =
        (6. * (value1 + value3) + 9. * (value1 + value4) + 6. * (value1 + value5)
        + 8. * (value2 + value3) + 12. * (value2 + value4) + 8. * (value2 + value5)) / 49.;
    CHECK(are_equal(sum_stat.mean.get(), expected_mean, tol));
    CHECK_FALSE(sum_stat.n_samples != size[0] * size[1]);
}

}