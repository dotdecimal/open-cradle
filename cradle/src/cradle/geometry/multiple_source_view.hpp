/*
 * Author(s):  Mike Meyer <mmeyer@dotdecimal.com>
 * Date:       06/20/2014
 *
 * Copyright:
 * This work was developed as a joint effort between .decimal, Inc. and
 * Partners HealthCare under research agreement A213686; as such, it is
 * jointly copyrighted by the participating organizations.
 * (c) 2013 .decimal, Inc. All rights reserved.
 * (c) 2013 Partners HealthCare. All rights reserved.
 */

#ifndef CRADLE_GEOMETRY_MULTIPLE_SOURCE_VIEW_HPP
#define CRADLE_GEOMETRY_MULTIPLE_SOURCE_VIEW_HPP

#include <alia/geometry.hpp>
#include <cradle/geometry/common.hpp>

namespace cradle {

// A multiple source view used for displaying multiple viewports simultaneously
api(struct)
struct multiple_source_view
{
    // The center position of the view
    alia::vector<3,double> center;

    // Defines how wide the field of view is at center (usually you want it centered on (0,0))
    alia::box<2,double> display_surface;

    // The direction
    alia::vector<3,double> direction;

    // Distance can be different in the X and Y dimensions (of the canvas).
    vector2d distance;

    // The 'up' direction of the view
    alia::vector<3,double> up;
};

// zoom the view in on a specific scene box.
multiple_source_view fit_view_to_scene(box<3,double> const& scene_bounds, multiple_source_view const& view);

// create a (row-major) projection matrix from a view
matrix<4,4,double>
create_projection_matrix(multiple_source_view const& view);

// calculate a modelview matrix for a view
matrix<4,4,double>
create_modelview(multiple_source_view const& view);

// move the camera around
cradle::multiple_source_view pan_view(cradle::multiple_source_view const& view, vector<3,double> const& offset);

// center the view on a location
cradle::multiple_source_view center_on(cradle::multiple_source_view const& view, vector<3,double> const& center);

// change the zoom (if zoom factor is 2, objects appear twice as large. 0.5 would be half as large, etc.)
cradle::multiple_source_view zoom_in(cradle::multiple_source_view const& view, double zoom_factor);

// preprocess a point to account for the effects of multiple virtual source points
// (or leaves it alone and uses normal OpenGL transforms instead)
vector3d preprocess_point(multiple_source_view const& view, vector3d const& v);

// inverse of preprocess_point
vector3d preprocess_point_inverse(multiple_source_view const& view, vector3d const& v);

// project a point from 3d world space to 2d coordinates in a plane containing view.center and lying perpendicular to view.direction
vector2d project(vector3d const& v, multiple_source_view const& view);

// project a point from 2d coordinates in the plane containing view.center and lying perpendicular to view.direction3d to 3d world coordinates
vector3d unproject(vector2d const& v, multiple_source_view const& view);

// project view bounds to the plane containing view.center and lying perpendicular to view.direction3d
box<2, double> make_2d_scene_box_from_view(vector3d const& origin, multiple_source_view const& view);

// make a plane perpendicular to the view at some distance from the focus point
plane<double> make_plane(double distance, multiple_source_view const& view);

}

#endif // CRADLE_GEOMETRY_MULTIPLE_SOURCE_VIEW_HPP
