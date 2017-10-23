#ifndef CRADLE_GEOMETRY_TRANSFORMATIONS_HPP
#define CRADLE_GEOMETRY_TRANSFORMATIONS_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/angle.hpp>

namespace cradle {

// A linear transformation for N-dimensional space is a represented as an
// (N+1)x(N+1) matrix where the bottom row of the matrix is (0 ... 0 1).
// This module provides several methods for creating and applying linear
// transformations of this form.
// SCALING

// Create a scaling transformation matrix.
api(fun with(N:2,3; T:double) name(make_scaling_matrix))
template<unsigned N, typename T>
matrix<N + 1,N + 1,T> scaling_transformation(vector<N,T> const& scale)
{
    matrix<N + 1,N + 1,T> m;
    typename matrix<N + 1,N + 1,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N + 1; ++i)
        for (unsigned j = 0; j < N + 1; ++j, ++p)
            *p = (i == j) ? ((i < N) ? scale[i] : 1) : 0;
    return m;
}

// TRANSLATION

// Create a translation matrix.
api(fun with(N:2,3; T:double) name(make_translation_matrix))
template<unsigned N, typename T>
matrix<N + 1,N + 1,T> translation(vector<N,T> const& v)
{
    matrix<N + 1,N + 1,T> m;
    typename matrix<N + 1,N + 1,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N + 1; ++i)
        for (unsigned j = 0; j < N + 1; ++j, ++p)
            *p = (i == j) ? 1 : ((j == N) ? v[i] : 0);
    return m;
}

// ROTATION

// Create a 2D CCW rotation matrix.
template<typename T, class AngleUnits>
matrix<3,3,T> rotation(angle<T,AngleUnits> a)
{
    T c = cos(a), s = sin(a);
    return make_matrix<T>(
        c,-s, 0,
        s, c, 0,
        0, 0, 1);
}

// Create a 2D CCW rotation matrix.
// The angle is specified in degrees.
api(fun)
matrix<3,3,double>
make_2d_rotation_matrix(double angle)
{ return rotation(cradle::angle<double,degrees>(angle)); }

// Generate a 3D rotation about the X-axis.
template<typename T, class AngleUnits>
matrix<4,4,T> rotation_about_x(angle<T,AngleUnits> a)
{
    T c = cos(a), s = sin(a);
    return make_matrix<T>(
        1, 0, 0, 0,
        0, c,-s, 0,
        0, s, c, 0,
        0, 0, 0, 1);
}

// Generate a 3D rotation about the Y-axis.
template<typename T, class AngleUnits>
matrix<4,4,T> rotation_about_y(angle<T,AngleUnits> a)
{
    T c = cos(a), s = sin(a);
    return make_matrix<T>(
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1);
}

