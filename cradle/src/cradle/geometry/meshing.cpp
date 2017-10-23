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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cradle/geometry/meshing.hpp>

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/distance.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/geometry/intersection.hpp>
#include <cradle/geometry/polygonal.hpp>

#include <cradle/imaging/image.hpp>
#include <cradle/imaging/geometry.hpp>

#include <boost/compressed_pair.hpp>

#include <fstream>

//#include <cradle/io/stereolithography_io.hpp>

namespace cradle {

// amount to scale our geometry by to make it sized correctly for clipper
static const double geometry_scale_factor = 1.0 / cradle::clipper_integer_precision;

triangle_mesh make_cube(
    vertex3 const& origin,
    vertex3 const& extent)
{
    triangle_mesh mesh;

    auto vertices = allocate(&mesh.vertices, 8);
    vertices[0] = make_vector(origin[0], origin[1], origin[2]);
    vertices[1] = make_vector(extent[0], origin[1], origin[2]);
    vertices[2] = make_vector(origin[0], extent[1], origin[2]);
    vertices[3] = make_vector(extent[0], extent[1], origin[2]);
    vertices[4] = make_vector(origin[0], origin[1], extent[2]);
    vertices[5] = make_vector(extent[0], origin[1], extent[2]);
    vertices[6] = make_vector(origin[0], extent[1], extent[2]);
    vertices[7] = make_vector(extent[0], extent[1], extent[2]);

    auto faces = allocate(&mesh.faces, 12);
    faces[0] = make_vector(0, 3, 1);
    faces[1] = make_vector(0, 2, 3);
    faces[2] = make_vector(0, 1, 5);
    faces[3] = make_vector(0, 5, 4);
    faces[4] = make_vector(0, 4, 2);
    faces[5] = make_vector(2, 4, 6);
    faces[6] = make_vector(4, 5, 6);
    faces[7] = make_vector(5, 7, 6);
    faces[8] = make_vector(2, 6, 3);
    faces[9] = make_vector(3, 6, 7);
    faces[10] = make_vector(1, 3, 5);
    faces[11] = make_vector(3, 7, 5);

    return mesh;
}

triangle_mesh make_cylinder(vector3d const& base, double radius, double height, int resolution, unsigned axis_direction)
{
    triangle_mesh mesh;

    unsigned direction = 2;
    if (axis_direction < 3) { direction = axis_direction; }
    vector3i ordinates;
    if (direction == 0) { ordinates = make_vector(1, 2, 0); }
    if (direction == 1) { ordinates = make_vector(2, 0, 1); }
    if (direction == 2) { ordinates = make_vector(0, 1, 2); }

    int k = 8;
    if (resolution > k) { k = resolution; }
    vector3d axis = make_vector(0.0, 0.0, 0.0);
    axis[direction] = height;
    double delta = (2.0 * pi) / k;
    auto vertices = allocate(&mesh.vertices, 2 * k + 2);
    for (int i = 0; i < k; ++i)
    {
        vector3d v = make_vector(0., 0., 0.);
        v[ordinates[0]] = radius * std::cos(i * delta);
        v[ordinates[1]] = radius * std::sin(i * delta);
        vertices[i] = base + v;
        v[ordinates[2]] = height;
        vertices[i + k + 1] = base + v;
    }
    vertices[k] = base;
    vertices[2 * k + 1] = base + axis;

    auto faces = allocate(&mesh.faces, 4 * k);
    for (int i = 0; i < k; ++i)
    {
        faces[i] = make_vector(i, k, i + 1);
        faces[i + k] = make_vector(i + k + 2, 2 * k + 1, i + k + 1);
    }
    faces[k - 1] = make_vector(k - 1, k, 0);
    faces[2 * k - 1] = make_vector(k + 1, 2 * k + 1, 2 * k);
    for (int i = 0; i < k; ++i)
    {
        faces[2 * i + 2 * k] = make_vector(i, i + 1, i + k + 1);
        faces[2 * i + 2 * k + 1] = make_vector(i + 1, i + k + 2, i + k + 1);
    }
    faces[4 * k - 2] = make_vector(k - 1, 0, 2 * k);
    faces[4 * k - 1] = make_vector(0, k + 1, 2 * k);

    return mesh;
};

triangle_mesh make_sphere(vector3d const& center, double radius, int theta_count, int phi_count)
{
    triangle_mesh mesh;

    int kt = 8;
    if (theta_count > kt) { kt = theta_count; }

    int kp = 8;
    if (phi_count > kp) { kp = phi_count; }

    double deltat = (2.0 * pi) / kt;
    double deltap = pi / (double(kp) - 1.0);
    auto vertices = allocate(&mesh.vertices, kt * (kp - 2) + 2);
    auto faces = allocate(&mesh.faces, (kp - 2) * 2 * kt);

    int k = 1;
    int f = 0;
    vertices[0] = center - make_vector(0., 0., radius);
    int kL = 0;
    for (int j = 1; j < kp-1; ++j)
    {
        double z = center[2] - radius * std::cos(j * deltap);
        vector3d v = make_vector(0., 0., z);
        double rL = radius * std::sin(j * deltap);
        for (int i = 0; i < kt; ++i)
        {
            // Make next batch of vertices
            v[0] = center[0] + rL * std::cos(i * deltat);
            v[1] = center[1] + rL * std::sin(i * deltat);
            vertices[k+i] = v;
        }

        if (j == 1)
        {
            // First level
            for (int i = 0; i < kt-1; ++i)
            {
                faces[f++] = make_vector(k + i, k + i + 1, 0);
            }
            faces[f++] = make_vector(k + kt - 1, k, 0);
        }
        else
        {
            // All other levels
            kL = k - kt;
            for (int i = 0; i < kt-1; ++i)
            {
                faces[f++] = make_vector(kL + i + 1, kL + i, k + i);
                faces[f++] = make_vector(k + i, k + i + 1, kL + i + 1);
            }
            faces[f++] = make_vector(kL, kL + kt - 1, k + kt - 1);
            faces[f++] = make_vector(k + kt - 1, k, kL);
        }
        k += kt;
    }
    // Last Level
    vertices[k] = center + make_vector(0., 0., radius);
    kL = k - kt;
    for (int i = 0; i < kt-1; ++i)
    {
        faces[f++] = make_vector(kL + i + 1, kL + i, k);
    }
    faces[f++] = make_vector(kL + kt - 1, kL, k);
    //std::cout << "Faces: " << f << " Edges: " << k+1 << std::endl;
    return mesh;
}

triangle_mesh make_pyramid(vector3d const& base, double width, double length, double height, unsigned axis_direction)
{
    triangle_mesh mesh;

    unsigned direction = 2;
    if (axis_direction < 3) { direction = axis_direction; }
    vector3i ordinates;
    if (direction == 0) { ordinates = make_vector(1, 2, 0); }
    if (direction == 1) { ordinates = make_vector(2, 0, 1); }
    if (direction == 2) { ordinates = make_vector(0, 1, 2); }

    vector3d axis = make_vector(0.0, 0.0, 0.0);
    axis[direction] = height;

    auto vertices = allocate(&mesh.vertices, 5);


    vector3d v = base;
    v[ordinates[0]] -= 0.5 * width;
    v[ordinates[1]] -= 0.5 * length;
    vertices[0] = v;
    v[ordinates[0]] += width;
    vertices[1] = v;
    v[ordinates[1]] += length;
    vertices[2] = v;
    v[ordinates[0]] -= width;
    vertices[3] = v;
    v = base;
    v[ordinates[2]] += height;
    vertices[4] = v;

    auto faces = allocate(&mesh.faces, 6);
    faces[0] = make_vector(0, 1, 2);
    faces[1] = make_vector(0, 2, 3);
    faces[2] = make_vector(0, 4, 1);
    faces[3] = make_vector(1, 4, 2);
    faces[4] = make_vector(2, 4, 3);
    faces[5] = make_vector(3, 4, 0);

    return mesh;
}

triangle_mesh make_parallelepiped(vector3d const& corner, vector3d const& a, vector3d const& b, vector3d const& c)
{
    triangle_mesh mesh;

    auto vertices = allocate(&mesh.vertices, 8);
    vertices[0] = corner;
    vertices[1] = corner + a;
    vertices[2] = corner + b;
    vertices[3] = corner + a + b;
    vertices[4] = corner + c;
    vertices[5] = corner + a + c;
    vertices[6] = corner + b + c;
    vertices[7] = corner + a + b + c;

    auto faces = allocate(&mesh.faces, 12);
    faces[0] = make_vector(0, 3, 1);
    faces[1] = make_vector(0, 2, 3);
    faces[2] = make_vector(0, 1, 5);
    faces[3] = make_vector(0, 5, 4);
    faces[4] = make_vector(0, 4, 2);
    faces[5] = make_vector(2, 4, 6);
    faces[6] = make_vector(4, 5, 6);
    faces[7] = make_vector(5, 7, 6);
    faces[8] = make_vector(2, 6, 3);
    faces[9] = make_vector(3, 6, 7);
    faces[10] = make_vector(1, 3, 5);
    faces[11] = make_vector(3, 7, 5);

    return mesh;
};

typedef boost::compressed_pair<unsigned char, unsigned char> cpair;

typedef line_segment<2, double> line_segment2;



double interpolate_value(double ss, double tol, double ptb, double a, double b)
{
    double dsq = (b * b + ss);
    if (dsq < a * a)
    {
        return ((a < 0) ? 1.0 : -1.0) * std::sqrt(dsq);
        //return -std::sqrt(dsq);
    }
    else
    {
        return std::fabs(a) > tol ? -a : -a + ptb;
    }
}

vertex3 interpolate_position(vector3d const& origin, vector3d const& extent, int c, int r, double a, double b)
{
    double u = origin[c] - a * (extent[c] - origin[c]) / (b - a);
    switch (c)
    {
        case 0:
        {
            switch (r)
            {
                case 0: { return make_vector(u, origin[1], origin[2]); }
                case 1: { return make_vector(u, extent[1], origin[2]); }
                case 2: { return make_vector(u, origin[1], extent[2]); }
                case 3: { return make_vector(u, extent[1], extent[2]); }
                default: { return vertex3(); }
            }
            break;
        }
        case 1:
        {
            switch (r)
            {
                case 0: { return make_vector(origin[0], u, origin[2]); }
                case 1: { return make_vector(extent[0], u, origin[2]); }
                case 2: { return make_vector(origin[0], u, extent[2]); }
                case 3: { return make_vector(extent[0], u, extent[2]); }
                default: { return vertex3(); }
            }
            break;
        }
        case 2:
        {
            switch (r)
            {
                case 0: { return make_vector(origin[0], origin[1], u); }
                case 1: { return make_vector(extent[0], origin[1], u); }
                case 2: { return make_vector(origin[0], extent[1], u); }
                case 3: { return make_vector(extent[0], extent[1], u); }
                default: { return vertex3(); }
            }
            break;
        }
        default: { return vertex3(); }
    }
}

typedef std::vector<vertex3> growable_vertex3_array;

typedef std::vector<face3> growable_face3_array;

api(struct)
struct growable_triangle_mesh
{
    growable_vertex3_array vertices;
    growable_face3_array faces;
};

template<class T>
void vector_to_array(array<T>* array, std::vector<T> const& vector)
{
    size_t size = vector.size();
    if (size != 0)
    {
        auto p = allocate(array, size);
        memcpy(p, &vector[0], sizeof(T) * size);
    }
    else
    {
        clear(array);
    }
}

static triangle_mesh
collapse_mesh(growable_triangle_mesh const& growable)
{
    triangle_mesh mesh;
    vector_to_array(&mesh.vertices, growable.vertices);
    vector_to_array(&mesh.faces, growable.faces);
    return mesh;
}

typedef std::vector<vertex3> growable_vertex3_array;

typedef std::vector<face3> growable_face3_array;

api(struct)
struct growable_triangle_mesh_with_normals
{
    growable_vertex3_array vertex_positions;
    growable_vertex3_array vertex_normals;
    growable_face3_array face_position_indices;
    growable_face3_array face_normal_indices;
};

static triangle_mesh_with_normals
collapse_mesh(growable_triangle_mesh_with_normals const& growable)
{
    triangle_mesh_with_normals mesh;
    vector_to_array(&mesh.vertex_positions, growable.vertex_positions);
    vector_to_array(&mesh.vertex_normals, growable.vertex_normals);
    vector_to_array(&mesh.face_position_indices,
        growable.face_position_indices);
    vector_to_array(&mesh.face_normal_indices, growable.face_normal_indices);
    return mesh;
}

image<3, float, shared> set_data_for_structure(image<3, float, shared> const& img, structure_geometry const& structure, float threshold, bool setDataInside)
{
    image<3, float, unique> tmp;
    create_image(tmp, img.size);
    set_spatial_mapping(tmp, img.origin, make_vector(img.axes[0][0], img.axes[1][1], img.axes[2][2]));
    set_value_mapping(tmp, img.value_mapping.intercept, img.value_mapping.slope, img.units);

    auto image_const_view = as_const_view(img);
    unsigned kk = 0;
    for (unsigned int k = 0; k < img.size[2]; ++k)
    {
    double z = img.origin[2] + img.axes[2][2] * k;
    for (unsigned int j = 0; j < img.size[1]; ++j)
    {
        double y = img.origin[1] + img.axes[1][1] * j;
        for (unsigned int i = 0; i < img.size[0]; ++i)
        {
        double x = img.origin[0] + img.axes[0][0] * i;
        if (is_inside(structure, make_vector(x, y, z)) == setDataInside)
        {
            tmp.pixels.ptr[kk] = (image_const_view.pixels[kk] > threshold) ? 0.0f : 1.0f;
        }
        else
        {
            tmp.pixels.ptr[kk] = 0.0f;
        }
        ++kk;
        }
    }
    }
    return share(tmp);
}

image<3, double, shared> set_data_for_structure(image<3, double, shared> const& img, structure_geometry const& structure, double threshold, bool setDataInside)
{
    image<3, double, unique> tmp;
    create_image(tmp, img.size);
    set_spatial_mapping(tmp, img.origin, make_vector(img.axes[0][0], img.axes[1][1], img.axes[2][2]));
    set_value_mapping(tmp, img.value_mapping.intercept, img.value_mapping.slope, img.units);

    auto image_const_view = as_const_view(img);
    unsigned kk = 0;
    for (unsigned int k = 0; k < img.size[2]; ++k)
    {
        double z = img.origin[2] + img.axes[2][2] * k;
        for (unsigned int j = 0; j < img.size[1]; ++j)
        {
            double y = img.origin[1] + img.axes[1][1] * j;
            for (unsigned int i = 0; i < img.size[0]; ++i)
            {
                double x = img.origin[0] + img.axes[0][0] * i;
                if (is_inside(structure, make_vector(x, y, z)) == setDataInside)
                {
                    tmp.pixels.ptr[kk] = (image_const_view.pixels[kk] > threshold) ? 0.0 : 1.0;
                }
                else
                {
                    tmp.pixels.ptr[kk] = 0.0;
                }
                ++kk;
            }
        }
    }
    return share(tmp);
}

image<3, float, shared> set_data_for_mesh_float(image<3, float, shared> const& img, std::vector<optimized_triangle_mesh> const& meshes, float threshold, bool setDataInside)
{
    image<3, float, unique> tmp;
    create_image(tmp, img.size);
    set_spatial_mapping(tmp, img.origin, make_vector(img.axes[0][0], img.axes[1][1], img.axes[2][2]));
    set_value_mapping(tmp, img.value_mapping.intercept, img.value_mapping.slope, img.units);
    //optional<box3d> mesh_bounds;
    //for (size_t i = 0; i < meshes.size(); ++i)
    //{
    //    if (i == 0) { mesh_bounds = bounding_box(meshes.at(0).mesh); }
    //    else { compute_bounding_box(mesh_bounds, bounding_box(meshes.at(i).mesh)); }
    //}

    auto image_const_view = as_const_view(img);
    unsigned kk = 0;
    for (unsigned int k = 0; k < img.size[2]; ++k)
    {
        double z = img.origin[2] + img.axes[2][2] * k;
        for (unsigned int j = 0; j < img.size[1]; ++j)
        {
            double y = img.origin[1] + img.axes[1][1] * j;
            for (unsigned int i = 0; i < img.size[0]; ++i)
            {
                double x = img.origin[0] + img.axes[0][0] * i;
                bool isInside = false;
                size_t n = 0;
                while (!isInside && n < meshes.size()) { isInside = mesh_contains(meshes.at(n++), make_vector(x, y, z)); }
                if (isInside == setDataInside)
                {
                    tmp.pixels.ptr[kk] = (image_const_view.pixels[kk] > threshold) ? 0.0f : 1.0f;
                }
                else
                {
                    tmp.pixels.ptr[kk] = 0.0f;
                }
                ++kk;
            }
        }
    }
    return share(tmp);
}

image<3, double, shared> set_data_for_mesh_double(image<3, double, shared> const& img, std::vector<optimized_triangle_mesh> const& meshes, double threshold, bool setDataInside)
{
    image<3, double, unique> tmp;
    create_image(tmp, img.size);
    set_spatial_mapping(tmp, img.origin, make_vector(img.axes[0][0], img.axes[1][1], img.axes[2][2]));
    set_value_mapping(tmp, img.value_mapping.intercept, img.value_mapping.slope, img.units);
    //optional<box3d> mesh_bounds;
    //for (size_t i = 0; i < meshes.size(); ++i)
    //{
    //    if (i == 0) { mesh_bounds = bounding_box(meshes.at(0).mesh); }
    //    else { compute_bounding_box(mesh_bounds, bounding_box(meshes.at(i).mesh)); }
    //}

    auto image_const_view = as_const_view(img);
    unsigned kk = 0;
    for (unsigned int k = 0; k < img.size[2]; ++k)
    {
        double z = img.origin[2] + img.axes[2][2] * (k + 0.5);
        for (unsigned int j = 0; j < img.size[1]; ++j)
        {
            double y = img.origin[1] + img.axes[1][1] * (j + 0.5);
            for (unsigned int i = 0; i < img.size[0]; ++i)
            {
                double x = img.origin[0] + img.axes[0][0] * (i + 0.5);
                bool isInside = false;
                size_t n = 0;
                while (!isInside && n < meshes.size()) { isInside = mesh_contains(meshes.at(n++), make_vector(x, y, z)); }
                if (isInside == setDataInside)
                {
                    tmp.pixels.ptr[kk] = (image_const_view.pixels[kk] > threshold) ? 0.0 : 1.0;
                }
                else
                {
                    tmp.pixels.ptr[kk] = 0.0;
                }
                ++kk;
            }
        }
    }
    return share(tmp);
}

optimized_triangle_mesh
make_optimized_triangle_mesh_for_structure(
    structure_geometry const& structure)
{
    triangle_mesh mesh = compute_triangle_mesh_from_structure(structure);
    return optimized_triangle_mesh(mesh, make_bin_collection_from_mesh(mesh));
}

// loading .OBJ files
triangle_mesh_with_normals load_mesh_from_obj(cradle::file_path const& path)
{
    triangle_mesh_with_normals mesh;
    std::ifstream f;
    f.open(path.string(), std::ios::in);
    if (!f)
    {
        throw new cradle::file_error(path, "unable to open OBJ file");
        return mesh;
    }
    mesh = load_mesh_from_obj(f);
    f.close();
    return mesh;
}

// loading .OBJ files
triangle_mesh_with_normals load_mesh_from_obj(std::istream & obj)
{
    growable_triangle_mesh_with_normals mesh;

    std::string line;
    while (std::getline(obj, line))
    {
        boost::trim_right(line);
        if (line.c_str()[0] == '#')
        {
            // skip comments
            continue;
        }

        std::vector<string> tokens;
        boost::split(tokens, line, boost::is_space(), boost::token_compress_on);

        if (tokens.size())
        {
            if ("v" == tokens[0])
            {
                if (tokens.size() != 4)
                {
                    throw cradle::exception("Error in OBJ format");
                }
                mesh.vertex_positions.push_back(
                    make_vector(
                        atof(tokens[1].c_str()),
                        atof(tokens[2].c_str()),
                        atof(tokens[3].c_str())));
            }
            else if ("vn" == tokens[0])
            {
                if (tokens.size() != 4)
                {
                    throw cradle::exception("Error in OBJ format");
                }
                mesh.vertex_normals.push_back(
                    make_vector(
                        atof(tokens[1].c_str()),
                        atof(tokens[2].c_str()),
                        atof(tokens[3].c_str())));
            }
            else if ("f" == tokens[0])
            {
                if (tokens.size() != 4)
                {
                    throw new cradle::exception("Error in OBJ format");
                }

                face3 face_p;
                face3 face_n = make_vector<int>(-1, -1, -1);
                for (int i=1; i<4; ++i)
                {
                    if (tokens[i].find('/'))
                    {
                        std::vector<string> vertex_triplet;
                        boost::split(vertex_triplet, tokens[i], boost::is_any_of("/"));
                        if (vertex_triplet.size() != 3)
                        {
                            throw new cradle::exception("Error in OBJ format");
                        }
                        face_p[i-1] = atoi(vertex_triplet[0].c_str()) - 1;
                        face_n[i-1] = atoi(vertex_triplet[2].c_str()) - 1;
                    }
                    else
                    {
                        face_p[i-1] = atoi(tokens[i].c_str()) - 1;
                    }
                }
                mesh.face_position_indices.push_back(face_p);
                mesh.face_normal_indices.push_back(face_n);
            }
            // ignore other kinds of lines
        }
    }

    return collapse_mesh(mesh);
}

// convert triangle_mesh_with_normals to a plain old triangle_mesh
triangle_mesh remove_normals(triangle_mesh_with_normals const& orig)
{
    triangle_mesh mesh;
    mesh.vertices = orig.vertex_positions;
    mesh.faces = orig.face_position_indices;
    return mesh;
}


triangle3d get_triangle(triangle_mesh const& mesh, face3_array::size_type index)
{
    face3 face = mesh.faces[index];
    return triangle3d(mesh.vertices[face[0]], mesh.vertices[face[1]], mesh.vertices[face[2]]);
}

vector3d get_normal(triangle_mesh const& mesh, face3_array::size_type index)
{
    face3 face = mesh.faces[index];
    vertex3 v0 = mesh.vertices[face[0]];
    return unit(cross(mesh.vertices[face[1]] - v0, mesh.vertices[face[2]] - v0));
}
// calculate the bounds of a triangle mesh
box3d bounding_box(triangle_mesh const& mesh)
{
    if (mesh.vertices.size() < 1) { return cradle::box<3,double>(); }

    vertex3_array::const_iterator iter = mesh.vertices.begin();
    vector3d mins = (*iter);
    vector3d maxs = (*iter);
    ++iter;
    vertex3_array::const_iterator vertices_end = mesh.vertices.end();
    for ( ; iter != vertices_end; ++iter)
    {
        for (int i = 0; i < 3; ++i)
        {
            if ((*iter)[i] < mins[i]) mins[i] = (*iter)[i];
            if ((*iter)[i] > maxs[i]) maxs[i] = (*iter)[i];
        }
    }
    return cradle::box<3,double>(mins, maxs - mins);
}

// calculate the bounds for a face of a triangle mesh
box3d bounding_box(triangle_mesh const& mesh, face3_array::size_type index)
{
    return bounding_box(get_triangle(mesh, index));
}

struct sum
{
    triangle_mesh const& mesh;
    line_segment<3, double> const& segment;

