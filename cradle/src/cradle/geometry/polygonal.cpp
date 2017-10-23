#include <algorithm>

#ifdef _WIN32
    #pragma warning(push, 0)
    #include <boost/polygon/polygon.hpp>
    #pragma warning(pop)
#else
    #include <boost/polygon/polygon.hpp>
#endif

#include <cradle/geometry/angle.hpp>
#include <cradle/geometry/clipper.hpp>
#include <cradle/geometry/intersection.hpp>
#include <cradle/geometry/polygonal.hpp>

namespace cradle {


// COMMON FUNCTIONS

static vector<2,double> get_corner(box<2,double> const& box, int index)
{
    vector<2,double> corner;
    while(index >= 4)
    {
        index -= 4;
    }
    switch (index)
    {
        case 0:
            return box.corner;
        case 1:
            return box.corner + make_vector(box.size[0], 0.);
        case 2:
            return box.corner + make_vector(box.size[0], box.size[1]);
        case 3:
            return box.corner + make_vector(0., box.size[1]);
    }
    return make_vector(0.,0.);
}

// POLYGONS

polygon2 make_polygon2(std::vector<vertex2> const& vertices)
{
    polygon2 poly;
    initialize(&poly.vertices, vertices);
    return poly;
}

double get_area(polygon2 const& poly)
{
    return get_area_and_centroid(poly).first;
}

vector2d get_centroid(polygon2 const& poly)
{
    return get_area_and_centroid(poly).second;
}

std::pair<double, vector2d> get_area_and_centroid(polygon2 const& poly)
{
    double a = 0.0;
    double cx = 0.0;
    double cy = 0.0;
    for (polygon2_edge_view ev(poly); !ev.done(); ev.advance())
    {
        double cross = ev.p0()[0] * ev.p1()[1] - ev.p0()[1] * ev.p1()[0];
        a += cross;
        cx += (ev.p0()[0] + ev.p1()[0]) * cross;
        cy += (ev.p0()[1] + ev.p1()[1]) * cross;
    }
    double scale = 3.0 * a;
    return std::make_pair(std::fabs(0.5 * a),
        make_vector(cx / scale, cy / scale));
}

polygon2 scale(polygon2 const& poly, double factor)
{
    return scale(poly, make_vector(factor, factor));
}

struct point_scaling_fn
{
    vector<2,double> factor;
    vertex2 operator()(vertex2 const& src) const
    {
        return make_vector(src[0] * factor[0], src[1] * factor[1]);
    }
};

polygon2 scale(polygon2 const& poly, vector<2,double> const& factor)
{
    point_scaling_fn fn;
    fn.factor = factor;
    return map_points(poly, fn);
}

bool is_inside(polygon2 const& poly, vector<2,double> const& p)
{
    bool c = false;
    for (polygon2_edge_view ev(poly); !ev.done(); ev.advance())
    {
        if (((ev.p0()[1] <= p[1] && p[1] < ev.p1()[1]) ||
            (ev.p1()[1] <= p[1] && p[1] < ev.p0()[1]))
         && (p[0] < (ev.p1()[0] - ev.p0()[0]) * (p[1] - ev.p0()[1]) /
            (ev.p1()[1] - ev.p0()[1]) + ev.p0()[0]))
        {
            c = !c;
        }
    }
    return c;
}

bool is_inside(polygon2 const& parent_poly, polygon2 const& child_poly)
{
    for (auto const& point : child_poly.vertices)
    {
        if (!is_inside(parent_poly, point))
        {
            return false;
        }
    }
    return true;
}

bool is_ccw(polygon2 const& poly)
{
    assert(poly.vertices.size() > 2);
    vector<2,double> edge0 = poly.vertices[1] - poly.vertices[0];
    vector<2,double> edge1 = poly.vertices[2] - poly.vertices[1];
    return (edge0[0] * edge1[1]) - (edge0[1] * edge1[0]) > 0;
}

polygon2 as_polygon(box<2,double> const& box)
{
    polygon2 poly;
    vertex2* vertices = allocate(&poly.vertices, 4);
    vertices[0] = box.corner;
    vertices[1] = box.corner + make_vector(box.size[0], 0.);
    vertices[2] = box.corner + box.size;
    vertices[3] = box.corner + make_vector(0., box.size[1]);
    return poly;
}

polygon2 as_polygon(circle<double> const& circle, unsigned n_segments)
{
    polygon2 poly;
    vertex2* vertices = allocate(&poly.vertices, n_segments);
    for (unsigned i = 0; i < n_segments; ++i)
    {
        angle<double,radians> a(2 * pi * i / n_segments);
        vertices[i] =
            make_vector(cos(a), sin(a)) * circle.radius + circle.center;
    }
    return poly;
}

polygon2 as_polygon(triangle<2,double> const& tri)
{
    polygon2 poly;
    vertex2* vertices = allocate(&poly.vertices, 3);
    for (unsigned i = 0; i != 3; ++i)
        vertices[i] = tri[i];
    return poly;
}

// POLYSETS

void create_polyset(polyset* set, polygon2 const& poly)
{
    *set = polyset();
    set->polygons.push_back(poly);
}

polyset make_polyset(polygon2 const& poly)
{
    polyset p;
    create_polyset(&p, poly);
    return p;
}

void add_polygon(polyset& set, polygon2 const& poly)
{
    polyset addition;
    create_polyset(&addition, poly);
    if (set.polygons.empty())
        swap(set, addition);
    else
        do_set_operation(&set, set_operation::UNION, addition, set);
}
void add_hole(polyset& set, polygon2 const& hole)
{
    assert(!set.polygons.empty());
    polyset subtraction;
    create_polyset(&subtraction, hole);
    do_set_operation(&set, set_operation::DIFFERENCE, set, subtraction);
}

polyset remove_polyset_holes(
    polyset const& original_shape)
{
    polyset shape = original_shape;

    while (!shape.holes.empty())
    {
        do_set_operation(
            &shape,
            set_operation::UNION,
            shape,
            make_polyset(shape.holes.front()));
    }

    return shape;
}

polyset scale(polyset const& set, double factor)
{
    return scale(set, make_vector(factor, factor));
}
polyset scale(polyset const& set, cradle::vector<2,double> const& factor)
{
    point_scaling_fn fn;
    fn.factor = factor;
    return map_points(set, fn);
}

std::pair<double, vector2d> static
get_area_and_centroid(polyset const& set)
{
    double area = 0;
    auto centroid = make_vector(0., 0.);
    for (auto const& i : set.polygons)
    {
        auto pair = get_area_and_centroid(i);
        area += pair.first;
        centroid += pair.second * pair.first;
    }
    for (auto const& i : set.holes)
    {
        auto pair = get_area_and_centroid(i);
        area -= get_area(i);
        centroid -= pair.second * pair.first;
    }
    centroid /= area;
    return std::make_pair(area, centroid);
}

double get_area(polyset const& set)
{
    return get_area_and_centroid(set).first;
}

vector2d get_centroid(polyset const& set)
{
    auto pair = get_area_and_centroid(set);
    if (pair.first == 0)
        throw exception("centroid requested for empty polyset");
    return pair.second;
}

bool is_inside(polyset const& set, vector<2,double> const& p)
{
    int inside_count = 0;
    for (auto const& i : set.polygons)
    {
        if (is_inside(i, p))
            ++inside_count;
    }
    for (auto const& i : set.holes)
    {
        if (is_inside(i, p))
            --inside_count;
    }
    return inside_count > 0;
}

typedef line_segment<2, double> line_segment2;

double distance_to_polyset(vector<2,double> const& p, polyset const& set)
{
    double d2p = 1.0e10;
    size_t polygon_count = set.polygons.size();
    for (size_t k = 0; k < polygon_count; ++k)
    {
        size_t vertex_count = set.polygons[k].vertices.n_elements;
        auto v0 = set.polygons[k].vertices.elements[vertex_count - 1];
        for (size_t i = 0; i < vertex_count; ++i)
        {
            // Get segment end point
            auto v1 = set.polygons[k].vertices.elements[i];
            line_segment2 segment(v0, v1);
            auto len = length(segment);

            // Ensure the line segment has finite length
            if (len >= 1.0e-8)
            {
                // Discretize line segment and determine distance
                double spacing = 0.25;
                int split_count = std::min(std::max(int(std::ceil(len / spacing)), 2), 30);
                for (int j = 0; j < split_count; ++j)
                {
                    double dist = distance(segment, p);

                    if (dist < std::fabs(d2p))
                    {
                        bool pip = point_in_polygon(p, set.polygons[k]);
                        d2p = pip ? -dist : dist;
                    }
                }
            }

            // Increment segment start point
            v0 = v1;
        }
    }

    // Handle Holes too
    size_t hole_count = set.holes.size();
    for (size_t k = 0; k < hole_count; ++k)
    {
        size_t vertex_count = set.holes[k].vertices.n_elements;
        auto v0 = set.holes[k].vertices.elements[vertex_count - 1];
        for (size_t i = 0; i < vertex_count; ++i)
        {
            // Get segment end point
            auto v1 = set.holes[k].vertices.elements[i];
            line_segment2 segment(v0, v1);
            auto len = length(segment);

            // Ensure the line segment has finite length
            if (len >= 1.0e-8)
            {
                // Discretize line segment and determine distance
                double spacing = 0.25;
                int split_count = std::min(std::max(int(std::ceil(len / spacing)), 2), 30);
                for (int j = 0; j < split_count; ++j)
                {
                    double dist = distance(segment, p);

                    if (dist < std::fabs(d2p))
                    {
                        bool pip = point_in_polygon(p, set.holes[k]);
                        d2p = pip ? dist : -dist;
                    }
                }
            }

            // Increment segment start point
            v0 = v1;
        }
    }

    return d2p;
}

namespace {

