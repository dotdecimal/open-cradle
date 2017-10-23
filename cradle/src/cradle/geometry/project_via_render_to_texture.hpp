/*
 * Author(s):  Mike Meyer <mmeyer@dotdecimal.com>
 * Date:       11/14/2013
 *
 * Copyright:
 * This work was developed as a joint effort between .decimal, Inc. and
 * Partners HealthCare under research agreement A213686; as such, it is
 * jointly copyrighted by the participating organizations.
 * (c) 2013 .decimal, Inc. All rights reserved.
 * (c) 2013 Partners HealthCare. All rights reserved.
 */
#ifndef CRADLE_GEOMETRY_PROJECT_VIA_RENDER_TO_TEXTURE_HPP
#define CRADLE_GEOMETRY_PROJECT_VIA_RENDER_TO_TEXTURE_HPP

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/geometry/multiple_source_view.hpp>

#include <vector>

using namespace alia;
using namespace cradle;


namespace cradle {

// project a triangle mesh onto a plane,
// rendering them all at once so you get the benefits of occlusion.
// An item at index i in the resulting vector contains unoccluded regions
// of the triangle mesh at index i in the "meshes" vector.
api(fun)
polyset project_mesh_via_render_to_texture(
    box<3, double> const& bounds,
    triangle_mesh const& mesh,
    multiple_source_view const& view,
    double projection_plane_distance);

// given a list(std::vector) of triangle meshes, project them onto a plane,
// rendering them all at once so you get the benefits of occlusion.
// An item at index i in the resulting vector contains unoccluded regions
// of the triangle mesh at index i in the "meshes" vector.
api(fun)
std::vector<polyset> project_meshes_via_render_to_texture(
    box<3, double> const& bounds,
    std::vector<triangle_mesh> const& meshes,
    multiple_source_view const& view,
    double projection_plane_distance);

}

#endif // CRADLE_GEOMETRY_PROJECT_VIA_RENDER_TO_TEXTURE_HPP