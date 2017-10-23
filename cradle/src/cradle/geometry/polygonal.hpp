#ifndef CRADLE_GEOMETRY_POLYGONAL_HPP
#define CRADLE_GEOMETRY_POLYGONAL_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/slicing.hpp>
#include <map>

namespace cradle {

// POLYGONS

typedef vector<2,double> vertex2;

typedef array<vertex2> vertex2_array;

// Polygon2 represents a closed loop of vertices in 2D space.
// The initial vertex is only stored once, so the polygon is implicitly closed.
// Additionally, the polygon's edges must not intersect one another.
api(struct)
struct polygon2
{
    // Array list of 2d vertices that compose the polygon (2d)
    vertex2_array vertices;
};

polygon2 make_polygon2(std::vector<vertex2> const& vertices);

// Computes the area of a polygon shape
api(fun trivial name(polygon_area))
// Area of the polygon
double get_area(
    // Polygon to compute the area of
    polygon2 const& poly);

// Computes the geometric center of a polygon
api(fun trivial name(polygon_centroid))
// The geometric center
vector2d get_centroid(
    // Polygon shape
    polygon2 const& poly);

std::pair<double, vector2d> get_area_and_centroid(polygon2 const& poly);

polygon2 scale(polygon2 const& poly, double factor);

// Scales a polygon shape in XY (independently) based on a vector2D factor
api(fun trivial name(scale_polygon))
// Scaled polygon
polygon2 scale(
    // Polygon shape to scale
    polygon2 const& poly,
    // Desired X,y scale factor
    vector<2,double> const& factor);

bool is_inside(polygon2 const& poly, vector<2,double> const& p);

bool is_inside(polygon2 const& parent_poly, polygon2 const& child_poly);

// Test if a point is inside a polygon.
api(fun trivial)
// Result of if the specified point is within the polygon
bool point_in_polygon(
    // 2D point to check
    vector<2,double> const& p,
    // Polygon to check if contains point
    polygon2 const& poly)
{ return is_inside(poly, p); }

// Applies a mapping function to each point on the source polygon to generate
// the result polygon.
template<class Fn>
polygon2 map_points(polygon2 const& src, Fn const& fn)
{
    size_t n_vertices = src.vertices.size();
    polygon2 dst;
    vertex2* dst_vertices = allocate(&dst.vertices, n_vertices);
    for (size_t i = 0; i != n_vertices; ++i)
        dst_vertices[i] = fn(src.vertices[i]);
    return dst;
}

// Determine if the winding order of a 2D polygon is CCW, as viewed in a
// standard 2D coordinate system with +Y up and +X to the right.
bool is_ccw(polygon2 const& poly);

struct polygon2_edge_view
{
    polygon2_edge_view(polygon2 const& poly)
      : p0_(poly.vertices.end() - 1)
      , p1_(poly.vertices.begin())
      , end_(poly.vertices.end())
    {}

    vector<2,double> const& p0() const { return *p0_; }
    vector<2,double> const& p1() const { return *p1_; }

    bool done() const { return p1_ == end_; }

    void advance() { p0_ = p1_; ++p1_; }