    typedef boost::polygon::point_data<int> boost_point;

    typedef boost::polygon::polygon_data<int> boost_polygon;

    typedef boost::polygon::polygon_with_holes_data<int>
        boost_polygon_with_holes;

    typedef boost::polygon::polygon_set_data<int> boost_polygon_set;

    void to_boost_polygon(boost_polygon& bp, polygon2 const& poly)
    {
        size_t n_points = poly.vertices.size();
        bp.coords_.resize(n_points);
        for (size_t i = 0; i != n_points; ++i)
        {
            vector<2,double> const& p = poly.vertices[i];
            bp.coords_[i] = boost_polygon::point_type(
                int(p[0] / clipper_integer_precision),
                int(p[1] / clipper_integer_precision));
        }
    }

    void to_boost_polygon(boost_polygon_set& boost_set,
        polyset const& cradle_set)
    {
        using namespace boost::polygon::operators;
        boost_set.clear();
        for (auto const& i : cradle_set.polygons)
        {
            boost_polygon poly;
            to_boost_polygon(poly, i);
            boost_set ^= poly;
        }
        for (auto const& i : cradle_set.holes)
        {
            boost_polygon poly;
            to_boost_polygon(poly, i);
            boost_set ^= poly;
        }
    }

    vector<2,double>
    from_boost_point(boost_point const& p)
    {
        return make_vector<double>(
            double(p.x()) * clipper_integer_precision,
            double(p.y()) * clipper_integer_precision);
    }

