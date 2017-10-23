#include <cradle/geometry/distance.hpp>
#include <limits>

namespace cradle {

template<typename T>
optional<vector<3,T> >
intersection(plane<T> const& plane, line_segment<3,T> const& segment)
{
    T d0 = distance(plane, segment[0]);
    T d1 = distance(plane, segment[1]);

    if (d0 < 0 && d1 >= 0 || d0 >= 0 && d1 < 0)
    {
        return optional<vector<3,T> >(
                segment[0] + as_vector(segment) * d0 / (d0 - d1));
    }
    else
        return optional<vector<3,T> >();
}

template<typename T>
optional<line_segment<3,T> >
intersection(plane<T> const& plane, triangle<3,T> const& tri)
{
    // Test the first two edges of the triangle for interesections.
    optional<vector<3,T> > i0 =
        intersection(plane, line_segment<3,T>(tri[0], tri[1]));
    optional<vector<3,T> > i1 =
        intersection(plane, line_segment<3,T>(tri[1], tri[2]));

    // If the triangle intersects the plane at all, it must intersect along
    // exactly two edges.
    if (i0)
    {
        if (i1)
            return line_segment<3,T>(i0.get(), i1.get());
        else
        {
            optional<vector<3,T> > i2 =
                intersection(plane, line_segment<3,T>(tri[2], tri[0]));
            assert(i2);
            return line_segment<3,T>(i2.get(), i0.get());
        }
    }
    else if (i1)
    {
        optional<vector<3,T> > i2 =
            intersection(plane, line_segment<3,T>(tri[2], tri[0]));
        assert(i2);
        return line_segment<3,T>(i1.get(), i2.get());
    }
    else
    {
        assert(!intersection(plane, line_segment<3,T>(tri[2], tri[0])));
        return optional<line_segment<3,T> >();
    }
}

#if 0

template<typename T>
std::vector<cradle::line_segment<2,double> >
intersection(plane<T> const& plane, triangle_mesh<T> const& mesh)
{
    std::vector<cradle::line_segment<2,double> > segments;
    for (unsigned i = 0; i < mesh.n_tris(); ++i)
    {
        auto segment = intersection(plane, mesh.triangle(i));
        if (segment)
            segments.push_back(get(segment));
    }
    return segments;
}

#endif

template<unsigned N, typename T>
ray_box_intersection<N,T>
intersection(ray<N,T> const& ray, box<N,T> const& box)
{
    T entrance = 0;
    T exit = std::numeric_limits<T>::infinity();

    // We process the intersections separately for each dimension.
    for (unsigned i = 0; i < N; ++i)
    {
        // If the ray's direction vector has a component of 0 in this
        // dimension, it can't intersect in this dimension, so it's a simple
        // containment test on the origin.
        if (almost_equal<T>(ray.direction[i], 0))
        {
            if (!is_inside(box, i, ray.origin[i]))
            {
                ray_box_intersection<N,T> result;
                result.n_intersections = 0;
                return result;
            }
            continue;
        }

        // Calculate the distance until the ray intersects the lower plane of
        // the box in this dimension.
        T t = (box.corner[i] - ray.origin[i]) / ray.direction[i];

        // If the ray originates outside the lower plane...
        if (ray.origin[i] < box.corner[i])
        {
            // If it points away from the wall, there's no intersection.
            if (t < 0)
            {
                ray_box_intersection<N,T> result;
                result.n_intersections = 0;
                return result;
            }
            // If it points towards the wall, update the entrance depth.
            else if (t > entrance)
                entrance = t;
        }
        // Otherwise, the ray originates inside the plane...
        else
        {
            // If the ray points towards the plane, update the exit depth.
            if (t > 0 && t < exit)
                exit = t;
        }

        // Now do all the same tests on the upper plane...
        t = (get_high_corner(box)[i] - ray.origin[i]) /
            ray.direction[i];
        if (ray.origin[i] > get_high_corner(box)[i])
        {
            if (t < 0)
            {
                ray_box_intersection<N,T> result;
                result.n_intersections = 0;
                return result;
            }
            else if (t > entrance)
                entrance = t;
        }
        else
        {
            if (t > 0 && t < exit)
                exit = t;
        }
    }

    // If we've determined that the ray exits the box before it enters it, then
    // it actually means that it passes outside the corner of the box, so
    // there's no intersection.
    if (exit < entrance)
    {
        ray_box_intersection<N,T> result;
        result.n_intersections = 0;
        return result;
    }

    ray_box_intersection<N,T> result;
    result.n_intersections = (entrance > 0) ? 2 : 1;
    result.entrance_distance = entrance;
    result.exit_distance = exit;
    return result;
}

template<unsigned N, typename T>
optional<line_segment<N,T> >
intersection(line_segment<N,T> const& segment, box<N,T> const& box)
{
    // Construct a ray from the line segment and let the ray-box intersection
    // code do most of the work.
    ray<N,T> ray(segment[0], unit(as_vector(segment)));
    auto rbi = intersection(ray, box);

    if (rbi.n_intersections == 0)
        return optional<line_segment<N,T> >();

    // The exit distance might actually be farther than the second point, so
    // we need to correct for that.
    T distance_to_second = std::min(length(segment), rbi.exit_distance);

    return line_segment<N,T>(
        ray.origin + ray.direction * rbi.entrance_distance,
        ray.origin + ray.direction * distance_to_second);
}

template<unsigned N, typename T>
optional<box<N,T> >
intersection(cradle::box<N,T> const& a, cradle::box<N,T> const& b)
{
    cradle::box<N,T> r;
    for (unsigned i = 0; i < N; ++i)
    {
        T low = (std::max)(a.corner[i], b.corner[i]);
        T high = (std::min)(a.corner[i] + a.size[i], b.corner[i] + b.size[i]);
        if (low >= high)
            return optional<box<N,T> >();
        r.corner[i] = low;
        r.size[i] = high - low;
    }
    return r;
}

template<typename T>
segment_triangle_intersection_type
is_intersecting(line_segment<3, T> const& segment, triangle<3, T> const& tri)
{
    // Compute edge vectors
    vector<3, T> e1 = tri[1] - tri[0];
    vector<3, T> e2 = tri[2] - tri[0];
    vector<3, T> p = cross(as_vector(segment), e2);
    T tmp = dot(p, e1);

    // Check angle for coplanarity
    if (-1.0e-12 < tmp && tmp < 1.0e-12) {
        return segment_triangle_intersection_type::COPLANAR;
    }

    // Check first barycentric coordinate
    tmp = 1.0 / tmp;
    vector<3, T> s = segment[0] - tri[0];
    T eu = tmp * dot(s, p);
    if (eu < -1.0e-10 || 1.0 + 1.0e-10 < eu) {
        return segment_triangle_intersection_type::NONE;
    }

    // Check second barycentric coordinate
    vector<3, T> q = cross(s, e1);
    T ev = tmp * dot(as_vector(segment), q);
    if (ev < -1.0e-10 || 1.0 + 1.0e-10 < ev || (eu + ev) > 1.0 + 1.0e-10) {
        return segment_triangle_intersection_type::NONE;
    }

    // Check intersection coordinate on segment
    T mu = tmp * dot(e2, q);
    if (mu < 0.0 || 1.0 < mu) {
        return segment_triangle_intersection_type::NONE;
    }

    // Ensure coordinates do not fall on an edge
    if (std::fabs(eu) < 1.0e-10 || std::fabs(ev) < 1.0e-10 || std::fabs(eu + ev - 1.0) < 1.0e-10 ) {
        return segment_triangle_intersection_type::EDGE;
    }

    // Passes all tests, must intersect the face interior
    return segment_triangle_intersection_type::FACE;
}

}
