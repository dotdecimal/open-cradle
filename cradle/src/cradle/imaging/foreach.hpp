#ifndef CRADLE_IMAGING_FOREACH_HPP
#define CRADLE_IMAGING_FOREACH_HPP

#include <cradle/imaging/image.hpp>

namespace cradle {

// FOREACH - one image

template<unsigned Dimension>
struct foreach_pixel_algorithm
{
    template<unsigned N, class Pixel, class Storage, class Functor>
    static void apply(image<N,Pixel,Storage> const& img,
        typename image<N,Pixel,Storage>::iterator_type start,
        Functor& fn)
    {
        for (unsigned i = 0; i < img.size[Dimension]; ++i)
        {
            foreach_pixel_algorithm<Dimension - 1>::apply(img, start, fn);
            start += img.step[Dimension];
        }
    }
};
template<>
struct foreach_pixel_algorithm<0>
{
    template<unsigned N, class Pixel, class Storage, class Functor>
    static void apply(image<N,Pixel,Storage> const& img,
        typename image<N,Pixel,Storage>::iterator_type start,
        Functor& fn)
    {
        typename image<N,Pixel,Storage>::iterator_type end =
            start + img.step[0] * img.size[0];
        typename image<N,Pixel,Storage>::iterator_type i = start;
        typename image<N,Pixel,Storage>::step_type step = img.step[0];
        for (; i != end; i += step)
            fn(*i);
    }
};

template<unsigned N, class Pixel, class Storage, class Functor>
void foreach_pixel(image<N,Pixel,Storage> const& img, Functor& fn)
{
    foreach_pixel_algorithm<N - 1>::apply(img, get_iterator(img.pixels), fn);
}

// FOREACH - two images

template<unsigned Dimension>
struct foreach_pixel2_algorithm
{
    template<unsigned N, class Pixel1, class Storage1,
        class Pixel2, class Storage2, class Functor>
    static void apply(
        image<N,Pixel1,Storage1> const& img1,
        typename image<N,Pixel1,Storage1>::iterator_type start1,
        image<N,Pixel2,Storage2> const& img2,
        typename image<N,Pixel2,Storage2>::iterator_type start2,
        Functor& fn)
    {
        for (unsigned i = 0; i < img1.size[Dimension]; ++i)
        {
            foreach_pixel2_algorithm<Dimension - 1>::apply(
                img1, start1, img2, start2, fn);
            start1 += img1.step[Dimension];
            start2 += img2.step[Dimension];
        }
    }
};
template<>
struct foreach_pixel2_algorithm<0>
{
    template<unsigned N, class Pixel1, class Storage1,
        class Pixel2, class Storage2, class Functor>
    static void apply(
        image<N,Pixel1,Storage1> const& img1,
        typename image<N,Pixel1,Storage1>::iterator_type start1,
        image<N,Pixel2,Storage2> const& img2,
        typename image<N,Pixel2,Storage2>::iterator_type start2,
        Functor& fn)
    {
        typename image<N,Pixel1,Storage1>::iterator_type end1 =
            start1 + img1.step[0] * img1.size[0];
        typename image<N,Pixel1,Storage1>::iterator_type i1 = start1;
        typename image<N,Pixel1,Storage1>::step_type step1 = img1.step[0];
        typename image<N,Pixel2,Storage2>::iterator_type i2 = start2;
        typename image<N,Pixel2,Storage2>::step_type step2 = img2.step[0];
        for (; i1 != end1; i1 += step1, i2 += step2)
            fn(*i1, *i2);
    }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Functor>
void foreach_pixel2(image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2, Functor& fn)
{
    assert(img1.size == img2.size);
    foreach_pixel2_algorithm<N - 1>::apply(img1, get_iterator(img1.pixels),
        img2, get_iterator(img2.pixels), fn);
}

// FOREACH - three images

template<unsigned Dimension>
struct foreach_pixel3_algorithm
{
    template<unsigned N, class Pixel1, class Storage1,
        class Pixel2, class Storage2, class Pixel3, class Storage3,
        class Functor>
    static void apply(
        image<N,Pixel1,Storage1> const& img1,
        typename image<N,Pixel1,Storage1>::iterator_type start1,
        image<N,Pixel2,Storage2> const& img2,
        typename image<N,Pixel2,Storage2>::iterator_type start2,
        image<N,Pixel3,Storage3> const& img3,
        typename image<N,Pixel3,Storage3>::iterator_type start3,
        Functor& fn)
    {
        for (unsigned i = 0; i < img1.size[Dimension]; ++i)
        {
            foreach_pixel3_algorithm<Dimension - 1>::apply(
                img1, start1, img2, start2, img3, start3, fn);
            start1 += img1.step[Dimension];
            start2 += img2.step[Dimension];
            start3 += img3.step[Dimension];
        }
    }
};
template<>
struct foreach_pixel3_algorithm<0>
{
    template<unsigned N, class Pixel1, class Storage1,
        class Pixel2, class Storage2, class Pixel3, class Storage3,
        class Functor>
    static void apply(
        image<N,Pixel1,Storage1> const& img1,
        typename image<N,Pixel1,Storage1>::iterator_type start1,
        image<N,Pixel2,Storage2> const& img2,
        typename image<N,Pixel2,Storage2>::iterator_type start2,
        image<N,Pixel3,Storage3> const& img3,
        typename image<N,Pixel3,Storage3>::iterator_type start3,
        Functor& fn)
    {
        typename image<N,Pixel1,Storage1>::iterator_type end1 =
            start1 + img1.step[0] * img1.size[0];
        typename image<N,Pixel1,Storage1>::iterator_type i1 = start1;
        typename image<N,Pixel1,Storage1>::step_type step1 = img1.step[0];
        typename image<N,Pixel2,Storage2>::iterator_type i2 = start2;
        typename image<N,Pixel2,Storage2>::step_type step2 = img2.step[0];
        typename image<N,Pixel3,Storage3>::iterator_type i3 = start3;
        typename image<N,Pixel3,Storage3>::step_type step3 = img3.step[0];
        for (; i1 != end1; i1 += step1, i2 += step2, i3 += step3)
            fn(*i1, *i2, *i3);
    }
};

template<unsigned N, class Pixel1, class Storage1,
    class Pixel2, class Storage2, class Pixel3, class Storage3,
    class Functor>
void foreach_pixel3(image<N,Pixel1,Storage1> const& img1,
    image<N,Pixel2,Storage2> const& img2, image<N,Pixel3,Storage3> const& img3,
    Functor& fn)
{
    assert(img1.size == img2.size && img1.size == img3.size);
    foreach_pixel3_algorithm<N - 1>::apply(img1, get_iterator(img1.pixels),
        img2, get_iterator(img2.pixels), img3, get_iterator(img3.pixels), fn);
}

}

#endif
