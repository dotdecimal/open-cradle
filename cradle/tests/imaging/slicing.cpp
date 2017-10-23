#include <cradle/imaging/slicing.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/common.hpp>
#include <cradle/imaging/view_transforms.hpp>

#define BOOST_TEST_MODULE slicing
#include <cradle/imaging/test.hpp>

using namespace cradle;

template<unsigned N, class T, class SP>
void check_slice(image<N,T,SP> const& img, unsigned axis, int at,
    T const* ref_data, size_t ref_data_size)
{
    image_slice<N-1,T,SP> slice = sliced_view(img, axis, at);
    CRADLE_CHECK_IMAGE(slice.content, ref_data, ref_data + ref_data_size);
}

BOOST_AUTO_TEST_CASE(simple_test)
{
    // create a 3x3x3 cube filled with increasing values
    unsigned const s = 3;
    image<3,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s, s));
    sequential_fill(img, 0, 1);

    image<3,cradle::uint8_t,const_view> view =
        cast_storage_type<const_view>(img);

    // test slicing along each axis
    {
        cradle::uint8_t trace0[] = { 1, 4, 7, 10, 13, 16, 19, 22, 25 };
        check_slice(view, 0, 1, trace0, sizeof(trace0));
        cradle::uint8_t trace1[] = { 0, 1, 2, 9, 10, 11, 18, 19, 20 };
        check_slice(view, 1, 0, trace1, sizeof(trace1));
        cradle::uint8_t trace2[] = { 18, 19, 20, 21, 22, 23, 24, 25, 26 };
        check_slice(view, 2, 2, trace2, sizeof(trace2));
    }

    // run the same slicing tests on a sumsampled view
    image<3,cradle::uint8_t,const_view> subview = subsampled_view(
        cast_image<image<3,cradle::uint8_t,const_view> >(view),
        make_vector<unsigned>(2, 2, 2));
    {
        cradle::uint8_t trace0[] = { 0, 6, 18, 24 };
        check_slice(subview, 0, 0, trace0, sizeof(trace0));
        cradle::uint8_t trace1[] = { 0, 2, 18, 20 };
        check_slice(subview, 1, 0, trace1, sizeof(trace1));
        cradle::uint8_t trace2[] = { 18, 20, 24, 26 };
        check_slice(subview, 2, 1, trace2, sizeof(trace2));
    }
}

BOOST_AUTO_TEST_CASE(spatial_test)
{
    using namespace cradle;

    unsigned const s = 3;
    image<3,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s, s));
    set_spatial_mapping(
        img, make_vector<double>(1, 3, 2), make_vector<double>(3, 1, 2));
    sequential_fill(img, 0, 1);

    image_slice<2,cradle::uint8_t,const_view> sliced_v = sliced_view(
        cast_image<image<3,cradle::uint8_t,const_view> >(img), 1, 1);
    CRADLE_CHECK_ALMOST_EQUAL(
        transform_point(get_spatial_mapping(sliced_v.content),
            make_vector<double>(1, 3)),
        make_vector<double>(4, 8));
    CRADLE_CHECK_ALMOST_EQUAL(sliced_v.position, 4.5);
    CRADLE_CHECK_ALMOST_EQUAL(sliced_v.thickness, 1.);
    BOOST_CHECK_EQUAL(sliced_v.axis, 1);
}
