/*
 * Author(s):  Salvadore Gerace <sgerace@dotdecimal.com>
 *             Thomas Madden <tmadden@mgh.harvard.edu>
 * Date:       03/27/2013
 *
 * Copyright:
 * This work was developed as a joint effort between .decimal, Inc. and
 * Partners HealthCare under research agreement A213686; as such, it is
 * jointly copyrighted by the participating organizations.
 * (c) 2013 .decimal, Inc. All rights reserved.
 * (c) 2013 Partners HealthCare. All rights reserved.
 */

#ifndef CRADLE_GEOMETRY_MESHING_HPP
#define CRADLE_GEOMETRY_MESHING_HPP

#include <cradle/geometry/bin_collection.hpp>
#include <cradle/geometry/common.hpp>
#include <cradle/geometry/clipper.hpp>

#include <cradle/geometry/transformations.hpp>

#include <cradle/imaging/image.hpp>
#include <cradle/io/forward.hpp>
#include <cradle/io/file.hpp>

#include <istream>
#include <vector>

#include <clipper.hpp>

namespace cradle {

typedef vector<3, double> vertex3;

typedef vector<3, int> face3;

typedef array<vertex3> vertex3_array;

typedef array<face3> face3_array;

typedef triangle<3, double> triangle3d;

// Triangle_mesh represents a set of triangular faces and vertices.
api(struct)
struct triangle_mesh
{
    // Array of vertices that compose the triangular mesh (3d)
    vertex3_array vertices;
    // Array of faces that compose the triangular mesh (3i)
    face3_array faces;
};

// Triangle_mesh with normals
api(struct)
struct triangle_mesh_with_normals
{
    // Array of vertex positions within the triangle mesh (3d)
    vertex3_array vertex_positions;
    // Array of vertex normals within the triangle mesh (3d)
    vertex3_array vertex_normals;
    // Array of face positions within the triangle mesh (3i)
    face3_array face_position_indices;
    // Array of face normal direction within the triangle mesh (3i)
    face3_array face_normal_indices;
};

typedef bin_collection<unsigned,3,double> face3_bin_collection;

// A triangle_mesh along with a bin collection of the faces to speed searching.
api(struct)
struct optimized_triangle_mesh
{
    // A triangle mesh.
    triangle_mesh mesh;
    // Bin collection containing all faces of mesh.
    face3_bin_collection bin_collection;
};

// Creates a triangle mesh representing a 3D box
api(fun)
// Mesh for the box (with 2 triangles per face)
triangle_mesh make_cube(
    // The start point of the box (smallest value position in each axis direction)
    vertex3 const& origin,
    // The end point of the box (largest value position in each axis direction)
    vertex3 const& extent);

// Creates a triangle mesh representing an axis aligned, right 3D cylinder
api(fun)
// Mesh for the cylinder
triangle_mesh make_cylinder(
    // The center point of the bottom face
    vector3d const& base,
    // The cylinder radius (in base plane)
    double radius,
    // The cylinder height (in axis direction)
    double height,
    // The number of cells on each circular face of the cylinder (min 32 recommended, 8 min allowed)
    int resolution,
    // The coordinate direction for the cylinder axis (X:0,Y:1,Z:2)
    unsigned axis_direction);

// Creates a triangle mesh representing a 3D sphere
api(fun)
// Mesh for the sphere
triangle_mesh make_sphere(
    // The center point of the sphere
    vector3d const& center,
    // The sphere radius
    double radius,
    // The number of edges along each circle (minimum 8)
    int theta_count,
    // The number of levels along the z axis (minimum 8)
    int phi_count);

// Creates a triangle mesh representing a rectangular based, right 3D pyramid
api(fun)
// Mesh for the pyramid
triangle_mesh make_pyramid(
    // The center point of the bottom face of the pyramid
    vector3d const& base,
    // The base size (in direction 1)
    double width,
    // The base size (in direction 2)
    double length,
    // The pyramid height (in axis direction)
    double height,
    // The coordinate direction for the pyramid axis (X:0,Y:1,Z:2)
    unsigned axis_direction);

// Creates a triangle mesh representing a generalized 3D parallelepiped
api(fun)
// Mesh for the parallelepiped (with 2 triangles per face)
triangle_mesh make_parallelepiped(
    // The start point of the parallelepiped on the base face
    vector3d const& corner,
    // Position of second point on the base face, relative to corner
    vector3d const& a,
    // Position of the third point on the base face, relative to corner (note: fourth point on base is at a+b)
    vector3d const& b,
    // Position of the start point of the top face, relative to corner (note: top face is equal in size and parallel to base)
    vector3d const& c);

// Computes a triangle mesh from the structure
api(fun disk_cached)
// Triangulated mesh
triangle_mesh
compute_triangle_mesh_from_structure_with_options(
    // Structure to compute into triangulated mesh
    structure_geometry const& structure,
    // Minimum grid spacing
    double min_spacing,
    // Maximum number of grid cells
    int max_grid_count);

// Computes a triangle mesh from the structure
api(fun disk_cached)
// Triangulated mesh
triangle_mesh
compute_triangle_mesh_from_structure(
    // Structure to compute into triangulated mesh
    structure_geometry const& structure);

// Creates a surface mesh from the image data at the specified iso-level of value
// Note: End planes (top/bottom) will be returned for external surfaces
api(fun disk_cached)
// Triangulated mesh
triangle_mesh
compute_triangle_mesh_from_image_double(
    // Image to compute triangle mesh from
    image<3,double,shared> const& img,
    // Value of iso-level to create structure frmo image pixel values
    double level);

// Creates a surface mesh from the image data at the specified iso-level of value
// Note: End planes (top/bottom) will be returned for external surfaces
api(fun disk_cached)
// Triangulated mesh
triangle_mesh
compute_triangle_mesh_from_image_float(
    // Image to compute triangle mesh from
    image<3,float,shared> const& img,
    // Value of iso-level to create structure frmo image pixel values
    double level);

image<3,float,shared>
set_data_for_structure(
    image<3, float, shared> const& img, structure_geometry const& structure,
    float threshold, bool set_data_inside);

image<3,double,shared>
set_data_for_structure(
    image<3, double, shared> const& img, structure_geometry const& structure,
    double threshold, bool set_data_inside);

// Constructs a 3D image such that all pixels that are inside/outside of meshes and have a value from img above threshold
// are given a value of 1.0, while all other pixels are set to zero (inside/outside is set by argument set_data_inside).
// Used for creating a clean image which can be used for further processing.
api(fun disk_cached)
// Image of 1 and 0 values.
image<3,float,shared>
set_data_for_mesh_float(
    // Image containing data to compare to threshold.
    image<3, float, shared> const& img,
    // Meshes to be used for checking contains
    std::vector<optimized_triangle_mesh> const& meshes,
    // Threshold value above which points are excluded from resulting image.
    float threshold,
    // Flag indicating if points inside or outside should be set to 1.
    bool set_data_inside);

// Constructs a 3D image such that all pixels that are inside/outside of meshes and have a value from img above threshold
// are given a value of 1.0, while all other pixels are set to zero (inside/outside is set by argument set_data_inside).
// Used for creating a clean image which can be used for further processing.
api(fun disk_cached)
// Image of 1 and 0 values.
image<3,double,shared>
set_data_for_mesh_double(
    // Image containing data to compare to threshold.
    image<3, double, shared> const& img,
    // Meshes to be used for checking contains
    std::vector<optimized_triangle_mesh> const& meshes,
    // Threshold value above which points are excluded from resulting image.
    double threshold,
    // Flag indicating if points inside or outside should be set to 1.
    bool set_data_inside);

// Creates a triangle mesh and constructs a bin collection containing all triangle faces
// for a given structure.
api(fun disk_cached execution_class(cpu.x2))
// The optimized triangle mesh of structure.
optimized_triangle_mesh
make_optimized_triangle_mesh_for_structure(
    // The structure to convert into a mesh.
    structure_geometry const& structure);

// loading .OBJ files
triangle_mesh_with_normals
load_mesh_from_obj(cradle::file_path const& path);

// loading .OBJ files
triangle_mesh_with_normals
load_mesh_from_obj(std::istream & obj);

// convert triangle_mesh_with_normals to a plain old triangle_mesh
api(fun name(remove_mesh_normals))
triangle_mesh
remove_normals(triangle_mesh_with_normals const& orig);

triangle3d
get_triangle(triangle_mesh const& mesh, face3_array::size_type index);

vector3d
get_normal(triangle_mesh const& mesh, face3_array::size_type index);

// calculate the bounds of a triangle mesh
api(fun name(mesh_bounding_box))
// 3D bounding box
box3d bounding_box(
    // Triangulated mesh
    triangle_mesh const& mesh);

// calculate the bounds for a face of a triangle mesh
box3d bounding_box(triangle_mesh const& mesh, face3_array::size_type index);

// project triangle meshes onto a plane.
clipper_polyset
project_triangle_mesh(triangle_mesh const& mesh, plane<double> const& plane,
    vector3d const& up);

// computes the first and last intersections of a segment defined by s1 & s2 with a list of targets in the form of triangle meshes
// pt1 & pt2 are returned points of intersection, first and last respectively
// uu1 & uu2 are returned segment u values at point of intersection, first and last respectively
// returns bool indicating whether or not an intersection was found
bool get_first_last_intersection(vector3d const& s1, vector3d const& s2, std::vector<triangle_mesh> const& targets, vector3d &pt1, vector3d &pt2, double &uu1, double &uu2);

// computes the last intersection of a segment defined by s1 & s2 with a list of targets in the form of triangle meshes
// pt is returned point of intersection
// uu is returned segment u value at point of intersection
// returns bool indicating whether or not an intersection was found
bool get_deepest_intersection(vector3d const& s1, vector3d const& s2, std::vector<triangle_mesh> const& targets, vector3d &pt, double &uu);

// Make a bin collection of the faces in a triangle mesh.
face3_bin_collection
make_bin_collection_from_mesh(
    triangle_mesh const& mesh, unsigned resolution_multiplier = 1);

// Does the given mesh contain the given point?
bool mesh_contains(optimized_triangle_mesh const& mesh, vector3d const& point);

double compute_solid_angle(triangle_mesh const& mesh, vector3d const& p);

// Transform the vertices in a triangle mesh.
api(fun)
triangle_mesh
transform_triangle_mesh(
    triangle_mesh const& original, matrix<4,4,double> const& matrix);

// Transform a triangle mesh with normals.
api(fun name(transform_triangle_mesh_with_normals))
triangle_mesh_with_normals
transform_triangle_mesh(
    triangle_mesh_with_normals const& original,
    matrix<4,4,double> const& matrix);

}

#endif
