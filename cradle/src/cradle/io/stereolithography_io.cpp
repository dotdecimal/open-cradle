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

#include <cradle/io/stereolithography_io.hpp>

#include <fstream>

#include <cradle/geometry/common.hpp>

namespace cradle {

void write_stl_file(
    file_path const& file,
    triangle_mesh const& mesh)
{
    // Create output stream and open
    std::ofstream stream;
    stream.open(file.c_str(), std::ios::out | std::ios::binary);

    // Create header
    char header[80];
    std::fill_n(header, 80, ' ');
    strncpy(header, file.filename().string().c_str(),
        std::min(size_t(80), strlen(file.filename().string().c_str())));
    stream.write(header, 80);

    // Write placeholder for facet count
    unsigned int facetCount = (unsigned int)mesh.faces.size();
    stream.write((char *)&facetCount, 4);

    // Write faces to stream
    vertex3 tri[3];
    float x, y, z, nx, ny, nz;
    for (auto const& face : mesh.faces)
    {
        // Get triangle
        tri[0] = mesh.vertices[face[0]];
        tri[1] = mesh.vertices[face[1]];
        tri[2] = mesh.vertices[face[2]];

        // Compute normal vector for face and write to stream
        vector3d normal = unit(cross(tri[1] - tri[0], tri[2] - tri[0]));
        nx = float(normal[0]);
        ny = float(normal[1]);
        nz = float(normal[2]);
        stream.write((char *)&nx, 4);
        stream.write((char *)&ny, 4);
        stream.write((char *)&nz, 4);

        // Write vertices to stream
        for (int i = 0; i < 3; ++i)
        {
            x = float(tri[i][0]);
            y = float(tri[i][1]);
            z = float(tri[i][2]);
            stream.write((char *)&x, 4);
            stream.write((char *)&y, 4);
            stream.write((char *)&z, 4);
        }

        // Write attribute (unused)
        unsigned short attribute = 0;
        stream.write((char *)&attribute, 2);
    }

    // Close stream
    stream.close();
}

}