    sum(triangle_mesh const& m, line_segment<3, double> const& s)
        : mesh(m), segment(s) {}

    bool operator () (unsigned const& index, int *value) {
        segment_triangle_intersection_type type = is_intersecting(segment, get_triangle(mesh, index));
        if (type == segment_triangle_intersection_type::NONE) {
            *value = 0;
            return true;
        } else if (type == segment_triangle_intersection_type::FACE) {
            *value = 1;
            return true;
        } else {
            return false;
        }
    }
};


typedef std::pair<int, int> edge;

edge make_edge(int a, int b)
{
    if (a > b) { std::swap(a, b); }
    return edge(a, b);
}

struct edge_state
{
    unsigned char state;

    edge_state()
        : state(0) { }

    void update(bool visible)
    {
        if (visible)
        {
            ++state;
        }
        else
        {
            --state;
        }
    }
};

bool is_coincident(clipper_point const& p1, clipper_point const& p2)
{
    return p1.X == p2.X && p1.Y == p2.Y;
}

typedef std::map<edge, edge_state> edge_map;

void erase_connectivity(unsigned short * connectivity_counts, int ** connectivity, int i1, int i2)
{
    // Erase connectivity for first index
    int count1 = connectivity_counts[i1];
    int *c1 = connectivity[i1];
    for (int i = 0; i < count1; ++i)
    {
        if (*c1 == i2) { *c1 = -1; break; }
        ++c1;
    }

    // Erase connectivity for second index
    int count2 = connectivity_counts[i2];
    int *c2 = connectivity[i2];
    for (int i = 0; i < count2; ++i)
    {
        if (*c2 == i1) { *c2 = -1; break; }
        ++c2;
    }
}


vector2d point_to_plane(plane<double> const& pl, vector3d const& pt)
{
    // Compute reference vector (shortcut)
    vector3d reference = cross(pl.normal, make_vector(0.0, 0.0, 1.0));
    if (length2(reference) < 1.0e-20)
    {
        reference = cross(pl.normal, make_vector(1.0, 0.0, 0.0));
    }
    reference = reference / length(reference);
    vector3d plU = reference - (dot(reference, pl.normal) * pl.normal);
    vector3d plV = cross(pl.normal, plU);
    vector3d v = pt - pl.point;
    return make_vector<double>(dot(v, plU), dot(v, plV));
}

bool triangle_segment_intersection(vector3d const& s1, vector3d const& s2, triangle<3, double> const& t, double &u)
{
    vector3d normal = cross(t[1] - t[0], t[2] - t[0]);

    plane<double> pl(t[1], normal);

    double dist1 = dot(s1 - pl.point, pl.normal);
    double dist2 = dot(s2 - pl.point, pl.normal);
    bool intersects = (dist1 * dist2 < 0);
    if (!intersects) return false;

    vector2d vp1 = point_to_plane(pl, t[0]);
    vector2d vp2 = point_to_plane(pl, t[1]);
    vector2d vp3 = point_to_plane(pl, t[2]);

    u = std::fabs(dist1) / (std::fabs(dist1) + std::fabs(dist2) + 1.0e-20);
    vector2d vp = point_to_plane(pl, point_along(s1, s2, u));

    vector2d d1 = vp2 - vp1;
    vector2d d2 = vp3 - vp1;

    double denom = cross(d1, d2);
    if (denom == 0.0) { return false; }

    double numer1 = cross(vp, d2) - cross(vp1, d2);
    double a = numer1 / denom;
    if (a < -1.0e-12) return false;

    double numer2 = cross(vp1, d1) - cross(vp, d1);
    double b = numer2 / denom;
    if (b < -1.0e-12) return false; // I believe there may be a problem in this computation somewhere - Sal (02.09.2009)

    double sum = a + b;
    if (sum > (1.0 + 1.0e-12)) return false;

    return true;
}

bool get_first_last_intersection(vector3d const& s1, vector3d const& s2, std::vector<triangle_mesh> const& targets, vector3d &pt1, vector3d &pt2, double &uu1, double &uu2)
{
    double u1 = 1.0e100;
    double u2 = -1.0e100;
    for (size_t i = 0; i < targets.size(); ++i)
    {
        for (size_t j = 0; j < targets[i].faces.size(); ++j)
        {
            double temp_u = 0.0;
            bool isIntersected = triangle_segment_intersection(s1, s2, get_triangle(targets[i], j), temp_u);
            if (isIntersected)
            {
                if (temp_u < u1) { u1 = temp_u; }
                if (temp_u > u2) { u2 = temp_u; }
            }
            //if (isIntersected) std::cout << "Intersect: " << temp_u << " " << u << std::endl;
        }
    }
    if (u2 > 0.0)
    {
        pt1 = point_along(s1, s2, u1);
        pt2 = point_along(s1, s2, u2);
        uu1 = u1;
        uu2 = u2;
        return true;
    }
    return false;
}

bool get_deepest_intersection(vector3d const& s1, vector3d const& s2, std::vector<triangle_mesh> const& targets, vector3d &pt, double &uu)
{
    double u = 0.0;
    for (size_t i = 0; i < targets.size(); ++i)
    {
        for (size_t j = 0; j < targets[i].faces.size(); ++j)
        {
            double temp_u = 0.0;
            bool isIntersected = triangle_segment_intersection(s1, s2, get_triangle(targets[i], j), temp_u);
            if (isIntersected && (temp_u > u)) { u = temp_u; }
            //if (isIntersected) std::cout << "Intersect: " << temp_u << " " << u << std::endl;
        }
    }
    if (u > 0.0)
    {
        pt = point_along(s1, s2, u);
        uu = u;
        return true;
    }
    return false;
}

double compute_solid_angle(
    triangle_mesh const& mesh,
    vector3d const& p)
{
    double ang = 0.0;
    face3_array::const_iterator end = mesh.faces.end();
    for (face3_array::const_iterator iter = mesh.faces.begin(); iter != end; ++iter)
    {
        // Get triangle
        vector3d a = mesh.vertices[(*iter)[0]] - p;
        vector3d b = mesh.vertices[(*iter)[1]] - p;
        vector3d c = mesh.vertices[(*iter)[2]] - p;

        double alength = length(a);
        double blength = length(b);
        double clength = length(c);

        double numer = dot(a, cross(b, c));
        double denom = (alength * blength * clength) + clength * dot(a, b) + blength * dot(a, c) + alength * dot(b, c);

        ang += std::atan2(numer, denom);
    }
    return 2.0 * ang;
}

// transforms the vertices in a triangle mesh
// (leaves the original alone and creates a transformed COPY)
triangle_mesh
transform_triangle_mesh(const triangle_mesh& original,
    const matrix<4,4,double>& matrix)
{
    triangle_mesh output_mesh;
    output_mesh.faces = original.faces;
    auto n_vertices = original.vertices.size();
    auto output_vertices = allocate(&output_mesh.vertices, n_vertices);
    for (size_t i = 0; i != n_vertices; ++i)
        output_vertices[i] = transform_point(matrix, original.vertices[i]);
    return output_mesh;
}

triangle_mesh_with_normals
transform_triangle_mesh(
    triangle_mesh_with_normals const& original,
    matrix<4,4,double> const& matrix)
{
    triangle_mesh_with_normals output_mesh;

    {
        output_mesh.face_position_indices = original.face_position_indices;
        auto n_vertices = original.vertex_positions.size();
        auto output_vertices =
            allocate(&output_mesh.vertex_positions, n_vertices);
        for (size_t i = 0; i != n_vertices; ++i)
        {
            output_vertices[i] =
                transform_point(matrix, original.vertex_positions[i]);
        }
    }

    {
        output_mesh.face_normal_indices = original.face_normal_indices;
        auto n_normals = original.vertex_normals.size();
        auto output_normals =
            allocate(&output_mesh.vertex_normals, n_normals);
        for (size_t i = 0; i != n_normals; ++i)
        {
            output_normals[i] =
                transform_vector(matrix, original.vertex_normals[i]);
        }
    }

    return output_mesh;
}

}
