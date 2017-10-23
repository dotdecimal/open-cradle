#include <cradle/test.hpp>
#include <cradle/imaging/foreach.hpp>

namespace cradle {

template<class Pixel>
struct sequential_fill_fn
{
    Pixel value;
    Pixel increment;
    void operator()(Pixel& dst)
    {
        dst = value;
        value += increment;
    }
};
template<unsigned N, class Pixel, class SP, class Value>
void sequential_fill(image<N,Pixel,SP>& img, Value initial, Value increment)
{
    sequential_fill_fn<Pixel> fn;
    fn.value = initial;
    fn.increment = increment;
    foreach_pixel(img, fn);
}

template<class Iter>
struct image_checker
{
    image_checker() : same(true) {}
    Iter i;
    bool same;
    template<class Pixel>
    void operator()(Pixel const& p)
    {
        if (p != *i)
            same = false;
        ++i;
    }
};

// Check that an image matches the given reference data.
template<unsigned N, class T, class SP, class Iter>
bool check_image(image<N,T,SP> const& img, Iter begin, Iter end)
{
    image_checker<Iter> checker;
    checker.i = begin;
    foreach_pixel(img, checker);
    return checker.same;
}
#define CRADLE_CHECK_IMAGE(view, ref_begin, ref_end) \
  { BOOST_CHECK_EQUAL(product((view).size), (ref_end) - (ref_begin)); \
    BOOST_CHECK(cradle::check_image(view, ref_begin, ref_end)); }

}
