#ifndef CRADLE_IMAGING_ISOBANDS_HPP
#define CRADLE_IMAGING_ISOBANDS_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/imaging/image.hpp>
#include <vector>

namespace cradle {

// Given a grayscale image and two value levels, this generates a set of
// triangles that fills the region where the image values are between those
// two levels.
api(fun with(Pixel:variant;Storage:shared))
template<class Pixel, class Storage>
// The list of triangles that fill the region where the image values are between the two given levels.
std::vector<triangle<2,double> >
compute_isobands(
    // The image that the isobands will occupy.
    image<2,Pixel,Storage> const& img,
    // The minimum image value that the isobands will cover.
    double low,
    // The maximum image value that the isobands will cover.
    double high);

api(struct with(N:2;T:double))
template<unsigned N, typename T>
struct colored_vertex
{
    vector<N,T> position;
    rgba8 color;
};

template<unsigned N, typename T>
struct colored_triangle : c_array<3,colored_vertex<N,T> >
{
    typedef colored_vertex<N,T> vertex_type;

    colored_triangle() {}

    colored_triangle(
        vertex_type const& v0, vertex_type const& v1, vertex_type const& v2)
    {
        (*this)[0] = v0;
        (*this)[1] = v1;
        (*this)[2] = v2;
    }
};

// hash function
} namespace std {
    template<unsigned N, class T>
    struct hash<cradle::colored_triangle<N,T> >
    {
        size_t operator()(cradle::colored_triangle<N,T> const& x) const
        {
            return alia::combine_hashes(
                alia::invoke_hash(x[0]),
                alia::combine_hashes(
                    alia::invoke_hash(x[1]),
                    alia::invoke_hash(x[2])));
        }
    };
} namespace cradle {

// This is similar to compute_isobands, but it also associates colors with
// the high and low levels of the isoband. The vertices of the triangles it
// produces are colored based on this shading.
api(fun with(Pixel:variant;Storage:shared))
template<class Pixel, class Storage>
std::vector<colored_triangle<2,double> >
compute_shaded_isobands(image<2,Pixel,Storage> const& img,
    rgba8 const& low_color, double low_level,
    rgba8 const& high_color, double high_level);

}

#include <cradle/imaging/isobands.ipp>

#endif
