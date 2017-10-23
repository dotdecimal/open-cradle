#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/image.hpp>

#define BOOST_TEST_MODULE geometry
#include <cradle/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(default_bounding_box_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));

    box2d box = get_bounding_box(img);
    CRADLE_CHECK_ALMOST_EQUAL(box.corner, make_vector<double>(0, 0));
    CRADLE_CHECK_ALMOST_EQUAL(box.size, make_vector<double>(10, 10));
}

BOOST_AUTO_TEST_CASE(bounding_box_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));

    set_spatial_mapping(
        img, make_vector<double>(-2, -6), make_vector<double>(1, 2));
    img.value_mapping = linear_function<double>(-2, 2);

    box2d box = get_bounding_box(img);
    CRADLE_CHECK_ALMOST_EQUAL(box.corner, make_vector<double>(-2, -6));
    CRADLE_CHECK_ALMOST_EQUAL(box.size, make_vector<double>(10, 20));
}
