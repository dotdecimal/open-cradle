#include <cradle/geometry/distance.hpp>

namespace cradle {

// 2D point <-> 2D polygon

double distance(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    double d = std::sqrt(absolute_distance2(poly, p, closest_point));
    return is_inside(poly, p) ? -d : d;
}

double distance2(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    double d2 = absolute_distance2(poly, p, closest_point);
    return is_inside(poly, p) ? -d2 : d2;
}

double absolute_distance2(polygon2 const& poly, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    bool have_min = false;
    double min_d2 = 0;
    vector<2,double> min_cp;

    for (polygon2_edge_view ev(poly); !ev.done(); ev.advance())
    {
        vector<2,double> cp;
        double d2 =
            distance2(line_segment<2,double>(ev.p0(), ev.p1()), p, &cp);
        if (!have_min || d2 < min_d2)
        {
            min_d2 = d2;
            min_cp = cp;
            have_min = true;
        }
    }
    assert(have_min);

    if (closest_point != NULL)
        *closest_point = min_cp;

    return min_d2;
}

// 2D point <-> 2D polygon set

double distance(polyset const& set, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    double d = std::sqrt(absolute_distance2(set, p, closest_point));
    return is_inside(set, p) ? -d : d;
}

double distance2(polyset const& set, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    double d2 = absolute_distance2(set, p, closest_point);
    return is_inside(set, p) ? -d2 : d2;
}

double absolute_distance2(polyset const& set, vector<2,double> const& p,
    vector<2,double>* closest_point)
{
    bool have_min = false;
    double min_d2 = 0;
    vector<2,double> min_cp;

    for (auto const& i : set.polygons)
    {
        vector<2,double> cp;
        double d2 = absolute_distance2(i, p, &cp);
        if (!have_min || d2 < min_d2)
        {
            min_d2 = d2;
            min_cp = cp;
            have_min = true;
        }
    }
    for (auto const& i : set.holes)
    {
        vector<2,double> cp;
        double d2 = absolute_distance2(i, p, &cp);
        if (!have_min || d2 < min_d2)
        {
            min_d2 = d2;
            min_cp = cp;
            have_min = true;
        }
    }
    assert(have_min);

    if (closest_point != NULL)
        *closest_point = min_cp;

    return min_d2;
}

}
