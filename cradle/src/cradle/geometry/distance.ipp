namespace cradle {

// 2D point <-> 2D line segment

template<typename T>
T distance(line_segment<2,T> const& segment,
    vector<2,T> const& p, vector<2,T>* closest_point)
{
    return std::sqrt(distance2(segment, p, closest_point));
}

template<typename T>
T distance2(line_segment<2,T> const& segment,
    vector<2,T> const& p, vector<2,T>* closest_point)
{
    vector<2,T> v = as_vector(segment);
    vector<2,T> n = v / length2(v);

    vector<2,T> c;
    T t = dot(p - segment[0], n);
    if (t < 0)
        c = segment[0];
    else if (t > 1)
        c = segment[1];
    else
        c = segment[0] + t * v;

    if (closest_point != NULL)
        *closest_point = c;

    return length2(c - p);
}

// 3D point <-> plane

template<typename T>
T distance(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point)
{
    T distance = dot(p - plane.point, plane.normal);
    if (closest_point != NULL)
        *closest_point = p - plane.normal * distance;
    return distance;
}

template<typename T>
T distance2(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point)
{
    T d = distance(plane, p, closest_point);
    return (d < 0) ? -d * d : d * d;
}

template<typename T>
T absolute_distance2(plane<T> const& plane, vector<3,T> const& p,
    vector<3,T>* closest_point)
{
    T d = distance(plane, p, closest_point);
    return d * d;
}

}