 private:
    vertex2_array::const_iterator p0_, p1_, end_;
};

// as_polygon(x) is a utility function that allows other objects to be treated
// as polygons.

api(fun trivial name(box_as_polygon))
polygon2 as_polygon(box<2,double> const& box);

api(fun trivial name(circle_as_polygon))
polygon2 as_polygon(circle<double> const& circle, unsigned n_segments);

api(fun trivial name(triangle_as_polygon))
polygon2 as_polygon(triangle<2,double> const& tri);

// Determine if two 2D polygons are almost equal.
bool almost_equal(polygon2 const& a, polygon2 const& b, double tolerance);
bool almost_equal(polygon2 const& a, polygon2 const& b);

// Get the bounding box of a polygon.
api(fun trivial name(polygon_bounding_box))
// Bounding box of the polygon
box<2,double> bounding_box(
    // Polygon to compute the bounding box of
    polygon2 const& poly);

void compute_bounding_box(
    optional<box<2,double> >& box,
    polygon2 const& poly);

// POLYSETS

typedef std::vector<polygon2> polygon2_list;

// A polyset represents an arbitrary (possibly noncontiguous) region in 2D
// space using a set of polygons, each with a set of holes. The polygons
// outline the included regions. The holes outline areas inside the polygons
// that are excluded. Polygons & holes must be non-intersecting.
api(struct)
struct polyset
{
    // List of polygons that represent the shape of the polyset
    polygon2_list polygons;
    // List of polygons that represent the holes of the polyset
    polygon2_list holes;
};

static inline bool is_empty(polyset const& set)
{ return set.polygons.empty() && set.holes.empty(); }

// Is this polyset empty?
api(fun trivial)
bool polyset_empty(polyset const& set)
{ return is_empty(set); }

// Create a polyset with a single polygon and no holes.
api(fun trivial)
polyset make_polyset(polygon2 const& poly);

// Create a polyset with a single polygon and no holes.
// (This is mostly for legacy purposes, as a lot of code uses it.)
void create_polyset(polyset* set, polygon2 const& poly);

// Create a polyset from a list of polygons (and no holes).
void create_polyset_from_polygons(polyset* set,
    std::vector<polygon2> const& polygons);

void add_polygon(polyset& set, polygon2 const& poly);
void add_hole(polyset& set, polygon2 const& hole);

// Remove holes, and polygons fully contained by holes,
// within an existing polyset
api(fun)
// The polyset
polyset remove_polyset_holes(
    // Polyset to remove holes from
    polyset const& original_shape);

polyset scale(polyset const& set, double factor);

// Scales a polyset shape in XY (independently) based on a vector2D factor
api(fun trivial name(scale_polyset))
// The scaled polyset
polyset scale(
    // Polyset to scale
    polyset const& set,
    // Desired X,y scale factor
    cradle::vector<2,double> const& factor);

// Get the area of a polyset.
api(fun trivial name(polyset_area))
double get_area(polyset const& set);

// Get the centroid of a polyset.
api(fun trivial name(polyset_centroid))
// The geometric center of the total polyset
vector2d get_centroid(
    // The polyset to analyze for the centroid
    polyset const& set);

bool is_inside(polyset const& set, vector<2,double> const& p);

// Test if a point is inside a polyset.
api(fun trivial)
// True if point is contained within the polyset
bool point_in_polyset(
    // 2D point to check
    vector<2,double> const& p,
    // Polyset to check if contains point
    polyset const& set)
{ return is_inside(set, p); }

// Get the distance from point to a polyset (inside < 0).
api(fun trivial)
// Distance to polyset. Points inside the polyset will return a negative distance
double distance_to_polyset(
    // 2D point to find distance between
    vector<2,double> const& p,
    // polyset to which distance is to be found
    polyset const& set);

// Convert the given polyset to a list of polygons (no holes).
api(fun name(polyset_as_polygon_list))
std::vector<polygon2> as_polygon_list(
    // Polyset to get polygon list from
    polyset const& set);

// Triangulate a polyset.
api(fun)
// Triangulated polyset
std::vector<triangle<2,double> >
triangulate_polyset(
    // Polyset to triangulate
    polyset const& set);

// Determine if the two polysets are almost equal.
bool almost_equal(polyset const& set1, polyset const& set2,
    double tolerance);
bool almost_equal(polyset const& set1, polyset const& set2);

// Get the bounding box of a polyset.
api(fun trivial name(polyset_bounding_box))
// Bounding box of polyset
box<2,double> bounding_box(
    // Polyset to get the bounding box for
    polyset const& set);

void compute_bounding_box(
    optional<box<2,double> >& box,
    polyset const& set);

// Applies a mapping function to each point on the source polyset to generate
// the result polyset.
template<class Fn>
polyset map_points(polyset const& src, Fn const& fn)
{
    polyset dst;

    size_t n_polys = src.polygons.size();
    dst.polygons.resize(n_polys);
    for (size_t i = 0; i != n_polys; ++i)
        dst.polygons[i] = map_points(src.polygons[i], fn);

    size_t n_holes = src.holes.size();
    dst.holes.resize(n_holes);
    for (size_t i = 0; i != n_holes; ++i)
        dst.holes[i] = map_points(src.holes[i], fn);

    return dst;
}

// Expands a polyset uniformly around the edges by the given amount.
// Note that if amount is negative, this is actually a contraction.
// If the polyset contains any holes, the holes will be removed from the polyset during expansion.
api(fun)
// Expanded polyset.
polyset polyset_expansion(
    // Polyset to expand
    polyset const& set,
    // Expansion amount; Positive values will expand the polyset and negative
    // numbers will contract the polyset.
    double amount);

void expand(polyset* dst, polyset const& src, double amount);

// Smooths a polyset via a distance weighted blurring.
api(fun trivial)
// The smoothed polyset
polyset smooth_polyset(
        // The polyset to be smoothed.
        polyset const& set,
        // The size factor of the Gaussian blur for smoothing (larger values increase smoothing).
        double smoothSize,
        // The weight factor of the Gaussian blur for smoothing (smaller values increase smoothing).
        // Note this value must be greater than 0.
        double smoothWeight);

// Smooths a polyset via an edge midpoint shift.
api(fun)
// The smoothed polyset
polyset smooth_polyset_2(
        // The polyset to be smoothed.
        polyset const& set,
        // The shift factor of the smoothing (ranges from 0.0 to 1.0).
        double smoothSize,
        // Number of smoothing iterations (larger values increase smoothing).
        int iter);

// Splits a polyset along the given ray.
// Note that the function returns the portion of set that lies to the left side of the ray.
// The ray must have a unit vector as its direction.
api(fun)
// Polyset of the portion of set on the left side of ray.
polyset split_polyset(
    // The polyset to split.
    polyset const& set,
    // Ray along which the polygon is to be split.
    ray<2,double> r);

// Slices a polyset along the given axis.
api(fun)
// List of points that intersect slice plane and polyset.
std::vector<vector3d> slice_polyset(
    // The polyset to be sliced.
    polyset const& p,
    // The axis the polyset lies on.
    unsigned polyset_axis,
    // The position of the polyset along the axis it lies on.
    double polyset_position,
    // The axis of the plane used to slice the polyset.
    unsigned slice_axis,
    // The position of the slice plane along its axis.
    double slice_position);

// Visual C++ seems to define DIFFERENCE somewhere in its standard headers.
#ifdef DIFFERENCE
    #undef DIFFERENCE
#endif

// Which type of boolean operation to perform for contour/structure manipulation in a left-associative manner.
api(enum)
enum class set_operation
{
    // The total combination of the sets used
    UNION,
    // The portion of intersection of the sets used
    INTERSECTION,
    // The boolean subtraction of the sets used
    DIFFERENCE,
    // Exclusive-Or of the sets used
    XOR
};

void do_set_operation(
    polyset* result,
    set_operation op,
    polyset const& set1,
    polyset const& set2);

// Compute a combination of two or more polysets.
// If more than two polysets are specified, the operator is applied in a
// left-associative manner.
api(fun trivial)
// Resulting combined poylset
polyset polyset_combination(
    // Enum definition of which type of combine operation to perform
    set_operation op,
    // List of polysets
    std::vector<polyset> const& polysets);

// STRUCTURES

// Represents a 2D slice of a structure as a polyset, a slice thickness,
// and the prescribed position along the slice axis.
api(struct)
struct structure_geometry_slice
{
    // Position of the slice in the structure along the slice axis
    double position;
    // Thickness of the slice
    double thickness;
    // Polyset of the structure on the slice
    polyset region;
};

// Determines if the position p is inside the slice s?
bool is_inside(structure_geometry_slice const& s, double p);

typedef std::map<double, polyset> structure_polyset_list;

// Represents an arbitrary (possibly noncontiguous) volume in 3D space.
// It uses a stack of polysets, which represent 2D slices through
// the 3D volume. It must be sorted by position in ascending order.
api(struct)
struct structure_geometry
{
    // List of polysets making up the actual structure slices
    structure_polyset_list slices;

