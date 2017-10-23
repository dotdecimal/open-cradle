#ifndef CRADLE_IMAGING_FORWARD_HPP
#define CRADLE_IMAGING_FORWARD_HPP

#include <cradle/common.hpp>

namespace cradle {

// Due to the templating, it's not really possible to forward declare concrete
// image types, so we need some definitions. However, this is still much less
// than would be included from the full header files.

template<unsigned N, class Pixel, class Storage>
struct image;

template<class Pixel>
struct shared_pointer
{
    ownership_holder ownership;
    Pixel const* view;
};

template<unsigned N, class Pixel>
struct shared_pointer_type
{
    typedef shared_pointer<Pixel> type;
};
template<unsigned N, class Pixel>
struct shared_iterator_type
{
    typedef Pixel const* type;
};
template<unsigned N, class Pixel>
struct shared_reference_type
{
    typedef Pixel const& type;
};
template<unsigned N, class Pixel>
struct shared_step_type
{
    typedef ptrdiff_t type;
};
struct shared
{
    template<unsigned N, class Pixel>
    struct pointer_type : shared_pointer_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct iterator_type : shared_iterator_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct reference_type : shared_reference_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct step_type : shared_step_type<N,Pixel> {};
};

template<unsigned N, class Pixel>
struct const_view_pointer_type
{
    typedef Pixel const* type;
};
template<unsigned N, class Pixel>
struct const_view_iterator_type
{
    typedef Pixel const* type;
};
template<unsigned N, class Pixel>
struct const_view_reference_type
{
    typedef Pixel const& type;
};
template<unsigned N, class Pixel>
struct const_view_step_type
{
    typedef ptrdiff_t type;
};
struct const_view
{
    template<unsigned N, class Pixel>
    struct pointer_type : const_view_pointer_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct iterator_type : const_view_iterator_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct reference_type : const_view_reference_type<N,Pixel> {};
    template<unsigned N, class Pixel>
    struct step_type : const_view_step_type<N,Pixel> {};
};

// VARIANTS

struct variant
{
};

typedef image<1,variant,shared> image1;
typedef image<2,variant,shared> image2;
typedef image<3,variant,shared> image3;

typedef image<1,variant,const_view> image_view1;
typedef image<2,variant,const_view> image_view2;
typedef image<3,variant,const_view> image_view3;

// SLICING

template<unsigned N, class Pixel, class Storage>
struct image_slice;

// slice typedefs
typedef image_slice<1,variant,shared> image1_slice;
typedef image_slice<2,variant,shared> image2_slice;
typedef image_slice<3,variant,shared> image3_slice;

// slice list typedefs
typedef std::vector<image_slice<1,variant,shared> > image1_slice_list;
typedef std::vector<image_slice<2,variant,shared> > image2_slice_list;
typedef std::vector<image_slice<3,variant,shared> > image3_slice_list;

}

#endif
