#include <cradle/imaging/sample.hpp>
#include <cradle/imaging/geometry.hpp>

#define BOOST_TEST_MODULE sample
#include <cradle/imaging/test.hpp>

using namespace cradle;

template<class View>
void do_raw_gray_text(View const& view)
{
    BOOST_CHECK_EQUAL(int(raw_image_sample(view,
        make_vector<double>(-2.9, 0.1)).get()),  0. );
    BOOST_CHECK_EQUAL(int(raw_image_sample(view,
        make_vector<double>( 0  , 0.9)).get()) , 1. );
    BOOST_CHECK_EQUAL(int(raw_image_sample(view,
        make_vector<double>(-2.9, 1.1)).get()),  3. );

    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>(-2.9, 0.1)).get()),  0.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>(-2  , 0.5)).get()),  0.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>(-1  , 0.5)).get()),  0.5 );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>(-2  , 1  )).get()),  1.5 );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>(-1  , 1  )).get()),  2.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>( 2.9, 1  )).get()),  3.5 );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>( 2.9, 0.9)).get()),  3.2 );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>( 0  , 2.9)).get()),  7.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>( 0.1, 2.9)).get()),  7.05);
    CRADLE_CHECK_ALMOST_EQUAL(double(raw_interpolated_image_sample(view,
        make_vector<double>( 2.9, 2.9)).get()),  8.  );

    BOOST_CHECK(!raw_image_sample_over_box(view,
        box2d(make_vector<double>(-4, 0), make_vector<double>(0.1, 0.1))));
    BOOST_CHECK(!raw_image_sample_over_box(view,
        box2d(make_vector<double>(3, 0), make_vector<double>(0.1, 0.1))));
    BOOST_CHECK(!raw_image_sample_over_box(view,
        box2d(make_vector<double>(0, -1), make_vector<double>(0.1, 0.1))));
    BOOST_CHECK(!raw_image_sample_over_box(view,
        box2d(make_vector<double>(0, 3), make_vector<double>(0.1, 0.1))));

    CRADLE_CHECK_ALMOST_EQUAL(double(raw_image_sample_over_box(view,
        box2d(make_vector<double>(-2, 0.1),
            make_vector<double>(0.1, 0.1))).get()), 0.);
}

template<class View>
void do_gray_test(View const& view)
{
    BOOST_CHECK(!image_sample(view, make_vector<double>(-3.1,  0  )));
    BOOST_CHECK(!image_sample(view, make_vector<double>( 3.1,  0  )));
    BOOST_CHECK(!image_sample(view, make_vector<double>( 0  , -0.1)));
    BOOST_CHECK(!image_sample(view, make_vector<double>( 0  ,  3.1)));

    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample(view,
        make_vector<double>(-2.9, 0.1)).get()), -1. );
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample(view,
        make_vector<double>( 0  , 0.9)).get()), -0.5);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample(view,
        make_vector<double>(-2.9, 1.1)).get()),  0.5);

    BOOST_CHECK(
        !interpolated_image_sample(view, make_vector<double>(-3.1,  0  )));
    BOOST_CHECK(
        !interpolated_image_sample(view, make_vector<double>( 3.1,  0  )));
    BOOST_CHECK(
        !interpolated_image_sample(view, make_vector<double>( 0  , -0.1)));
    BOOST_CHECK(
        !interpolated_image_sample(view, make_vector<double>( 0  ,  3.1)));

    CRADLE_CHECK_ALMOST_EQUAL(double(interpolated_image_sample(view,
        make_vector<double>(-2.9, 0.1)).get()), -1.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(interpolated_image_sample(view,
        make_vector<double>(-2  , 0.5)).get()), -1.  );
    CRADLE_CHECK_ALMOST_EQUAL(double(interpolated_image_sample(view,
        make_vector<double>(-1  , 0.5)).get()), -0.75);

    BOOST_CHECK(!image_sample_over_box(view,
        box2d(make_vector<double>(-4, 0), make_vector<double>(0.1, 0.1))));

    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(0, 1), make_vector<double>(2, 1))).get()),
        1.25);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(0.5, 1), make_vector<double>(2, 1))).get()),
        1.375);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(-4, 0), make_vector<double>(4, 0.5))).get()),
        -5. / 6);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(-2, 0.1), make_vector<double>(0.1, 0.1))).
        get()), -1.);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(0, 1.5), make_vector<double>(4, 2))).get()),
        7. / 3);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(0, 1.5), make_vector<double>(3, 1.5)))
        .get()), 7. / 3);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(-4, -1), make_vector<double>(8, 5))).get()),
        1.);
    CRADLE_CHECK_ALMOST_EQUAL(double(image_sample_over_box(view,
        box2d(make_vector<double>(-4, -1), make_vector<double>(6, 3.5))).get()),
        0.6);
}

BOOST_AUTO_TEST_CASE(gray_test)
{
    unsigned const s = 3;
    image<2,cradle::uint8_t,unique> img;
    create_image(img, make_vector(s, s));
    img.value_mapping = linear_function<double>(-1, 0.5);
    set_spatial_mapping(
        img, make_vector<double>(-3, 0), make_vector<double>(2, 1));
    sequential_fill(img, 0, 1);

    do_raw_gray_text(img);
    do_gray_test(img);

    do_gray_test(as_variant(as_const_view(img)));
}