    // Master list of all slices that this structure may potentially have contours on
    slice_description_list master_slice_list;
};

// Get the slice description list for the structure s.
api(fun name(structure_slice_descriptions))
slice_description_list
get_slice_descriptions(structure_geometry const& s);

// Get the slice that contains the given out-of-plane position.
// If the position is outside all actual slices, empty polyset is returned.
api(fun name(get_structure_slice_as_polyset))
polyset
get_slice(structure_geometry const& structure, double position);

// Get the slice that contains the given out-of-plane position.
api(fun)
optional<structure_geometry_slice>
get_structure_slice(structure_geometry const& structure, double position);

// Get the slices that are within the bounds of the given out-of-plane position limits.
std::vector<structure_geometry_slice>
get_structure_slices(structure_geometry const& structure, double p_low, double p_high);

// Constructs a structure_geometry_slice at a know position in the structure master list.
structure_geometry_slice
find_slice_at_exact_position(
    structure_polyset_list const& slices,
    double position,
    double thickness);

static inline bool is_empty(structure_geometry const& structure)
{ return structure.slices.empty(); }

// Is this structure empty?
api(fun trivial)
bool structure_empty(structure_geometry const& structure)
{ return is_empty(structure); }

// Get the volume of a structure.
api(fun disk_cached name(structure_volume))
// Volume of the structure
double get_volume(
    // Structure geometry to calculate volume of
    structure_geometry const& structure);

// Get the centroid of a structure.
api(fun disk_cached name(structure_centroid))
// 3d point representing the centroid of the structure
vector3d get_centroid(
    // Structure to calculate the centroid of
    structure_geometry const& structure);

// Is p inside the given structure?
bool is_inside(structure_geometry const& structure, vector<3,double> const& p);

// Test if a point is inside a structure.
api(fun)
// Returns true if the point is in the structure
bool point_in_structure(
    // Specified 3D point
    vector<3,double> const& p,
    // Structure to compute if point is containing
    structure_geometry const& structure)
{ return is_inside(structure, p); }

// Determine if the two structures are almost equal.
bool almost_equal(structure_geometry const& structure1,
    structure_geometry const& structure2, double tolerance);
bool almost_equal(structure_geometry const& structure1,
    structure_geometry const& structure2);

void do_set_operation(
    structure_geometry* result,
    set_operation op,
    structure_geometry const& structure1,
    structure_geometry const& structure2);

// Compute a combination of two or more structures.
// If more than two structures are specified, the operator is applied in a
// left-associative manner.
api(fun disk_cached)
// Combined structure
structure_geometry structure_combination(
    // Enum definition of which type of combine operation to perform
    set_operation op,
    // List of structures
    std::vector<structure_geometry> const& structures);

std::vector<double>
slice_position_list(structure_geometry const& structure);

void expand_in_2d(
    structure_geometry* result,
    structure_geometry const& structure,
    double amount);

// Compute the 2D expansion of a structure.
// The 2D expansion of a structure is computed by independently expanding each
// slice of the structure within its 2D plane.
// Note that if the expansion amount is negative, then this is a contraction.
api(fun disk_cached)
// Expanded 2d structure
structure_geometry structure_2d_expansion(
    // 2D Structure to expand
    structure_geometry const& structure,
    // Expansion amount; Positive values will expand the polyset and negative
    // numbers will contract the polyset
    double amount)
{
    structure_geometry result;
    expand_in_2d(&result, structure, amount);
    return result;
}

void expand_in_3d(
    structure_geometry* result,
    structure_geometry const& structure,
    double amount);

// Compute the 3D expansion of a structure_geometry.
// When computing the 3D expansion of a structure, the structure's slices are
// allowed to expand into other slices. Because of this, you must ensure the structures
// master_slice_list is the list of all slices in the structure's space.
// Again, if the expansion amount is negative, this computes a contraction.
api(fun disk_cached)
// Expanded 3D structure
structure_geometry structure_3d_expansion(
    // 3D structure to expand
    structure_geometry const& structure,
    // Expansion amount; Positive values will expand the polyset and negative
    // numbers will contract the polyset
    double amount)
{
    structure_geometry result;
    expand_in_3d(&result, structure, amount);
    return result;
}

// Get the bounding box of a structure.
api(fun disk_cached name(structure_bounding_box))
// Bounding 3d box of the specified structure
box<3,double> bounding_box(
    // 3D Structure to compute the bounding box of
    structure_geometry const& structure);

void compute_bounding_box(
    optional<box<3,double> >& box,
    structure_geometry const& structure);

// Splits a structure by the given plane.
// Note that the function returns the portion of the split structure that lies to the negative side of the plane
// (i.e. removes portion of the structure in the direction of the planes' normal).
// The structure slices are assumed to be positioned along the z axis.
api(fun disk_cached)
// Resulting split structure.
structure_geometry split_structure(
    // Structure to be split.
    structure_geometry const& structure,
    // Normal of the plane which is to split the structure.
    vector3d normal,
    // Point of the plane which is to split the structure.
    vector3d point);


// Test if a box and structure are overlapping.
bool
overlapping(
    box3d const& box,
    structure_geometry const& sg,
    unsigned structure_axis,
    optional<box3d> const& sg_bounds);

// Slices a structure along the given axis.
api(fun)
// The polyset resulting from slicing the structure.
polyset slice_structure(
    // The structure to be sliced.
    structure_geometry const& structure,
    // The axis of the plane used to slice the structure.
    unsigned slice_axis,
    // The position of the slice plane along its axis.
    double slice_position);

// Returns a structure that is the input structure sliced along a different axis
api(fun)
// The input structure sliced along a different axis
structure_geometry
slice_structure_along_different_axis(
    // The structure to be sliced.
    structure_geometry const& structure,
    // The slice axis of the new structure.
    unsigned new_axis,
    // The list of slice positions for the new structure
    std::vector<double> const& slice_positions);

}

#endif
