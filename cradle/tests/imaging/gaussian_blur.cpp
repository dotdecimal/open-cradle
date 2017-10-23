#include <cradle/imaging/gaussian_blur.hpp>
#include <cradle/imaging/utilities.hpp>
#include <cradle/io/standard_image_io.hpp>
#include <cradle/imaging/discretize.hpp>
#include <cradle/math/gaussian.hpp>

#define BOOST_TEST_MODULE gaussian_blur
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(single_pixel_blur_test)
{
    auto gaussian = create_gaussian_image(
        make_vector<double>(8, 8), make_vector<double>(1, 1));

    image<2,double,unique> src;
    create_image(src, make_vector<unsigned>(101, 101));
    fill_pixels(src, 0.);
    get_pixel_ref(src, make_vector<unsigned>(50, 50)) = 12;

    image<2,double,unique> blurred;
    create_image(blurred, make_vector<unsigned>(101, 101));

    double blurred_total = 0;
    for (unsigned i = 0; i != 101; ++i)
    {
        for (unsigned j = 0; j != 101; ++j)
        {
            double blurred_value =
                compute_gaussian_blurred_pixel(
                    as_const_view(src), as_const_view(gaussian),
                    make_vector<int>(i, j));
            double dx = std::fabs(50.5 - (i + 0.5));
            double dy = std::fabs(50.5 - (j + 0.5));
            double lower_bound =
                evaluate_gaussian(
                    make_vector<double>(dx + 0.5, dy + 0.5),
                    make_vector<double>(8, 8)) * 12.;
            double upper_bound =
                evaluate_gaussian(
                    make_vector<double>(dx - 0.5, dy - 0.5),
                    make_vector<double>(8, 8)) * 12.;
            BOOST_CHECK(
                blurred_value >= lower_bound - 0.001 &&
                blurred_value <= upper_bound + 0.001);
            get_pixel_ref(blurred, make_vector(i, j)) = blurred_value;
            blurred_total += blurred_value;
        }
    }
    CRADLE_CHECK_WITHIN_TOLERANCE(blurred_total, 12., 0.0001);

    image<2,cradle::uint8_t,shared> discretized;
    discretize(discretized, blurred, 255);

    boost::filesystem::path const ref_file =
        test_data_directory() / "imaging/blurred_pixel.png";

    write_image_file("blur.png", discretized);

    image<2,cradle::uint8_t,shared> ref_img;
    read_image_file(ref_img, ref_file);

    CRADLE_CHECK_IMAGE(discretized, ref_img.pixels.view,
        ref_img.pixels.view + product(ref_img.size));
}

BOOST_AUTO_TEST_CASE(grid_blur_test)
{
    image<2,double,shared> gaussian =
        create_gaussian_image(
            make_vector<double>(3.5, 3.5),
            make_vector<double>(1, 1));

    image<2,double,unique> src;
    create_image(src, make_vector<unsigned>(101, 101));
    fill_pixels(src, 0.);
    for (int i = 0; i != 5; ++i)
    {
        for (int j = 0; j != 5; ++j)
        {
            get_pixel_ref(src,
                make_vector<unsigned>(34 + i * 8, 34 + j * 8)) = 12;
        }
    }

    image<2,double,unique> blurred;
    create_image(blurred, make_vector<unsigned>(101, 101));

    double blurred_total = 0;
    for (unsigned i = 0; i != 101; ++i)
    {
        for (unsigned j = 0; j != 101; ++j)
        {
            double blurred_value =
                compute_gaussian_blurred_pixel(
                    as_const_view(src), as_const_view(gaussian),
                    make_vector<int>(i, j));
            get_pixel_ref(blurred, make_vector(i, j)) = blurred_value;
            blurred_total += blurred_value;
        }
    }
    CRADLE_CHECK_WITHIN_TOLERANCE(blurred_total, 300., 0.0001);

    image<2,cradle::uint8_t,shared> discretized;
    discretize(discretized, blurred, 255);

    boost::filesystem::path const ref_file =
        test_data_directory() / "imaging/blurred_grid.png";

    //write_image_file(ref_file, discretized);

    image<2,cradle::uint8_t,shared> ref_img;
    read_image_file(ref_img, ref_file);

    CRADLE_CHECK_IMAGE(discretized, ref_img.pixels.view,
        ref_img.pixels.view + product(ref_img.size));
}

BOOST_AUTO_TEST_CASE(single_pixel_blur_test_with_spacing)
{
    image<2,double,shared> gaussian =
        create_gaussian_image(
            make_vector<double>(16, 16),
            make_vector<double>(2, 2));

    image<2,double,unique> src;
    create_image(src, make_vector<unsigned>(101, 101));
    fill_pixels(src, 0.);
    set_spatial_mapping(src, make_vector<double>(0, 0),
        make_vector<double>(2, 2));
    get_pixel_ref(src, make_vector<unsigned>(50, 50)) = 12;

    image<2,double,unique> blurred;
    create_image(blurred, make_vector<unsigned>(101, 101));
    set_spatial_mapping(blurred, make_vector<double>(0, 0),
        make_vector<double>(2, 2));

    double blurred_total = 0;
    for (unsigned i = 0; i != 101; ++i)
    {
        for (unsigned j = 0; j != 101; ++j)
        {
            double blurred_value =
                compute_gaussian_blurred_pixel(
                    as_const_view(src), as_const_view(gaussian),
                    make_vector<int>(i, j));
            double dx = std::fabs(50.5 - (i + 0.5));
            double dy = std::fabs(50.5 - (j + 0.5));
            double lower_bound =
                evaluate_gaussian(
                    make_vector<double>(dx + 0.5, dy + 0.5),
                    make_vector<double>(8, 8)) * 12.;
            double upper_bound =
                evaluate_gaussian(
                    make_vector<double>(dx - 0.5, dy - 0.5),
                    make_vector<double>(8, 8)) * 12.;
            BOOST_CHECK(
                blurred_value >= lower_bound - 0.001 &&
                blurred_value <= upper_bound + 0.001);
            get_pixel_ref(blurred, make_vector(i, j)) = blurred_value;
            blurred_total += blurred_value;
        }
    }
    CRADLE_CHECK_WITHIN_TOLERANCE(blurred_total, 12., 0.0001);

    image<2,cradle::uint8_t,shared> discretized;
    discretize(discretized, blurred, 255);

    boost::filesystem::path const ref_file =
        test_data_directory() / "imaging/blurred_pixel.png";

    image<2,cradle::uint8_t,shared> ref_img;
    read_image_file(ref_img, ref_file);

    CRADLE_CHECK_IMAGE(discretized, ref_img.pixels.view,
        ref_img.pixels.view + product(ref_img.size));
}
