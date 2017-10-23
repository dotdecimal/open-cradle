#include <cradle/imaging/merge_slices.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/common.hpp>
#include <cradle/io/pxio_io.hpp>
#include <cradle/imaging/statistics.hpp>

#define BOOST_TEST_MODULE merge_slices
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(along_x_test)
{
    unsigned const s = 3, n_slices = 7;
    std::vector<image_slice<2,cradle::uint8_t,shared> > slices(n_slices);
    double slice_locations[n_slices] = { 0, 2, 4, 6, 8, 10, 12 };

    for (int i = 0; i < n_slices; ++i)
    {
        image<2,cradle::uint8_t,unique> tmp;
        create_image(tmp, make_vector(s, s));
        fill_pixels(tmp, cradle::uint8_t(i));
        slices[i].content = share(tmp);
        slices[i].position = slice_locations[i];
        set_spatial_mapping(slices[i].content, make_vector<double>(1, 0),
            make_vector<double>(3, 1));
        set_value_mapping(slices[i].content, 1, 1, no_units);
        slices[i].axis = 0;
    }

    image<3,cradle::uint8_t,shared> merged = merge_slices(slices);

    BOOST_CHECK_EQUAL(merged.size, make_vector(n_slices, s, s));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(0, 0, 0)),
        make_vector<double>(-1, 1, 0));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(1, 1, 1)),
        make_vector<double>(1, 4, 1));
    BOOST_CHECK_EQUAL(merged.value_mapping,
        linear_function<double>(1, 1));
}

BOOST_AUTO_TEST_CASE(along_y_test)
{
    unsigned const s = 3, n_slices = 7;
    std::vector<image_slice<2,cradle::uint8_t,shared> > slices(n_slices);
    double slice_locations[n_slices] = { 0, 2, 4, 6, 8, 10, 12 };

    for (int i = 0; i < n_slices; ++i)
    {
        image<2,cradle::uint8_t,unique> tmp;
        create_image(tmp, make_vector(s, s));
        fill_pixels(tmp, cradle::uint8_t(i));
        slices[i].content = share(tmp);
        slices[i].position = slice_locations[i];
        set_spatial_mapping(slices[i].content, make_vector<double>(1, 0),
            make_vector<double>(3, 1));
        set_value_mapping(slices[i].content, 1, 1, no_units);
        slices[i].axis = 1;
    }

    image<3,cradle::uint8_t,shared> merged = merge_slices(slices);

    BOOST_CHECK_EQUAL(merged.size, make_vector(s, n_slices, s));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(0, 0, 0)),
        make_vector<double>(1, -1, 0));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(1, 1, 1)),
        make_vector<double>(4, 1, 1));
    BOOST_CHECK_EQUAL(merged.value_mapping,
        linear_function<double>(1, 1));
}

BOOST_AUTO_TEST_CASE(along_z_test)
{
    unsigned const s = 3, n_slices = 7;
    std::vector<image_slice<2,cradle::uint8_t,shared> > slices(n_slices);
    double slice_locations[n_slices] = { 0, 2, 4, 6, 8, 10, 12 };

    for (int i = 0; i < n_slices; ++i)
    {
        image<2,cradle::uint8_t,unique> tmp;
        create_image(tmp, make_vector(s, s));
        fill_pixels(tmp, cradle::uint8_t(i));
        slices[i].content = share(tmp);
        slices[i].position = slice_locations[i];
        set_spatial_mapping(slices[i].content, make_vector<double>(1, 0),
            make_vector<double>(3, 1));
        set_value_mapping(slices[i].content, 1, 1, no_units);
        slices[i].axis = 2;
    }

    image<3,cradle::uint8_t,shared> merged = merge_slices(slices);

    BOOST_CHECK_EQUAL(merged.size, make_vector(s, s, n_slices));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(0, 0, 0)),
        make_vector<double>(1, 0, -1));
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(merged),
            make_vector<double>(1, 1, 1)),
        make_vector<double>(4, 1, 1));
    BOOST_CHECK_EQUAL(merged.value_mapping,
        linear_function<double>(1, 1));
}
