#include <cradle/imaging/level_window.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE level_window
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(level_window_test)
{
    unsigned const s = 3;
    image<2,cradle::uint16_t,unique> src;
    create_image(src, make_vector(s, s));
    src.value_mapping = linear_function<double>(-2, 2);
    sequential_fill(src, 1, 1);

    {
    auto result = apply_level_window(src, 8, 6);
    cradle::uint8_t correct_results[] = {
        0,   0,   0,
       42, 127, 212,
      255, 255, 255,
    };
    CRADLE_CHECK_IMAGE(result, correct_results, correct_results + s * s);
    }

    {
    image<2,cradle::uint8_t,unique> result;
    create_image(result, make_vector(s, s));
    apply_paletted_level_window(result, src, 8, 6);
    cradle::uint8_t correct_results[] = {
        0,   0,   0,
       42, 127, 212,
      255, 255, 255,
    };
    CRADLE_CHECK_IMAGE(result, correct_results, correct_results + s * s);
    }
}