    void from_boost_polygon(polygon2& poly, boost_polygon const& bp)
    {
        size_t n_points = bp.size();
        if (n_points != 0)
        {
            // The first and last vertices are the same, so skip the last.
            --n_points;
            vertex2* vertices = allocate(&poly.vertices, n_points);
            for (size_t i = 0; i != n_points; ++i)
                vertices[i] = from_boost_point(bp.coords_[i]);
        }
        else
            clear(&poly.vertices);
    }

    void from_boost_polygon(polyset& cradle_set,
        boost_polygon_set const& boost_set)
    {
        cradle_set = polyset();
        std::vector<boost_polygon_with_holes> boost_polys;
        boost_set.get(boost_polys);
        for (auto const& bp : boost_polys)
        {
            polygon2 poly;
            from_boost_polygon(poly, bp.self_);
            cradle_set.polygons.push_back(poly);
            for (auto const& bh : bp.holes_)
            {
                polygon2 hole;
                from_boost_polygon(hole, bh);
                cradle_set.holes.push_back(hole);
            }
        }
    }

    void from_boost_polygon(std::vector<polygon2>& polys,
        boost_polygon_set const& set)
    {
        std::vector<boost_polygon> boost_polys;
        set.get(boost_polys);
        size_t n_polys = boost_polys.size();
        polys.resize(n_polys);
        for (size_t i = 0; i != n_polys; ++i)
        {
            boost_polygon const& bp = boost_polys[i];
            polygon2& poly = polys[i];
            from_boost_polygon(poly, bp);
        }
    }

} // anonymous namespace

std::vector<polygon2> as_polygon_list(polyset const& set)
{
    boost_polygon_set boost_set;
    to_boost_polygon(boost_set, set);
    std::vector<polygon2> polygons;
    from_boost_polygon(polygons, boost_set);
    return polygons;
}

bool almost_equal(polyset const& set1, polyset const& set2, double tolerance)
{
    polyset xor_;
    do_set_operation(&xor_, set_operation::XOR, set1, set2);
    return almost_equal(get_area(xor_), 0., tolerance);
}

bool almost_equal(polyset const& set1, polyset const& set2)
{
    polyset xor_;
    do_set_operation(&xor_, set_operation::XOR, set1, set2);
    return almost_equal(get_area(xor_), 0.);
}

void do_set_operation(
    polyset* result,
    set_operation op,
    polyset const& set1,
    polyset const& set2)
{
    ClipperLib::Clipper clipper;
    clipper.AddPolygons(to_clipper(set1), ClipperLib::ptSubject);
    clipper.AddPolygons(to_clipper(set2), ClipperLib::ptClip);
    ClipperLib::ClipType clipper_op;
    switch (op)
    {
     case set_operation::UNION:
        clipper_op = ClipperLib::ctUnion;
        break;
     case set_operation::INTERSECTION:
        clipper_op = ClipperLib::ctIntersection;
        break;
     case set_operation::DIFFERENCE:
        clipper_op = ClipperLib::ctDifference;
        break;
     case set_operation::XOR:
        clipper_op = ClipperLib::ctXor;
        break;
     default:
         throw exception("set operation undefined");
    }
    ClipperLib::Polygons solution;
    clipper.Execute(clipper_op, solution);
    from_clipper(result, solution);
}

api(fun)
polyset
polyset_combination(set_operation op, std::vector<polyset> const& polysets)
{
    if (polysets.empty())
    {
        return polyset();
    }
    else if (polysets.size() == 1)
    {
        return polysets[0];
    }
    polyset tmp[2];
    int output_index = 0;
    polyset const* input = &polysets.front();
    auto i = polysets.begin();
    ++i;
    for (; i != polysets.end(); ++i)
    {
        output_index = 1 - output_index;
        auto output = &tmp[output_index];
        do_set_operation(output, op, *input, *i);
        input = output;
    }
    return tmp[output_index];
}

// Triangulate a convex Boost polygon.
// Note that this does NOT clear the list of triangles.
// It simply pushes more triangles onto the back of it.
static void
triangulate_boost_polygon(
    std::vector<triangle<2,double> >& tris, boost_polygon const& poly)
{
    size_t n_points = poly.size();
    if (n_points > 0)
    {
        // The first and last vertices are the same, so skip the last.
        --n_points;
        for (size_t i = 2; i < n_points; ++i)
        {
            triangle<2,double> tri;
            tri[0] = from_boost_point(poly.coords_[0]);
            tri[1] = from_boost_point(poly.coords_[i - 1]);
            tri[2] = from_boost_point(poly.coords_[i]);
            tris.push_back(tri);
        }
    }
}

std::vector<triangle<2,double> >
triangulate_polyset(polyset const& set)
{
    boost_polygon_set boost_set;
    to_boost_polygon(boost_set, set);
    std::vector<boost_polygon> trapezoids;
    boost_set.get_trapezoids(trapezoids);
    size_t n_trapezoids = trapezoids.size();
    std::vector<triangle<2,double> > tris;
    tris.reserve(n_trapezoids * 2);
    for (size_t i = 0; i != n_trapezoids; ++i)
        triangulate_boost_polygon(tris, trapezoids[i]);
    return tris;
}

void
expand(polyset* dst, polyset const& src, double amount)
{
    using namespace boost::polygon::operators;
    auto clipper_in = to_clipper(src);
    ClipperLib::Polygons clipper_out;
    OffsetPaths(clipper_in, clipper_out, amount / clipper_integer_precision,
        ClipperLib::jtRound, ClipperLib::etClosed);
    from_clipper(dst, clipper_out);
}

polyset polyset_expansion(polyset const& src, double amount)
{
    polyset result;
    expand(&result, src, amount);
    return result;
}

void create_polyset_from_polygons(polyset* set,
    std::vector<polygon2> const& polygons)
{
    using namespace boost::polygon::operators;
    boost_polygon_set boost_set;
    for (std::vector<polygon2>::const_iterator
        i = polygons.begin(); i != polygons.end(); ++i)
    {
        boost_polygon bp;
        to_boost_polygon(bp, *i);
        boost_set += bp;
    }
    from_boost_polygon(*set, boost_set);
}

// STRUCTURES

bool is_inside(structure_geometry_slice const& s, double p)
{
    return ((p >= s.position - (s.thickness / 2.)) &&
        (p < s.position + (s.thickness / 2.)));
}

void static
reset_structure_to_slice_list(
    structure_geometry* structure,
    slice_description_list const& slices)
{
    structure->slices.clear();
    structure->master_slice_list.clear();
    structure->master_slice_list = slices;
}

structure_geometry static
remove_empty_slices(structure_geometry const& structure)
{
    structure_geometry result;
    result.master_slice_list = structure.master_slice_list;
    for (auto const& slice : structure.slices)
    {
        if (!is_empty(slice.second))
        {
            result.slices.insert(slice);
        }
    }
    return result;
}

polyset
get_slice(structure_geometry const& structure, double position)
{
    auto slice = get_structure_slice(structure, position);
    if (slice)
    {
        return get(slice).region;
    }

    return polyset();
}

structure_geometry_slice
find_slice_at_exact_position(
    structure_polyset_list const& slices,
    double position,
    double thickness)
{
    auto s = slices.find(position);
    if (s != slices.end())
    {
        return structure_geometry_slice(position, thickness, s->second);
    }
    return structure_geometry_slice(position, thickness, polyset());
}

optional<structure_geometry_slice>
get_structure_slice(structure_geometry const& structure, double position)
{
    auto const& masters = structure.master_slice_list;
    auto const& slices = structure.slices;
    auto p = position;

    if (masters.size() == 0 ||
        p < masters[0].position - 0.5 * masters[0].thickness ||
        p > masters.back().position + 0.5 * masters.back().thickness)
    {
        return none;
    }

    for (size_t i = 1; i < masters.size(); ++i)
    {
        if (p < masters[i].position)
        {
            if (p - masters[i - 1].position < masters[i].position - p)
            {
                return
                    some(
                        find_slice_at_exact_position(
                            slices,
                            masters[i - 1].position,
                            masters[i - 1].thickness));
            }
            else
            {
                return
                    some(
                        find_slice_at_exact_position(
                            slices,
                            masters[i].position,
                            masters[i].thickness));
            }
        }
    }
    // Valid case when p is past the last slice position, but within its thickness
    return
        some(
            find_slice_at_exact_position(
                slices,
                masters.rbegin()->position,
                masters.rbegin()->thickness));
}

std::vector<structure_geometry_slice>
get_structure_slices(structure_geometry const& structure, double p_low, double p_high)
{
    auto const& masters = structure.master_slice_list;
    auto const& slices = structure.slices;

    std::vector<structure_geometry_slice> output;

    if (masters.size() == 0 ||
        p_low > masters.back().position + 0.5 * masters.back().thickness ||
        p_high < masters[0].position - 0.5 * masters[0].thickness)
    {
        return output;
    }

    // Find the start and end slice indices
    // Note: the initial value set below is critical to this functioning properly
    size_t i_start = masters.size();

    for (size_t i = 1; i < masters.size(); ++i)
    {
        if (p_low < masters[i].position)
        {
            if (p_low - masters[i - 1].position < masters[i].position - p_low)
            {
                i_start = i - 1;
            }
            else
            {
                i_start = i;
            }
            output.push_back(
                find_slice_at_exact_position(
                    slices,
                    masters[i_start].position,
                    masters[i_start].thickness));
            break;
        }
    }

    for (size_t i = i_start + 1; i < masters.size(); ++i)
    {
        if (p_high < masters[i].position)
        {
            if (p_high - masters[i - 1].position < masters[i].position - p_high)
            {
                // Last slice is already in the list, just return now
                return output;
            }
            else
            {
                // Add the last slice and then return
                output.push_back(
                    find_slice_at_exact_position(
                        slices,
                        masters[i].position,
                        masters[i].thickness));
                return output;
            }
        }

        // Slice is within limits, add it
        output.push_back(
            find_slice_at_exact_position(
                slices,
                masters[i].position,
                masters[i].thickness));
    }

    // All slices have been added (but never reached p_high position), just return
    return output;
}

void static
fold_in(double& volume, vector3d& centroid,
    std::pair<double,vector2d> const& area_and_centroid,
    double z, double thickness)
{
    volume += area_and_centroid.first * thickness;
    centroid += unslice(area_and_centroid.second, 2, z) *
        area_and_centroid.first * thickness;
}

void static
fold_in_above(double& volume, vector3d& centroid,
    std::pair<double,vector2d> const& area_and_centroid,
    double z, double thickness)
{
    fold_in(volume, centroid, area_and_centroid, z + thickness / 2, thickness);
}

void static
fold_in_below(double& volume, vector3d& centroid,
    std::pair<double,vector2d> const& area_and_centroid,
    double z, double thickness)
{
    fold_in(volume, centroid, area_and_centroid, z - thickness / 2, thickness);
}

std::pair<double,vector3d> static
get_volume_and_centroid(structure_geometry const& structure)
{
    auto volume = 0.;
    auto centroid = make_vector(0., 0., 0.);

    auto const& masters = structure.master_slice_list;
    auto const& slices = structure.slices;

    auto begin_m = masters.begin();
    auto end_m = masters.end();
    auto i = begin_m;
    auto end_slice = slices.end();

    while (i != end_m)
    {
        auto slice = slices.find(i->position);
        if (slice != end_slice && get_area(slice->second) != 0.)
        {
            break;
        }
        ++i;
    }

    for (; i != end_m; ++i)
    {
        auto slice = slices.find(i->position);
        if (slice == end_slice) { continue; }

        auto i_info = get_area_and_centroid(slice->second);

        // Add lower half of slice thickness
        auto temp = i;
        if (i == begin_m)
        {
            fold_in_below(volume, centroid, i_info, i->position, i->thickness * 0.5);
        }
        else
        {
            --temp;
            fold_in_below(
                volume, centroid,
                i_info,
                i->position,
                0.5 * (i->position - temp->position));
        }

        // Add upper half of slice thickness
        temp = i;
        ++temp;
        if (temp == end_m)
        {
            fold_in_above(volume, centroid, i_info, i->position, i->thickness * 0.5);
        }
        else
        {
            fold_in_above(
                volume, centroid,
                i_info,
                i->position,
                0.5 * (temp->position - i->position));
        }
    }

    if (volume != 0.)
    {
        centroid /= volume;
    }

    return std::make_pair(volume, centroid);
}

double get_volume(structure_geometry const& structure)
{
    return get_volume_and_centroid(structure).first;
}

vector3d get_centroid(
    structure_geometry const& structure)
{
    auto pair = get_volume_and_centroid(structure);
    if (pair.first == 0)
        throw exception("centroid requested for empty structure");
    return pair.second;
}

bool is_inside(structure_geometry const& structure, vector<3,double> const& p)
{
    auto s = get_slice(structure, p[2]);
    return is_inside(s, slice(p, 2));
}

slice_description_list
get_slice_descriptions(structure_geometry const& s)
{
    return s.master_slice_list;
}

bool
almost_equal(
    structure_geometry const& volume1,
    structure_geometry const& volume2,
    double tolerance)
{
    if (volume1.master_slice_list.size() != volume2.master_slice_list.size())
        return false;

    auto i1 = volume1.master_slice_list.begin();
    auto end1 = volume1.master_slice_list.end();
    auto i2 = volume2.master_slice_list.begin();

    auto const& slices1 = volume1.slices;
    auto const& slices2 = volume2.slices;

    for (; i1 != end1; ++i1, ++i2)
    {
        if (!almost_equal(i1->position, i2->position, tolerance) ||
            !almost_equal(i1->thickness, i2->thickness, tolerance))
        {
            return false;
        }
        auto r1 = find_slice_at_exact_position(slices1, i1->position, i1->thickness);
        auto r2 = find_slice_at_exact_position(slices2, i2->position, i2->thickness);
        if (!almost_equal(r1.region, r2.region, tolerance))
        {
            return false;
        }
    }

    return true;
}

bool almost_equal(structure_geometry const& a, structure_geometry const& b)
{
    return almost_equal(a, b, default_equality_tolerance<double>());
}

void do_set_operation(
    structure_geometry* result,
    set_operation op,
    structure_geometry const& structure1,
    structure_geometry const& structure2)
{
    if (structure1.master_slice_list != structure2.master_slice_list)
    {
        throw exception("structure set_operation requires matching master lists");
    }

    auto const& master_slices = structure1.master_slice_list;

    result->master_slice_list = master_slices;
    result->slices.clear();
    for (auto const& i : master_slices)
    {
        auto s1 =
            find_slice_at_exact_position(structure1.slices, i.position, i.thickness);
        auto s2 =
            find_slice_at_exact_position(structure2.slices, i.position, i.thickness);

        if (!is_empty(s1.region) || !is_empty(s2.region))
        {
            polyset region;
            do_set_operation(&region, op, s1.region, s2.region);
            if (!is_empty(region))
            {
                result->slices[i.position] = region;
            }
        }
    }
}

structure_geometry
structure_combination(
    set_operation op,
    std::vector<structure_geometry> const& structures)
{
    if (structures.size() < 2)
    {
        throw exception("structure_combination requires at least two structures");
    }
    structure_geometry tmp[2];
    int output_index = 0;
    structure_geometry const* input = &structures.front();
    auto i = structures.begin();
    ++i;
    for (; i != structures.end(); ++i)
    {
        output_index = 1 - output_index;
        auto output = &tmp[output_index];
        do_set_operation(output, op, *input, *i);
        input = output;
    }
    return tmp[output_index];
}

std::vector<double>
slice_position_list(structure_geometry const& structure)
{
    return map(
        [](slice_description const& slice)
        {
            return slice.position;
        },
        structure.master_slice_list);
}

void expand_in_2d(
    structure_geometry* result,
    structure_geometry const& structure,
    double amount)
{
    auto slice_descriptions = get_slice_descriptions(structure);
    reset_structure_to_slice_list(result, slice_descriptions);
    for (auto const& slice : structure.slices)
    {
        polyset p;
        expand(&p, slice.second, amount);
        result->slices[slice.first] = p;
    }
}

box<2,double>
bounding_box(polygon2 const& poly)
{
    optional<box<2,double> > box;
    compute_bounding_box(box, poly);
    // If there is no box, then the polygon is empty, so just return any box.
    return box ? box.get() :
        make_box(make_vector(0., 0.), make_vector(0., 0.));
}
void compute_bounding_box(
    optional<box<2,double> >& box,
    polygon2 const& poly)
{
    vertex2_array::const_iterator
        i = poly.vertices.begin(),
        end = poly.vertices.end();
    if (i == end)
        return;

    vector<2,double> min, max;
    if (box)
    {
        min = get_low_corner(box.get());
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
        for (unsigned j = 0; j != 2; ++j)
        {
            if ((*i)[j] < min[j])
                min[j] = (*i)[j];
            if ((*i)[j] > max[j])
                max[j] = (*i)[j];
        }
    }

    box = cradle::box<2,double>(min, max - min);
}

// for a polyset
box<2,double> bounding_box(polyset const& region)
{
    optional<box<2,double> > box;
    compute_bounding_box(box, region);
    // If there is no box, then the polyset is empty, so just return any box.
    return box ? box.get() :
        make_box(make_vector(0., 0.), make_vector(0., 0.));
}
void compute_bounding_box(optional<box<2,double> >& box, polyset const& region)
{
    for (auto const& i : region.polygons)
        compute_bounding_box(box, i);
}

// for a structure_geometry
box<3,double> bounding_box(structure_geometry const& structure)
{
    optional<box<3,double> > box;
    compute_bounding_box(box, structure);
    // If there is no box, then the structure is empty, so just return any box.
    return box ? box.get() :
        make_box(make_vector(0., 0., 0.), make_vector(0., 0., 0.));
}
void compute_bounding_box(optional<box<3,double> >& box,
    structure_geometry const& structure)
{
    optional<cradle::box<2,double> > xy_box;
    if (box)
    {
        xy_box = slice(box.get(), 2);
    }

    double zmin = 1.0e100;
    double zmax = -1.0e100;
    auto master_slices = get_slice_descriptions(structure);
    for (auto const& m_slice : master_slices)
    {
        auto slice =
            find_slice_at_exact_position(
                structure.slices,
                m_slice.position,
                m_slice.thickness);
        if (!is_empty(slice.region))
        {
            compute_bounding_box(xy_box, slice.region);

            if (slice.position - 0.5 * slice.thickness < zmin)
            {
                zmin = slice.position - 0.5 * slice.thickness;
            }
            if (slice.position + 0.5 * slice.thickness > zmax)
            {
                zmax = slice.position + 0.5 * slice.thickness;
            }
        }
    }


    if (!structure.slices.empty() && xy_box)
    {
        //double z_min = structure.slices.begin()->position -
        //    structure.slices.begin()->thickness / 2;
        //auto last = structure.slices.end();
        //--last;
        //double z_max = last->position + last->thickness / 2;

        vector<2,double> const& xy_corner = xy_box.get().corner;
        vector<2,double> const& xy_size = xy_box.get().size;

        box = cradle::box<3,double>(
            make_vector(xy_corner[0], xy_corner[1], zmin),
            make_vector(xy_size[0], xy_size[1], zmax - zmin));
    }
}


bool
overlapping(
    box3d const& box,
    structure_geometry const& sg,
    unsigned structure_axis,
    optional<box3d> const& sg_bounds)
{
    if (sg_bounds)
    {
        if (!overlapping(box, get(sg_bounds)))
        {
            return false;
        }
    }

    // Get slices that the box contains
    auto slices =
        get_structure_slices(
            sg,
            box.corner[structure_axis],
            get_high_corner(box)[structure_axis]);

    // Check for overlap on each slice
    for (auto const& s : slices)
    {
        if (s.region.polygons.size() == 0)
        {
            continue;
        }

        auto const box2 = slice(box, structure_axis);

        // Check center of voxel as this may be a fast "short-circuit" for many cases
        if (point_in_polyset(get_center(box2), s.region))
        {
            return true;
        }
        else
        {
            // Check all points in the polyset
            // Note this isn't 100% accurate because holes can actually make a polygon
            // vertex outside and we don't catch that case
            for (auto const& p : s.region.polygons)
            {
                for (auto const& v : p.vertices)
                {
                    if (contains(box2, v))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


bool almost_equal(polygon2 const& a, polygon2 const& b, double tolerance)
{
    polyset region_a, region_b, xor_region;
    create_polyset(&region_a, a);
    create_polyset(&region_b, b);
    do_set_operation(&xor_region, set_operation::XOR, region_a, region_b);
    return almost_equal(get_area(xor_region), 0., tolerance);
}
bool almost_equal(polygon2 const& a, polygon2 const& b)
{
    polyset region_a, region_b, xor_region;
    create_polyset(&region_a, a);
    create_polyset(&region_b, b);
    do_set_operation(&xor_region, set_operation::XOR, region_a, region_b);
    return almost_equal(get_area(xor_region), 0.);
}

}
