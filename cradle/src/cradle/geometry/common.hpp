#ifndef CRADLE_GEOMETRY_COMMON_HPP
#define CRADLE_GEOMETRY_COMMON_HPP

#include <alia/geometry.hpp>
#include <cradle/math/common.hpp>
#include <cradle/geometry/forward.hpp>

// CRADLE uses the same geometric primitives as alia.
// Note that most utility functions are defined in the alia namespace so that
// they work properly with ADL.

// VECTOR

namespace cradle {

using alia::uniform_vector;
using alia::make_vector;

// 1D constructor
template<class T>
vector<1,T> make_vector(T x)
{
    vector<1,T> v;
    v[0] = x;
    return v;
}
// 4D constructor
template<class T>
vector<4,T> make_vector(T x, T y, T z, T w)
{
    vector<4,T> v;
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
    return v;
}

}

namespace alia {

// Given a vector, this returns a corresponding vector with one less dimension
// by removing the value at index i.
template<unsigned N, class T>
vector<N - 1,T> slice(vector<N,T> const& p, unsigned i)
{
    assert(i < N);
    vector<N - 1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    for(unsigned j = i; j < N - 1; ++j)
        r[j] = p[j + 1];
    return r;
}

// Given a vector, this returns a corresponding vector with one more dimension
// by inserting the given value at index i.
template<unsigned N, class T, class OtherValue>
vector<N+1,T> unslice(vector<N,T> const& p, unsigned i, OtherValue value)
{
    assert(i <= N);
    vector<N+1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    r[i] = static_cast<T>(value);
    for(unsigned j = i; j < N; ++j)
        r[j + 1] = p[j];
    return r;
}

// 3D cross product of two vectors
template<class T>
vector<3,T> cross(vector<3,T> const& a, vector<3,T> const& b)
{
    return make_vector<T>(
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]);
}

// 2D cross product of two vectors
template<class T>
T cross(vector<2,T> const& a, vector<2,T> const& b)
{
    return a[0] * b[1] - a[1] * b[0];
}

// Get a unit vector that's perpendicular to the given one.
template<class T>
vector<3,T> get_perpendicular(vector<3,T> const& v)
{
    vector<3,T> u;
    if ((std::abs)(v[0]) < (std::abs)(v[1]))
    {
        if ((std::abs)(v[0]) < (std::abs)(v[2]))
            u = make_vector<T>(1, 0, 0);
        else
            u = make_vector<T>(0, 0, 1);
    }
    else
    {
        if ((std::abs)(v[1]) < (std::abs)(v[2]))
            u = make_vector<T>(0, 1, 0);
        else
            u = make_vector<T>(0, 0, 1);
    }
    return unit(cross(u, v));
}

template<unsigned N, typename T>
T product(vector<N,T> const& v)
{
    T r = 1;
    for (unsigned i = 0; i != N; ++i)
        r *= v[i];
    return r;
}

// Determine if the two vectors are almost equal.
template<unsigned N, class Value>
bool almost_equal(vector<N,Value> const& a, vector<N,Value> const& b)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (!cradle::almost_equal<Value>(a[i], b[i]))
            return false;
    }
    return true;
}
template<unsigned N, class Value, class Tolerance>
bool almost_equal(vector<N,Value> const& a, vector<N,Value> const& b,
    Tolerance tolerance)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (!cradle::almost_equal(a[i], b[i],
            static_cast<Value>(tolerance)))
        {
            return false;
        }
    }
    return true;
}

}

// BOX

namespace cradle {

// Implement the CRADLE regular type interface for boxes
api(struct preexisting(comparisons,iostream) namespace(alia)
    with(N:1,2,3,4;T:double))
template<unsigned N, class T>
struct box
{
    // Vector of N dimensions, with N being 1 thru 4, that defines the lower left corner
    vector<N,T> corner;
    // Vector of N dimensions, with N being 1 thru 4, that defines the length, width, etc
    vector<N,T> size;
};

}

