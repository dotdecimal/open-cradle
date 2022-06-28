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

#include <cradle/io/vtk_io.hpp>

#include <cstdint>
#include <fstream>

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/distance.hpp>
#include <cradle/geometry/transformations.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/imaging/geometry.hpp>
#include <cradle/imaging/variant.hpp>

#include <typeinfo>

namespace cradle {

void write_vtk_file(
    file_path const& file,
    triangle_mesh const& mesh)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE TRIANGLE MESH\n";
    stream << "ASCII\n";
    stream << "DATASET UNSTRUCTURED_GRID\n";

    // Write vertices to stream
    stream << "POINTS " << mesh.vertices.size() << " double\n";
    vertex3_array::const_iterator vertices_end = mesh.vertices.end();
    for (vertex3_array::const_iterator iter = mesh.vertices.begin(); iter != vertices_end; ++iter)
    {
        stream << (*iter)[0] << " " << (*iter)[1] << " " << (*iter)[2] << "\n";
    }

    // Write faces to stream
    int face_count = int(mesh.faces.size());
    stream << "CELLS " << face_count << " " << (face_count * 4) << "\n";
    face3_array::const_iterator faces_end = mesh.faces.end();
    for (face3_array::const_iterator iter = mesh.faces.begin(); iter != faces_end; ++iter)
    {
        stream << "3 " << (*iter)[0] << " " << (*iter)[1] << " " << (*iter)[2] << "\n";
    }

    // Write cell types to stream
    stream << "CELL_TYPES " << face_count << "\n";
    for (int i = 0; i < face_count; ++i) { stream << "7\n"; }

    // Close stream
    stream.close();
}

void write_vtk_file(
    file_path const& file,
    std::vector<vector3d> const& points)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE TRIANGLE MESH\n";
    stream << "ASCII\n";
    stream << "DATASET UNSTRUCTURED_GRID\n";

    // Write vertices to stream
    stream << "POINTS " << points.size() << " double\n";
    for (size_t iter = 0; iter < points.size(); ++iter)
    {
        stream << points[iter][0] << " " << points[iter][1] << " " << points[iter][2] << "\n";
    }

    // Write vertices to stream
    stream << "CELLS " << points.size() << " " << (points.size() * 2) << "\n";
    for (size_t iter = 0; iter < points.size(); ++iter)
    {
        stream << "1 " << iter << "\n";
    }

    // Write cell types to stream
    stream << "CELL_TYPES " << points.size() << "\n";
    for (size_t i = 0; i < points.size(); ++i) { stream << "1\n"; }

    // Close stream
    stream.close();
}

void write_vtk_file(
    file_path const& file,
    polyset const& poly, double z)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE POLYSET\n";
    stream << "ASCII\n";
    stream << "DATASET POLYDATA\n";

    // Count the points
    size_t polygon_count = poly.polygons.size();
    size_t v_count = 0;
    for (size_t k = 0; k < polygon_count; ++k)
    {
        v_count += poly.polygons[k].vertices.n_elements;
        //auto v0 = poly.polygons[k].vertices.elements[vertex_count - 1];
        //for (size_t i = 0; i < vertex_count; ++i)
        //{
        //    // Get segment end point
        //    auto v1 = poly.polygons[k].vertices.elements[i];
        //    auto delta = v1 - v0;
        //}
    }

    // Write vertices to stream
    stream << "POINTS " << v_count << " double\n";
    string buffer = "POLYGONS " + to_string(polygon_count) + " " + to_string(v_count + polygon_count) + "\n";
    int j = 0;
    for (size_t k = 0; k < polygon_count; ++k)
    {
        size_t vertex_count = poly.polygons[k].vertices.n_elements;
        buffer += to_string(vertex_count);
        for (size_t i = 0; i < vertex_count; ++i)
        {
            // Get segment end point
            auto v1 = poly.polygons[k].vertices.elements[i];
            stream << v1[0] << " " << v1[1] << " " << z << "\n";
            buffer += " " + to_string(j++);
        }
        buffer += "\n";
    }

    // Write polygons indices to stream
    stream << buffer;

    // Close stream
    stream.close();
}

