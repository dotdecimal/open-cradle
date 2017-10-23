#ifndef CRADLE_GEOMETRY_INTERSECTION_HPP
#define CRADLE_GEOMETRY_INTERSECTION_HPP

#include <cradle/geometry/polygonal.hpp>

namespace cradle {

// Compute the intersection between a plane and a line segment.
// Note that for purposes of uniformity, the case where the line segment lies
// exactly on the plane is treated as no intersection.
template<typename T>
optional<vector<3,T> >
intersection(plane<T> const& plane, line_segment<3,T> const& segment);

// Compute the intersection between a plane and a triangle.
// Note that for purposes of uniformity, the case where the triangle lies
// exactly on the plane is treated as no intersection. (The case where one of
// the triangle's edges lies exactly on the plane depends on which side of the
// plane the rest of the triangle is on.)
template<typename T>
optional<line_segment<3,T> >
intersection(plane<T> const& plane, triangle<3,T> const& tri);

// ray_box_intersection represents the intersection between a ray and a box.
api(struct with(N:2,3;T:double))
template<unsigned N, typename T>
struct ray_box_intersection
{
    // This stores the number of times that the ray intersects the box.
    //    0: No intersection.
    //    1: The ray originates inside the box.  exit_distance stores the
    //       distance at which the ray exits the box.  entrance_distance is 0.
    //    2: The ray originates outside the box and passes through it.  Both
    //       entrance_distance and exit_distance are set accordingly.
    unsigned n_intersections;

    T entrance_distance, exit_distance;
};

// Compute the intersection between a ray and a box.
template<unsigned N, typename T>
ray_box_intersection<N,T>
intersection(ray<N,T> const& ray, box<N,T> const& box);

// Compute the intersection between a line segment and a box.
template<unsigned N, typename T>
optional<line_segment<N,T> >
intersection(line_segment<N,T> const& segment, box<N,T> const& box);

// Compute the intersection of two boxes.
template<unsigned N, typename T>
optional<box<N,T> >
intersection(box<N,T> const& a, box<N,T> const& b);

enum class segment_triangle_intersection_type
{
    NONE,
    FACE,
    EDGE,
    VERTEX,
    COPLANAR
};

// Compute whether or not a line and triangle intersect
template<typename T>
segment_triangle_intersection_type
is_intersecting(line_segment<3, T> const& segment, triangle<3, T> const& tri);

}

#include <cradle/geometry/intersection.ipp>

#endif
