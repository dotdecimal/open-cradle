#include <cradle/imaging/variant.hpp>
#include <cradle/imaging/utilities.hpp>

#define BOOST_TEST_MODULE variant
#include <cradle/imaging/test.hpp>

using namespace cradle;

struct size_fn
{
    std::size_t result;
    template<class Pixel, class SP>
    void operator()(image<2,Pixel,SP> const& img)
    { result = sizeof(Pixel); }
};

//template<class Pixel>
//void test_dispatch()
//{
//    unsigned const s = 3;
//    image<2,Pixel,unique> src;
//    create_image(src, vector2u(s, s));
//    size_fn f;
//    f.result = 1025;
//    image<2,variant,shared> v = as_variant(share(src));
//    apply_fn_to_variant(f, v);
//    BOOST_CHECK_EQUAL(f.result, sizeof(Pixel));
//}
//
//BOOST_AUTO_TEST_CASE(dispatch)
//{
//    test_dispatch<int8>();
//    test_dispatch<uint8>();
//    test_dispatch<rgb<uint8> >();
//    test_dispatch<rgba<uint8> >();
//    test_dispatch<double>();
//    test_dispatch<rgb<float> >();
//    test_dispatch<int16>();
//    test_dispatch<uint16>();
//}

template<class Pixel>
void test_gray_dispatch()
{
    unsigned const s = 3;
    image<2,Pixel,unique> src;
    create_image(src, make_vector(s, s));
    size_fn f;
    f.result = 1025;
    apply_fn_to_gray_variant(f, as_variant(as_const_view(src)));
    BOOST_CHECK_EQUAL(f.result, sizeof(Pixel));
}

BOOST_AUTO_TEST_CASE(gray_dispatch)
{
    test_gray_dispatch<cradle::int8_t>();
    test_gray_dispatch<cradle::uint8_t>();
    test_gray_dispatch<double>();
    test_gray_dispatch<cradle::int16_t>();
    test_gray_dispatch<cradle::uint16_t>();
}

BOOST_AUTO_TEST_CASE(copying)
{
    unsigned const s = 3;
    image<2,double,unique> tmp;
    create_image(tmp, make_vector(s, s));
    sequential_fill(tmp, 4.1, 0.);
    auto src = as_variant(share(tmp));
    auto dst = make_eager_image_copy(src);
    auto dst_view = cast_image<image<2,double,const_view> >(dst);
    // TODO: more exhaustive checking (should have a function for this)
    BOOST_CHECK_EQUAL(dst.size, src.size);
    BOOST_CHECK_EQUAL(dst.value_mapping, src.value_mapping);
    for (unsigned i = 0; i < s * s; ++i)
        BOOST_CHECK_EQUAL(dst_view.pixels[i], 4.1);
}
