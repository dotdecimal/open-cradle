#include <cradle/imaging/bounds.hpp>
#include <cradle/imaging/view_transforms.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE get_memory_bounds
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(get_memory_bounds_test)
{
    unsigned const s = 10;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));

    auto view = cast_storage_type<cradle::view>(img);

    memory_bounds<cradle::uint8_t> bounds;

    bounds = get_quick_bounds(img);
    BOOST_CHECK_EQUAL(bounds.begin, img.pixels.ptr);
    BOOST_CHECK_EQUAL(bounds.end, img.pixels.ptr + product(img.size));

    bounds = get_quick_bounds(aligned_view(flipped_view(view, 0)));
    BOOST_CHECK_EQUAL(bounds.begin, img.pixels.ptr);
    BOOST_CHECK_EQUAL(bounds.end, img.pixels.ptr + product(img.size));

    bounds = get_quick_bounds(aligned_view(flipped_view(view, 1)));
    BOOST_CHECK_EQUAL(bounds.begin, img.pixels.ptr);
    BOOST_CHECK_EQUAL(bounds.end, img.pixels.ptr + product(img.size));

    bounds =
        get_quick_bounds(aligned_view(flipped_view(flipped_view(view, 1), 0)));
    BOOST_CHECK_EQUAL(bounds.begin, img.pixels.ptr);
    BOOST_CHECK_EQUAL(bounds.end, img.pixels.ptr + product(img.size));
}
