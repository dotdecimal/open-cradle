#ifndef CRADLE_GEOMETRY_DISTANCE_HPP
#define CRADLE_GEOMETRY_DISTANCE_HPP

// This file provides several functions for computing the distance from one
// geometric object to another.  All distance() functions accept two required
// arguments - the objects in question.
//
// Additionally, for those distance() functions that compute the distance from
// a point to a more complex object, an extra optional argument is accepted, a
// pointer to a point type.  If supplied, the algorithm also computes the point
// on the more complex object that is closest to the point in question and
// writes it back to the supplied pointer.  For algorithms that compare two
// complex (non-point) objects, two optional arguments are accepted, one to
// store the closest point on each object.
//
// Note that for objects that have an inside and an outside (polygons, cubes,
// etc), a point inside the object is defined as being a negative distance away
// from the object, and the value returned by distance() will reflect this.
// (The absolute value will still be the actual distance to the nearest edge.)
//
// Every distance() function has an equivalent distance2() function which
// returns the distance squared.  When you only care about comparing distances,
// it is generally faster to use distance2()...
// Note that in the case of negative distances, the square is also negative.
//
// For distance2() functions that can return negative values, there are also
// versions that always return positive results.  This is generally the fastest
// version of all the distance functions (unless otherwise noted).
//

#include <cradle/geometry/polygonal.hpp>

namespace cradle {

// Originally, these were implemented by giving the closest_point parameter a
// default value of NULL. However, that causes strange errors on Visual C++
// 9.0, so until that's fixed, each function has two overloads.

// 2D point <-> 2D line segment
template<typename T>
T distance(line_segment<2,T> const& segment, vector<2,T> const& p,
    vector<2,T>* closest_point);
template<typename T>
T distance(line_segment<2,T> const& segment, vector<2,T> const& p)
{ return distance(segment, p, (vector<2,T>*)0); }
//
template<typename T>
T distance2(line_segment<2,T> const& segment, vector<2,T> const& p,
    vector<2,T>* closest_point);
template<typename T>
T distance2(line_segment<2,T> const& segment, vector<2,T> const& p)
{ return distance2(segment, p, (vector<2,T>*)0); }

// 2D point <-> 2D polygon
double distance(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double distance(polygon2 const& poly, vector<2,double> const& p)
{ return distance(poly, p, (vector<2,double>*)0); }
//
double distance2(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double distance2(polygon2 const& poly, vector<2,double> const& p)
{ return distance2(poly, p, (vector<2,double>*)0); }
//
double absolute_distance2(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double absolute_distance2(
    polygon2 const& poly, vector<2,double> const& p)
{ return absolute_distance2(poly, p, (vector<2,double>*)0); }

// 2D point <-> 2D polyset
double distance(polyset const& area, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double distance(polyset const& area, vector<2,double> const& p)
{ return distance(area, p, (vector<2,double>*)0); }
//
double distance2(polyset const& area, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double distance2(polyset const& area, vector<2,double> const& p)
{ return distance2(area, p, (vector<2,double>*)0); }
//
double absolute_distance2(polyset const& area, vector<2,double> const& p,
    vector<2,double>* closest_point);
static inline double absolute_distance2(
    polyset const& area, vector<2,double> const& p)
{ return absolute_distance2(area, p, (vector<2,double>*)0); }

// 3D point <-> plane
// NOTE: The returned distance is positive if the point is on the side of the
// plane to which the normal points and negative if on the other side.
// NOTE: In this case, distance() is fastest. The other versions are computed
// using distance(), and are only really provided for consistency's sake.
template<typename T>
T distance(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point);
template<typename T>
T distance(plane<T> const& plane, vector<3,T> const& p)
{ return distance(plane, p, (vector<3,T>*)0); }
//
template<typename T>
T distance2(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point);
template<typename T>
T distance2(plane<T> const& plane, vector<3,T> const& p)
{ return distance2(plane, p, (vector<3,T>*)0); }
//
template<typename T>
T absolute_distance2(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point);
template<typename T>
T absolute_distance2(plane<T> const& plane, vector<3,T> const& p)
{ return absolute_distance2(plane, p, (vector<3,T>*)0); }

}

#include <cradle/geometry/distance.ipp>

#endif
