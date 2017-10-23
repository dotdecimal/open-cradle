#include <cradle/imaging/statistics.hpp>
#include <cradle/imaging/image.hpp>
#include <cradle/common.hpp>

#define BOOST_TEST_MODULE statistics
#include <cradle/imaging/test.hpp>

using namespace cradle;

BOOST_AUTO_TEST_CASE(uint8_test)
{
    unsigned const s = 3;
    int initial_n = 61;
    image<3,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s, s));
    img.value_mapping = linear_function<double>(1, 2);
    sequential_fill(img, initial_n, 1);

    auto min_max = raw_image_min_max(img);
    BOOST_CHECK(min_max);
    BOOST_CHECK_EQUAL(int(get(min_max).min), initial_n);
    BOOST_CHECK_EQUAL(int(get(min_max).max), initial_n + 26);
    sequential_fill(img, initial_n, -1);

    min_max = raw_image_min_max(img);
    BOOST_CHECK(min_max);
    BOOST_CHECK_EQUAL(int(get(min_max).min), initial_n - 26);
    BOOST_CHECK_EQUAL(int(get(min_max).max), initial_n);

    auto mapped_min_max = image_min_max(img);
    CRADLE_CHECK_ALMOST_EQUAL(get(mapped_min_max).min,
        (initial_n - 26) * 2 + 1.);
    CRADLE_CHECK_ALMOST_EQUAL(get(mapped_min_max).max, initial_n * 2 + 1.);

    auto stats = raw_image_statistics(img);
    BOOST_CHECK(stats.min);
    BOOST_CHECK_EQUAL(int(get(stats.min)), initial_n - 26);
    BOOST_CHECK(stats.max);
    BOOST_CHECK_EQUAL(int(get(stats.max)), initial_n);
    BOOST_CHECK(stats.mean);
    BOOST_CHECK_EQUAL(int(get(stats.mean)), initial_n - 13);

    auto mapped_stats = image_statistics(img);
    BOOST_CHECK(mapped_stats.min);
    CRADLE_CHECK_ALMOST_EQUAL(get(mapped_stats.min),
        (initial_n - 26) * 2 + 1.);
    BOOST_CHECK(mapped_stats.max);
    CRADLE_CHECK_ALMOST_EQUAL(get(mapped_stats.max), initial_n * 2 + 1.);
    BOOST_CHECK(mapped_stats.mean);
    CRADLE_CHECK_ALMOST_EQUAL(get(mapped_stats.mean),
        (initial_n - 13) * 2 + 1.);
}

BOOST_AUTO_TEST_CASE(float_test)
{
    float random_data[] = {
        17.0f, 12.1f, 43.2f,
        16.2f, 25.0f, 34.7f,
        71.3f, 27.0f, 19.1f,
    };
    image<2,float,const_view> view = make_const_view(random_data,
        make_vector<unsigned>(3, 3));

    auto min_max = raw_image_min_max(view);
    BOOST_CHECK(min_max);
    BOOST_CHECK_EQUAL(get(min_max).min, 12.1f);
    BOOST_CHECK_EQUAL(get(min_max).max, 71.3f);

    auto stats = image_statistics(view);
    BOOST_CHECK(stats.min);
    BOOST_CHECK_EQUAL(get(stats.min), 12.1f);
    BOOST_CHECK(stats.max);
    BOOST_CHECK_EQUAL(get(stats.max), 71.3f);
    BOOST_CHECK(stats.mean);
    CRADLE_CHECK_WITHIN_TOLERANCE(get(stats.mean), 265.6 / 9, 0.0001);
}
