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

#ifndef CRADLE_IO_VTK_IO_HPP
#define CRADLE_IO_VTK_IO_HPP

#include <cradle/imaging/forward.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/io/file.hpp>

namespace cradle {

struct triangle_mesh;
struct divergent_grid;
struct weighted_grid_index;

// Writes a triangle_mesh to visualization toolkit (vtk) file format.
void write_vtk_file(
    file_path const& file,
    triangle_mesh const& mesh);

// Writes a point cloud to visualization toolkit (vtk) file format.
void write_vtk_file(
    file_path const& file,
    std::vector<vector3d> const& points);

// Writes a polyset to visualization toolkit (vtk) file format.
void write_vtk_file(
    file_path const& file,
    polyset const& poly, double z = 0);


void write_vtk_file(
    file_path const& file,
    image3 const& image, string const& data_type);

void write_vtk_file(
    file_path const& file,
    image3 const& image1, image3 const& image2);


void write_vtk_file(
    file_path const& file,
    image3 const& image, bool writeMe);

void write_vtk_file(
    file_path const& file,
    image<3,double,cradle::shared> const& image);

void write_vtk_file(
    file_path const& file,
    image<2,double,cradle::shared> const& image);

void write_vtk_file(
    file_path const& file,
    image<2,float,cradle::shared> const& image);

void write_vtk_file2(file_path const& file, image<2,double,cradle::shared> const& image);

}

#endif