namespace alia {

// Get the area of a box.
template<typename T>
T area(box<2,T> const& box) { return box.size[0] * box.size[1]; }

// Clamp the given point such that it lies within the given box.
template<unsigned N, typename T>
vector<N,T> clamp(vector<N,T> const& p, box<N,T> const& box)
{
    vector<N,T> result;
    for (unsigned i = 0; i < N; i++)
    {
        result[i] = cradle::clamp(p[i], box.corner[i], box.corner[i] +
            box.size[i]);
    }
    return result;
}

// check to see if the given point lies within the given box
template<unsigned N, typename T>
bool contains(box<N,T> const& box, vector<N,T> const& p)
{
    for (unsigned i=0; i<N; ++i)
    {
        if ( (p[i] < box.corner[i]) ||
             (box.corner[i] + box.size[i] < p[i]) )
        {
            return false;
        }
    }
    return true;
}

// Slice an axis off the given box.
template<unsigned N, typename T>
box<N-1,T> slice(box<N,T> const& box, unsigned axis)
{
    assert(axis < N);
    return alia::box<N-1,T>(slice(box.corner, axis),
        slice(box.size, axis));
}

template<unsigned N, typename T>
box<N+1,T> unslice(box<N,T> const& b, unsigned axis, box<1,T> const& slice)
{
    assert(axis < N + 1);
    return alia::box<N+1,T>(unslice(b.corner, axis, slice.corner[0]),
        unslice(b.size, axis, slice.size[0]));
}

template<unsigned N, typename T>
box<N,T> scale(box<N,T> const& box, double factor)
{
    for (unsigned i = 0; i != N; ++i)
    {
        box.corner[i] *= factor;
        box.size[i] *= factor;
    }
}

template<unsigned N, typename T>
box<N,T> bounding_box(box<N,T> const& b)
{ return b; }

template<unsigned N, typename T>
void compute_bounding_box(
    optional<box<N,T> >& box,
    alia::box<N,T> const& b)
{
    if (box)
    {
        for (unsigned i = 0; i < N; ++i)
        {
            if (b.corner[i] < box.get().corner[i])
            {
                box.get().corner[i] = b.corner[i];
            }
            if (get_high_corner(b)[i] > get_high_corner(box.get())[i])
            {
                box.get().size[i] = get_high_corner(b)[i] -
                    box.get().corner[i];
            }
        }
    }
    else
        box = b;
}

}

// MATRIX

namespace cradle {

using alia::matrix;
using alia::identity_matrix;
using alia::make_matrix;

typedef matrix<3,3,double> transformation_matrix_2d;
typedef matrix<4,4,double> transformation_matrix_3d;

}