void write_vtk_file(file_path const& file, image3 const& image, string const& data_type)
{
    std::cout << "image data type: " << typeid(image.pixels).name()  << std::endl;
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE3\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };

    // Write rectilinear grid coordinates to stream
    stream << "DIMENSIONS " << image.size[0] + 1 << " " << image.size[1] + 1 << " " << image.size[2] + 1 << "\n";
    for (int k = 0; k < 3; ++k) {
        int n = image.size[k] + 1;
        double origin = image.origin[k];
        double step = image.axes[k][k];
        stream << coords[k] << "_COORDINATES " << n << " double\n";
        for (int i = 0; i < n; ++i) {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    // Write cell data to stream
    int pixel_count = image.size[0] * image.size[1] * image.size[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels double\n";
    stream << "LOOKUP_TABLE default\n";

    if (data_type == string("int"))
    {
            auto image_const_view = as_const_view(cast_variant<int16_t>(image));
            for (int i = 0; i < pixel_count; ++i)
            {
                    stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
            }
    }
    else if (data_type == string("uint"))
    {
        auto image_const_view = as_const_view(cast_variant<uint16_t>(image));
        for (int i = 0; i < pixel_count; ++i)
        {
            stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
        }
    }
    else if (data_type == string("float"))
    {
        auto image_const_view = as_const_view(cast_variant<float>(image));
        for (int i = 0; i < pixel_count; ++i)
        {
            stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
        }
    }
    else if (data_type == string("double"))
    {
        auto image_const_view = as_const_view(cast_variant<double>(image));
        for (int i = 0; i < pixel_count; ++i)
        {
            stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
        }
    }
    else if (data_type == string("ushort"))
    {
        std::cout << "USHORT"  << std::endl;
        auto image_const_view = as_const_view(cast_variant<uint8_t>(image));
        for (int i = 0; i < pixel_count; ++i)
        {
            stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
        }
    }

}

void write_vtk_file(file_path const& file, image3 const& image1, image3 const& image2)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE3\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };

    // Write rectilinear grid coordinates to stream
    stream << "DIMENSIONS " << image1.size[0] + 1 << " " << image1.size[1] + 1 << " " << image1.size[2] + 1 << "\n";
    for (int k = 0; k < 3; ++k) {
        int n = image1.size[k] + 1;
        double origin = image1.origin[k];
        double step = image1.axes[k][k];
        stream << coords[k] << "_COORDINATES " << n << " double\n";
        for (int i = 0; i < n; ++i) {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    // Write cell data to stream
    int pixel_count = image1.size[0] * image1.size[1] * image1.size[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels double\n";
    stream << "LOOKUP_TABLE default\n";
    auto image_const_view1 = as_const_view(cast_variant<double>(image1));
    auto image_const_view2 = as_const_view(cast_variant<double>(image2));
    for (int i = 0; i < pixel_count; ++i) {
        stream << (image_const_view1.pixels[i] * image1.value_mapping.slope + image1.value_mapping.intercept) +
                  (image_const_view2.pixels[i] * image2.value_mapping.slope + image2.value_mapping.intercept) << "\n";
    }
}

void write_vtk_file(file_path const& file, image3 const& image, bool writeMe)
{
    if (!writeMe) return;
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE3\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };

    // Write rectilinear grid coordinates to stream
    stream << "DIMENSIONS " << image.size[0] + 1 << " " << image.size[1] + 1 << " " << image.size[2] + 1 << "\n";
    for (int k = 0; k < 3; ++k)
    {
        int n = image.size[k] + 1;
        double origin = image.origin[k];
        double step = image.axes[k][k];
        stream << coords[k] << "_COORDINATES " << n << " double\n";
        for (int i = 0; i < n; ++i)
        {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    // Write cell data to stream
    int pixel_count = image.size[0] * image.size[1] * image.size[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels int\n";
    stream << "LOOKUP_TABLE default\n";
    auto image_const_view = as_const_view(cast_variant<std::int16_t>(image));
    for (int i = 0; i < pixel_count; ++i)
    {
        stream << double(image_const_view.pixels[i]) * image.value_mapping.slope + image.value_mapping.intercept << "\n";
    }
}

void write_vtk_file(file_path const& file, image<3,double,cradle::shared> const& image)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE3\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };


    // Write rectilinear grid coordinates to stream
    stream << "DIMENSIONS " << image.size[0] + 1 << " " << image.size[1] + 1 << " " << image.size[2] + 1 << "\n";
    for (int k = 0; k < 3; ++k)
    {
        int n = image.size[k] + 1;
        double origin = image.origin[k];
        double step = image.axes[k][k];
        stream << coords[k] << "_COORDINATES " << n << " double\n";
        for (int i = 0; i < n; ++i)
        {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    // Write cell data to stream
    int pixel_count = image.size[0] * image.size[1] * image.size[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels double\n";
    stream << "LOOKUP_TABLE default\n";
    auto image_const_view = as_const_view(image);
    for (int i = 0; i < pixel_count; ++i)
    {
        stream << image_const_view.pixels[i] << "\n";
    }
}

void write_vtk_file(file_path const& file, image<2,double,cradle::shared> const& image)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE2\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };

    // Write rectilinear grid coordinates to stream
    vector3u sizes = make_vector(image.size[0], image.size[1], 1u);
    stream << "DIMENSIONS " << sizes[0] + 1 << " " << sizes[1] + 1 << " " << sizes[2] + 1 << "\n";
    for (int k = 0; k < 2; ++k)
    {
        int nn = sizes[k] + 1;
        double origin = image.origin[k];
        double step = image.axes[k][k];
        stream << coords[k] << "_COORDINATES " << nn << " double\n";
        for (int i = 0; i < nn; ++i) {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    int n = sizes[2] + 1;
    double origin = -0.05;
    double step = 0.1;
    stream << coords[2] << "_COORDINATES " << n << " double\n";
    for (int i = 0; i < n; ++i)
    {
        stream << origin + i * step << " ";
    }
    stream << "\n";

    // Write cell data to stream
    int pixel_count = sizes[0] * sizes[1] * sizes[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels double\n";
    stream << "LOOKUP_TABLE default\n";
    auto image_const_view = as_const_view(image);
    for (int i = 0; i < pixel_count; ++i)
    {
        stream << image_const_view.pixels[i] << "\n";
    }
}

void write_vtk_file(file_path const& file, image<2,float,cradle::shared> const& image)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE2\n";
    stream << "ASCII\n";
    stream << "DATASET RECTILINEAR_GRID\n";

    std::string coords[3] = {
        "X",
        "Y",
        "Z",
    };

    // Write rectilinear grid coordinates to stream
    vector3u sizes = make_vector(image.size[0], image.size[1], 1u);
    stream << "DIMENSIONS " << sizes[0] + 1 << " " << sizes[1] + 1 << " " << sizes[2] + 1 << "\n";
    for (int k = 0; k < 2; ++k)
    {
        int nn = sizes[k] + 1;
        double origin = image.origin[k];
        double step = image.axes[k][k];
        stream << coords[k] << "_COORDINATES " << nn << " double\n";
        for (int i = 0; i < nn; ++i)
        {
            stream << origin + i * step << " ";
        }
        stream << "\n";
    }

    int n = sizes[2] + 1;
    double origin = -0.05;
    double step = 0.1;
    stream << coords[2] << "_COORDINATES " << n << " double\n";
    for (int i = 0; i < n; ++i)
    {
        stream << origin + i * step << " ";
    }
    stream << "\n";

    // Write cell data to stream
    int pixel_count = sizes[0] * sizes[1] * sizes[2];
    stream << "CELL_DATA " << pixel_count << "\n";
    stream << "SCALARS pixels double\n";
    stream << "LOOKUP_TABLE default\n";
    auto image_const_view = as_const_view(image);
    for (int i = 0; i < pixel_count; ++i)
    {
        stream << image_const_view.pixels[i] << "\n";
    }
}

void write_vtk_file2(file_path const& file, image<2,double,cradle::shared> const& image)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out);

    // Write header information
    stream << "# vtk DataFile Version 2.0\n";
    stream << "CRADLE IMAGE2\n";
    stream << "ASCII\n";
    stream << "DATASET UNSTRUCTURED_GRID\n";

    // Write vertices to stream
    int ni = image.size[0];
    int nj = image.size[1];
    stream << "POINTS " << ni * nj << " double\n";
    auto image_const_view = as_const_view(image);
    int k = 0;
    double originX = image.origin[0];
    double originY = image.origin[1];
    double stepX = image.axes[0][0];
    double stepY = image.axes[1][1];
    for (int j = 0; j < nj; ++j)
    {
        for (int i = 0; i < ni; ++i)
        {
            stream << originX + double(i) * stepX << " " << originY + double(j) * stepY << " " << image_const_view.pixels[k++] << "\n";
        }
    }

    // Write faces to stream
    int face_count = (image.size[0] - 1) * (image.size[1] - 1);
    stream << "CELLS " << face_count << " " << (face_count * 5) << "\n";
    for (int j = 0; j < nj - 1; ++j)
    {
        for (int i = 0; i < ni - 1; ++i)
        {
            stream << "4 " << j * ni + i << " " << (j+1) * ni + i << " " << (j+1) * ni + i + 1 << " " << j * ni + i + 1 << "\n";
        }
    }

    // Write cell types to stream
    stream << "CELL_TYPES " << face_count << "\n";
    for (int i = 0; i < face_count; ++i) { stream << "7\n"; }
}

}