// Generate a 3D rotation about the Z-axis.
template<typename T, class AngleUnits>
matrix<4,4,T> rotation_about_z(angle<T,AngleUnits> a)
{
    T c = cos(a), s = sin(a);
    return make_matrix<T>(
        c,-s, 0, 0,
        s, c, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}

// Generate a 3D rotation about an arbitrary axis.
template<typename T, class AngleUnits>
matrix<4,4,T> rotation_about_axis(vector<3,T> const& axis,
    angle<T,AngleUnits> a)
{
    T const c = cos(a);
    T const s = sin(a);
    T const omc = static_cast<T>(1.0) - c;

    T const x = axis[0];
    T const y = axis[1];
    T const z = axis[2];

    return make_matrix<T>(
        x * x * omc + c,     x * y * omc - z * s, x * z * omc + y * s, 0,
        y * x * omc + z * s, y * y * omc + c,     y * z * omc - x * s, 0,
        z * x * omc - y * s, z * y * omc + x * s, z * z * omc + c,     0,
        0,                   0,                   0,                   1);
}

// Create a 3D rotation matrix.
// The rotation will be CCW if viewed with the axis pointing towards you.
// The angle is specified in degrees.
api(fun)
matrix<4,4,double>
make_3d_rotation_matrix(vector<3,double> const& axis, double angle)
{ return rotation_about_axis(axis, cradle::angle<double,degrees>(angle)); }

// Create an inverse transformation for the supplied matrix.
template<typename T>
matrix<4,4,T> inverse_transformation(matrix<4,4,T> const& m)
{
        //matrix<3,3,T> rt = make_matrix<T>(        m(0,0), m(1,0), m(2,0),
        //                                                                        m(0,1), m(1,1), m(2,1),
        //                                                                        m(0,2), m(1,2), m(2,2));
        vector<3,T> rtt = make_vector(        m(0,0) * m(0,3) + m(1,0) * m(1,3) + m(2,0) * m(2,3),
                                                                        m(0,1) * m(0,3) + m(1,1) * m(1,3) + m(2,1) * m(2,3),
                                                                        m(0,2) * m(0,3) + m(1,2) * m(1,3) + m(2,2) * m(2,3));
    return make_matrix<T>( m(0,0), m(1,0), m(2,0), -rtt[0],
                                                   m(0,1), m(1,1), m(2,1), -rtt[1],
                                                   m(0,2), m(1,2), m(2,2), -rtt[2],
                                                   0, 0, 0, 1);
}

// APPLYING TRANSFORMATIONS

// Transform a point by a matrix and return the result.
// 3D case
api(fun with(T:double))
template<typename T>
vector<3,T> transform_point(matrix<4,4,T> const& m, vector<3,T> const& p)
{
    return make_vector<T>(
        p[0] * m(0,0) + p[1] * m(0,1) + p[2] * m(0,2) + m(0,3),
        p[0] * m(1,0) + p[1] * m(1,1) + p[2] * m(1,2) + m(1,3),
        p[0] * m(2,0) + p[1] * m(2,1) + p[2] * m(2,2) + m(2,3));
}
// Transform a point by a matrix and return the result.
// 3D case with w, to support using this with inverted perspective projections
template<typename T>
vector<4,T> transform_point(matrix<4,4,T> const& m, vector<4,T> const& p)
{
    return make_vector<T>(
        p[0] * m(0,0) + p[1] * m(0,1) + p[2] * m(0,2) + p[3] * m(0,3),
        p[0] * m(1,0) + p[1] * m(1,1) + p[2] * m(1,2) + p[3] * m(1,3),
        p[0] * m(2,0) + p[1] * m(2,1) + p[2] * m(2,2) + p[3] * m(2,3),
        p[0] * m(3,0) + p[1] * m(3,1) + p[2] * m(3,2) + p[3] * m(3,3));
}
// 2D case
template<typename T>
vector<2,T> transform_point(matrix<3,3,T> const& m, vector<2,T> const& p)
{
    assert(
        almost_equal<T>(m(2,0), 0) &&
        almost_equal<T>(m(2,1), 0) &&
        almost_equal<T>(m(2,2), 1));
    return make_vector<T>(
        p[0] * m(0,0) + p[1] * m(0,1) + m(0,2),
        p[0] * m(1,0) + p[1] * m(1,1) + m(1,2));
}
// 1D case
template<typename T>
vector<1,T> transform_point(matrix<2,2,T> const& m, vector<1,T> const& p)
{
    assert(
        almost_equal<T>(m(1,0), 0) &&
        almost_equal<T>(m(1,1), 1));
    return make_vector<T>(p[0] * m(0,0) + m(0,1));
}

// Transform a vector by a matrix (rotation portion only) and return the result.
// 3D case
template<typename T>
vector<3,T> transform_vector(matrix<4,4,T> const& m, vector<3,T> const& v)
{
    return make_vector<T>(
        v[0] * m(0,0) + v[1] * m(0,1) + v[2] * m(0,2),
        v[0] * m(1,0) + v[1] * m(1,1) + v[2] * m(1,2),
        v[0] * m(2,0) + v[1] * m(2,1) + v[2] * m(2,2));
}
// Transform a vector by a matrix (rotation portion only) and return the result.
// 2D case
template<typename T>
vector<2,T> transform_vector(matrix<3,3,T> const& m, vector<2,T> const& v)
{
    assert(
        almost_equal<T>(m(2,0), 0) &&
        almost_equal<T>(m(2,1), 0) &&
        almost_equal<T>(m(2,2), 1));
    return make_vector<T>(
        v[0] * m(0,0) + v[1] * m(0,1),
        v[0] * m(1,0) + v[1] * m(1,1));
}
// Transform a vector by a matrix (rotation portion only) and return the result.
// 1D case
template<typename T>
vector<1,T> transform_vector(matrix<2,2,T> const& m, vector<1,T> const& v)
{
    assert(
        almost_equal<T>(m(1,0), 0) &&
        almost_equal<T>(m(1,1), 1));
    return make_vector<T>(v[0] * m(0,0));
}

template<typename T>
plane<double> transform_plane(matrix<4,4,T> const& m, plane<T> const& p)
{
    return plane<double>(transform_point(m, p.point), transform_vector(m, p.normal));
}

// Transform a box by a matrix and return the result.
// 3D case
template<typename T>
box<3,T> transform_box(matrix<4,4,T> const&m, box<3,T> const& b)
{
    std::vector<vector3d> corners(8);
    corners[0] = b.corner;
    corners[1] = b.corner + make_vector<double>(b.size[0],         0,         0);
    corners[2] = b.corner + make_vector<double>(b.size[0], b.size[1],         0);
    corners[3] = b.corner + make_vector<double>(        0, b.size[1],         0);
    corners[4] = b.corner + make_vector<double>(        0,         0, b.size[2]);
    corners[5] = b.corner + make_vector<double>(b.size[0],         0, b.size[2]);
    corners[6] = b.corner + make_vector<double>(b.size[0], b.size[1], b.size[2]);
    corners[7] = b.corner + make_vector<double>(        0, b.size[1], b.size[2]);
    for (unsigned i = 0; i != 8; ++i)
    {
        corners[i] = transform_point(m, corners[i]);
    }
    return bounding_box(corners);
}

}

#endif