namespace alia {

template<unsigned M, unsigned N, class T>
cradle::raw_type_info get_type_info(matrix<M,N,T>)
{
    using namespace cradle;
    using cradle::get_type_info;
    return
        raw_type_info(raw_kind::ARRAY, any(raw_array_info(M,
            raw_type_info(raw_kind::ARRAY, any(raw_array_info(N,
                get_type_info(T())))))));
}
template<unsigned M, unsigned N, class T>
size_t deep_sizeof(matrix<M,N,T>)
{
    using cradle::deep_sizeof;
    // We assume that T has a static deep_sizeof.
    return M * N * deep_sizeof(T());
}
template<unsigned M, unsigned N, class T>
void from_value(matrix<M,N,T>* x, cradle::value const& v)
{
    using cradle::from_value;
    cradle::value_list const& l = cradle::cast<cradle::value_list>(v);
    cradle::check_array_size(M, l.size());
    for (unsigned i = 0; i != M; ++i)
    {
        cradle::value_list const& row = cradle::cast<cradle::value_list>(l[i]);
        cradle::check_array_size(N, row.size());
        for (unsigned j = 0; j != N; ++j)
            from_value(&(*x)(i, j), row[j]);
    }
}
template<unsigned M, unsigned N, class T>
void to_value(cradle::value* v, matrix<N,M,T> const& x)
{
    using cradle::to_value;
    cradle::value_list l(M);
    for (unsigned i = 0; i != M; ++i)
    {
        cradle::value_list row(N);
        for (unsigned j = 0; j != N; ++j)
            to_value(&row[j], x(i, j));
        l[i].swap_in(row);
    }
    v->swap_in(l);
}

// Get the inverse of the given 4x4 matrix.
template<typename T>
matrix<4,4,T> inverse(matrix<4,4,T> const& m)
{
    T fa0 = m(0,0) * m(1,1) - m(0,1) * m(1,0);
    T fa1 = m(0,0) * m(1,2) - m(0,2) * m(1,0);
    T fa2 = m(0,0) * m(1,3) - m(0,3) * m(1,0);
    T fa3 = m(0,1) * m(1,2) - m(0,2) * m(1,1);
    T fa4 = m(0,1) * m(1,3) - m(0,3) * m(1,1);
    T fa5 = m(0,2) * m(1,3) - m(0,3) * m(1,2);
    T fb0 = m(2,0) * m(3,1) - m(2,1) * m(3,0);
    T fb1 = m(2,0) * m(3,2) - m(2,2) * m(3,0);
    T fb2 = m(2,0) * m(3,3) - m(2,3) * m(3,0);
    T fb3 = m(2,1) * m(2,2) - m(2,2) * m(2,1);
    T fb4 = m(2,1) * m(3,3) - m(2,3) * m(3,1);
    T fb5 = m(2,2) * m(3,3) - m(2,3) * m(3,2);

    T det = fa0 * fb5 - fa1 * fb4 + fa2 * fb3 + fa3 * fb2 - fa4 * fb1 +
        fa5 * fb0;
    if (det == 0)
        return make_matrix<T>(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    T inv_det = static_cast<T>(1.0) / det;

    matrix<4,4,T> inv = make_matrix<T>(
        + m(1,1) * fb5 - m(1,2) * fb4 + m(1,3) * fb3,
        - m(0,1) * fb5 + m(0,2) * fb4 - m(0,3) * fb3,
        + m(3,1) * fa5 - m(3,2) * fa4 + m(3,3) * fa3,
        - m(2,1) * fa5 + m(2,2) * fa4 - m(2,3) * fa3,
        - m(1,0) * fb5 + m(1,2) * fb2 - m(1,3) * fb1,
        + m(0,0) * fb5 - m(0,2) * fb2 + m(0,3) * fb1,
        - m(3,0) * fa5 + m(3,2) * fa2 - m(3,3) * fa1,
        + m(2,0) * fa5 - m(2,2) * fa2 + m(2,3) * fa1,
        + m(1,0) * fb4 - m(1,1) * fb2 + m(1,3) * fb0,
        - m(0,0) * fb4 + m(0,1) * fb2 - m(0,3) * fb0,
        + m(3,0) * fa4 - m(3,1) * fa2 + m(3,3) * fa0,
        - m(2,0) * fa4 + m(2,1) * fa2 - m(2,3) * fa0,
        - m(1,0) * fb3 + m(1,1) * fb1 - m(1,2) * fb0,
        + m(0,0) * fb3 - m(0,1) * fb1 + m(0,2) * fb0,
        - m(3,0) * fa3 + m(3,1) * fa1 - m(3,2) * fa0,
        + m(2,0) * fa3 - m(2,1) * fa1 + m(2,2) * fa0);
    inv *= inv_det;
    return inv;
}

template<unsigned N, typename T>
matrix<N-1,N-1,T> slice(matrix<N,N,T> const& m, unsigned axis)
{
    matrix<N-1,N-1,T> r;
    for (unsigned i = 0; i < N; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
            r(i, j) = m(i >= axis ? i + 1 : i, j >= axis ? j + 1 : j);
    }
}

}

// OTHER STUFF - Stuff that's not in alia.

namespace cradle {

// a circle
api(struct with(T:double))
template<typename T>
struct circle
{
    vector<2,T> center;
    T radius;
};

template<typename T>
T area(circle<T> const& circle)
{
    return pi * circle.radius * circle.radius;
}

template<typename T>
bool is_inside(circle<T> const& circle, vector<2,T> const& p)
{
    return length(p - circle.center) <= circle.radius;
}

// a plane in 3D place
api(struct with(T:double))
template<typename T>
struct plane
{
    vector<3,T> point;
    vector<3,T> normal;
};

// A ray is the combination of an origin point and a direction vector.
// The direction vector should be a unit vector.
/// N is the number of dimensions of the space in which the ray exists.
/// T is the type of the coordinates.
api(struct with(N:2,3;T:double))
template<unsigned N, typename T>
struct ray
{
    vector<N,T> origin;
    vector<N,T> direction;
};

CRADLE_GEOMETRY_DEFINE_FLOATING_TYPEDEFS(ray)

template<unsigned N, typename T>
struct line_segment : c_array<2,vector<N,T> >
{
    line_segment() {}
    line_segment(vector<N,T> const& v0, vector<N,T> const& v1)
    {
        (*this)[0] = v0;
        (*this)[1] = v1;
    }
};

// hash function
} namespace std {
    template<unsigned N, class T>
    struct hash<cradle::line_segment<N,T> >
    {
        size_t operator()(cradle::line_segment<N,T> const& x) const
        {
            return alia::combine_hashes(
                alia::invoke_hash(x[0]), alia::invoke_hash(x[1]));
        }
    };
} namespace cradle {

template<unsigned N, typename T>
line_segment<N,T>
make_line_segment(vector<N,T> const& v0, vector<N,T> const& v1)
{ return line_segment<N,T>(v0, v1); }

// Get a vector from the first point on the given line segment to the second.
template<unsigned N, typename T>
vector<N,T> as_vector(line_segment<N,T> const& segment)
{
    return segment[1] - segment[0];
}

// Get a point along the given line segment u[0,1].
template<unsigned N, typename T>
vector<N,T> point_along(line_segment<N,T> const& segment, double u)
{
    return segment[0] + (u * (segment[1] - segment[0]));
}
// Get a point along the given line segment u[0,1].
template<unsigned N, typename T>
vector<N,T> point_along(vector<N,T> const& s0, vector<N,T> const& s1, double u)
{
    return s0 + (u * (s1 - s0));
}

// Calculate the length of the line segment.
template<unsigned N, typename T>
T length(line_segment<N,T> const& segment)
{
    return length(as_vector(segment));
}

template<unsigned N, typename T>
struct triangle : c_array<3,vector<N,T> >
{
    typedef vector<N,T> vertex_type;

    triangle() {}

    triangle(
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
    struct hash<cradle::triangle<N,T> >
    {
        size_t operator()(cradle::triangle<N,T> const& x) const
        {
            return alia::combine_hashes(
                alia::invoke_hash(x[0]),
                alia::combine_hashes(
                    alia::invoke_hash(x[1]),
                    alia::invoke_hash(x[2])));
        }
    };
} namespace cradle {

template<typename T>
vector<3,T> get_normal(triangle<3,T> const& tri)
{ return unit(cross(tri[1] - tri[0], tri[2] - tri[0])); }

template<typename T>
bool is_ccw(triangle<2,T> const& tri)
{
    vector<2,T> edge0 = tri[1] - tri[0];
    vector<2,T> edge1 = tri[2] - tri[1];
    return (edge0[0] * edge1[1]) - (edge0[1] * edge1[0]) > 0;
}

template<typename T>
T get_area(triangle<2,T> const& tri)
{
    return std::fabs(cross(tri[1]-tri[0], tri[2]-tri[0])) * 0.5;
}

template<typename T>
T get_area(triangle<3,T> const& tri)
{
    return length(cross(tri[1]-tri[0], tri[2]-tri[0])) * 0.5;
}

template<unsigned N, typename T>
box<N,T> bounding_box(triangle<N,T> const& tri)
{
        vector<N,T> min = tri[0];
        vector<N,T> max = tri[0];
        for (unsigned j = 0; j < N; ++j)
    {
                if (tri[1][j] < min[j]) { min[j] = tri[1][j]; }
                if (tri[2][j] < min[j]) { min[j] = tri[2][j]; }
                if (tri[1][j] > max[j]) { max[j] = tri[1][j]; }
                if (tri[2][j] > max[j]) { max[j] = tri[2][j]; }
    }
    return cradle::box<N,T>(min, max - min);
}

}

// API functions

namespace cradle {

// Expand or contract a box by adding a uniform margin around the edge.
api(fun trivial with(N:1,2,3,4))
template<unsigned N>
box<N,double>
add_margin_to_box(box<N,double> const& box, vector<N,double> const& size)
{ return add_border(box, size); }

// Compute the inverse of a matrix.
api(fun with(N:2,3,4))
template<unsigned N>
// The matrix representing the inverse of the provided matrix
matrix<N,N,double>
matrix_inverse(
    // The matrix that is to inversed
    matrix<N,N,double> const& m)
{ return inverse(m); }

// Compute the product of two matrices.
api(fun with(N:1,2,3,4))
template<unsigned N>
// The matrix equivalent to the product of the provided matrices.
matrix<N,N,double>
matrix_product(
    // The provided matrix that is to be multiplied by the second
    matrix<N,N,double> const& a,
    // The provided matrix that is to be multiplied by the first
    matrix<N,N,double> const& b)
{ return a * b; }

template<unsigned N, typename T>
void compute_bounding_box(
    optional<box<N, T> >& box,
    std::vector<vector<N,T> > const& points)
{
    typename std::vector<vector<N,T> >::const_iterator
        i = points.begin(), end = points.end();
    if (i == end)
        return;

    vector<N,T> min, max;
    if (box)
    {
        min = get(box).corner;
        max = get_high_corner(box.get());
    }
    else
    {
        min = *i;
        max = *i;
        ++i;
    }

    for (; i != end; ++i)
    {
        for (unsigned j = 0; j < N; ++j)
        {
            if ((*i)[j] < min[j])
                min[j] = (*i)[j];
            if ((*i)[j] > max[j])
                max[j] = (*i)[j];
        }
    }

    box = cradle::box<N,T>(min, max - min);
}

// Compute the bounding box of a list of points.
api(fun with(N:1,2,3,4;T:double) name(point_list_bounding_box))
template<unsigned N, typename T>
// Bounding box size N of the given point list
box<N,T> bounding_box(
    // List of point vectors in which to compute bounding box
    std::vector<vector<N,T> > const& points)
{
    optional<box<N,T> > box;
    compute_bounding_box(box, points);
    return box ? get(box) :
        cradle::box<N,T>(uniform_vector<N,T>(0), uniform_vector<N,T>(0));
}

}

#endif
