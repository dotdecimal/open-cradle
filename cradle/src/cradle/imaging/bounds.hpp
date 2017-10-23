#ifndef CRADLE_IMAGING_BOUNDS_HPP
#define CRADLE_IMAGING_BOUNDS_HPP

#include <utility>
#include <cradle/imaging/image.hpp>

namespace cradle {

// Sometimes it's useful to have a quick test that tells if an image iterator
// can safely be dereferenced (without crashing the program) without
// guaranteeing that the iterator is actually part of the image.

// Given an iterator type, this returns the type of information needed to
// perform a quick bounds check.
template<class Iterator>
struct quick_bounds_check_type
{
};

// The info needed for a normal pointer is the bounds of the memory occupied
// by the image.
template<class Pixel>
struct memory_bounds
{
    Pixel const* begin;
    Pixel const* end;
};
template<class Pixel>
struct quick_bounds_check_type<Pixel const*>
{
    typedef memory_bounds<Pixel> type;
};
template<class Pixel>
struct quick_bounds_check_type<Pixel*>
{
    typedef memory_bounds<Pixel> type;
};

// Given an image with normal pointers as iterators, this returns the memory
// bounds of the image.
template<unsigned N, class Pixel, class Storage>
memory_bounds<Pixel> get_quick_bounds(image<N,Pixel,Storage> const& img)
{
    vector<N,unsigned> lower_index, upper_index;
    for (unsigned i = 0; i < N; ++i)
    {
        if (img.step[i] < 0)
        {
            lower_index[i] = img.size[i] - 1;
            upper_index[i] = 0;
        }
        else
        {
            lower_index[i] = 0;
            upper_index[i] = img.size[i] - 1;
        }
    }
    memory_bounds<Pixel> bounds;
    bounds.begin = get_pixel_iterator(img, lower_index);
    bounds.end = get_pixel_iterator(img, upper_index) + 1;
    return bounds;
}

// Check if a pointer is within its bounds.
template<class Pixel>
bool within_bounds(memory_bounds<Pixel> const& bounds, Pixel const* ptr)
{
    return ptr >= bounds.begin && ptr < bounds.end;
}

}

#endif
