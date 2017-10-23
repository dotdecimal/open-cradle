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

#ifndef CRADLE_IO_STEREOLITHOGRAPHY_IO_HPP
#define CRADLE_IO_STEREOLITHOGRAPHY_IO_HPP

#include <cradle/geometry/meshing.hpp>

#include <cradle/io/file.hpp>

namespace cradle {

// Writes a triangle_mesh to stereolithography (stl) file format.
void write_stl_file(
    file_path const& file,
    triangle_mesh const& mesh);

}

#endif