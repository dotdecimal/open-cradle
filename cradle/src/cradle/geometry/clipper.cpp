#include <cradle/geometry/clipper.hpp>

namespace cradle {

clipper_point to_clipper(vector<2,double> const& p)
{
    return clipper_point(
        ClipperLib::long64(p[0] / clipper_integer_precision),
        ClipperLib::long64(p[1] / clipper_integer_precision));
}

vector<2,double> from_clipper(clipper_point const& p)
{
    return make_vector<double>(
        p.X * clipper_integer_precision,
        p.Y * clipper_integer_precision);
}

ClipperLib::Polygon to_clipper(polygon2 const& poly)
{
    ClipperLib::Polygon cp;
    to_clipper(&cp, poly);
    return cp;
}

void to_clipper(ClipperLib::Polygon* cp, polygon2 const& poly)
{
    size_t n_points = poly.vertices.size();
    cp->resize(n_points);
    for (size_t i = 0; i != n_points; ++i)
        (*cp)[i] = to_clipper(poly.vertices[i]);
}

polygon2 from_clipper(ClipperLib::Polygon const& cp)
{
    polygon2 poly;
    from_clipper(&poly, cp);
    return poly;
}

void from_clipper(polygon2* poly, ClipperLib::Polygon const& cp)
{
    size_t n_points = cp.size();
    vertex2* vertices = allocate(&poly->vertices, n_points);
    for (size_t i = 0; i != n_points; ++i)
        vertices[i] = from_clipper(cp[i]);
}

ClipperLib::Polygons to_clipper(polyset const& cradle_set)
{
    ClipperLib::Polygons clipper_set;
    to_clipper(&clipper_set, cradle_set);
    return clipper_set;
}

void to_clipper(ClipperLib::Polygons* clipper_set, polyset const& cradle_set)
{
    clipper_set->clear();
    for (auto const& i : cradle_set.polygons)
    {
        ClipperLib::Polygon cp = to_clipper(i);
        if (!Orientation(cp))
            ReversePath(cp);
        clipper_set->push_back(cp);
    }
    for (auto const& i : cradle_set.holes)
    {
        ClipperLib::Polygon cp = to_clipper(i);
        if (Orientation(cp))
            ReversePath(cp);
        clipper_set->push_back(cp);
    }
}

polyset from_clipper(ClipperLib::Polygons const& clipper_set)
{
    polyset cradle_set;
    from_clipper(&cradle_set, clipper_set);
    return cradle_set;
}

void from_clipper(polyset* cradle_set, ClipperLib::Polygons const& clipper_set)
{
    cradle_set->polygons.clear();
    cradle_set->holes.clear();

    for (auto const& i : clipper_set)
    {
        auto poly = from_clipper(i);
        if (Orientation(i))
            cradle_set->polygons.push_back(poly);
        else
            cradle_set->holes.push_back(poly);
    }
}

raw_type_info get_type_info(clipper_polyset)
{
    return get_type_info(polygon2());
}

bool operator==(clipper_polyset const& a, clipper_polyset const& b)
{
    return from_clipper(a) == from_clipper(b);
}
bool operator!=(clipper_polyset const& a, clipper_polyset const& b)
{
    return from_clipper(a) != from_clipper(b);
}
bool operator<(clipper_polyset const& a, clipper_polyset const& b)
{
    return from_clipper(a) < from_clipper(b);
}

void to_value(value* v, clipper_poly const& x)
{
    to_value(v, from_clipper(x));
}
void from_value(clipper_poly* x, value const& v)
{
    polygon2 p;
    from_value(&p, v);
    to_clipper(x, p);
}
void to_value(value* v, clipper_polyset const& x)
{
    to_value(v, from_clipper(x));
}
void from_value(clipper_polyset* x, value const& v)
{
    polyset p;
    from_value(&p, v);
    to_clipper(x, p);
}

double static
get_area(clipper_polygon2 const& poly)
{
    double area = 0;
    size_t n_vertices = poly.size();
    size_t j = n_vertices - 1;
    for (size_t i = 0; i != n_vertices; j = i++)
    {
        area += poly[j].X * poly[i].Y;
        area -= poly[j].Y * poly[i].X;
    }
    return std::fabs(0.5 * area);
}

double get_area(clipper_polyset const& set)
{
    double area = 0;
    for (auto const& i : set)
        area += get_area(i);
    return area * clipper_integer_precision * clipper_integer_precision;
}

clipper_polyset smooth_polyset(clipper_polyset const& set, double smoothSize, double smoothWeight)
{
        polyset p_out = smooth_polyset(from_clipper(set), smoothSize, smoothWeight);
        return to_clipper(p_out);
}

}
