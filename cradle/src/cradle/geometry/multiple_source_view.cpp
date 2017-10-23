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

#include <cradle/geometry/multiple_source_view.hpp>
#include <cradle/geometry/transformations.hpp>

namespace cradle {

// clip planes distances
static const double CLIP_NEAR = 10.0;
static const double CLIP_FAR = 5000.0;

// zoom the view in on a specific scene box.
multiple_source_view fit_view_to_scene(box<3,double> const& scene_bounds, multiple_source_view const& view)
{
    auto modelview = create_modelview(view);
    std::vector<vector3d> corners;
    corners.push_back(scene_bounds.corner);
    corners.push_back(scene_bounds.corner + make_vector(scene_bounds.size[0],                   0.,                   0.));
    corners.push_back(scene_bounds.corner + make_vector(                  0., scene_bounds.size[1],                   0.));
    corners.push_back(scene_bounds.corner + make_vector(                  0.,                   0., scene_bounds.size[2]));
    corners.push_back(scene_bounds.corner + make_vector(scene_bounds.size[0], scene_bounds.size[1],                   0.));
    corners.push_back(scene_bounds.corner + make_vector(                  0., scene_bounds.size[1], scene_bounds.size[2]));
    corners.push_back(scene_bounds.corner + make_vector(scene_bounds.size[0],                   0., scene_bounds.size[2]));
    corners.push_back(scene_bounds.corner + scene_bounds.size);
    for (auto &i : corners)
    {
        i = transform_point(modelview, i);
    }
    double max_double = (std::numeric_limits<double>::max)();
    double min_double = (std::numeric_limits<double>::lowest)();
    vector2d min = make_vector(max_double, max_double);
    vector2d max = make_vector(min_double, min_double);
    for (auto &v : corners)
    {
        for (int i=0; i<2; ++i)
        {
            if (v[i] < min[i])
            {
                min[i] = v[i];
            }
            if (v[i] > max[i])
            {
                max[i] = v[i];
            }
        }
    }
    multiple_source_view v(view);
    v.display_surface = make_box(
        min,
        max-min
    );
    return v;
}

// determine, based on the multiple_source_view, whether we should preprocess verts for
// beams_eye_view or if normal OpenGl transformations are sufficient
bool should_preprocess_verts(multiple_source_view const& view)
{
    return false; //(view.distance[0] != view.distance[1]);
}

// create a (row-major) orthographic projection matrix from a view
matrix<4,4,double> static
ortho_projection(multiple_source_view const& view)
{
    double right = view.display_surface.corner[0] + view.display_surface.size[0];
    double left = view.display_surface.corner[0];
    double top = view.display_surface.corner[1] + view.display_surface.size[1];
    double bottom = view.display_surface.corner[1];

    double right_minus_left = right - left;
    double top_minus_bottom = top - bottom;
    double clip_far_minus_near = CLIP_FAR - CLIP_NEAR;

    return make_matrix<double>(
        2.0 / right_minus_left,                    0.0,                        0.0,            -(right + left) / right_minus_left,
                           0.0, 2.0 / top_minus_bottom,                        0.0,              -(top + bottom) / (top - bottom),
                           0.0,                    0.0, -2.0 / clip_far_minus_near, -(CLIP_FAR + CLIP_NEAR) / clip_far_minus_near,
                           0.0,                    0.0,                        0.0,                                           1.0);
}

// Not sure why this function was not being used in the
// create_projection_matrix code below. Gave compile warnings - Daniel
// create a (row-major) perspective projection matrix from a view
//matrix<4,4,double> static
//perspective_projection(multiple_source_view const& view)
//{
//    double scale = CLIP_NEAR / view.distance[0];
//    double right = scale * (view.display_surface.corner[0] + view.display_surface.size[0]);
//    double left = scale * view.display_surface.corner[0];
//    double w = right - left;
//    double top = scale * (view.display_surface.corner[1] + view.display_surface.size[1]);
//    double bottom = scale * view.display_surface.corner[1];
//    double h = top - bottom;
//    double A = (right+left) / w;
//    double B = (top+bottom) / h;
//
//    double zScale = -(CLIP_FAR + CLIP_NEAR) / (CLIP_FAR - CLIP_NEAR);
//    double zOffset = -(2.0 * CLIP_FAR * CLIP_NEAR) / (CLIP_FAR - CLIP_NEAR);
//
//    return make_matrix<double>(
//        2.0 * CLIP_NEAR / w,                 0.0,      A,     0.0,
//                        0.0, 2.0 * CLIP_NEAR / h,      B,     0.0,
//                        0.0,                 0.0, zScale, zOffset,
//                        0.0,                 0.0,   -1.0,     0.0);
//}

// create a (row-major) projection matrix from a view
matrix<4,4,double>
create_projection_matrix(multiple_source_view const& view)
{
    //if (should_preprocess_verts(view) || (view.distance[0] == 0))
    //{
        return ortho_projection(view);
    //}
    //else
    //{
    //    return perspective_projection(view);
    //}
}


// how far we translate in the modelview
double modelview_translation(multiple_source_view const& view)
{
    //if (should_preprocess_verts(view) || view.distance[0]==0)
    //{
        return 500.; // Should be ok with this value. Only effects clipping not scale or size
  //  }
  //  else
  //  {
                //// Limit of 10 here prevent possible issues with very small distances (which should never happen
  //      return std::max(10.0, std::max(view.distance[0], view.distance[1]));
  //  }
}

// does all the work for preprocess_point and preprocess_point_inverse
vector3d static _preprocess_point(multiple_source_view const& view, vector3d const& v, bool inverse=false)
{
    //if (should_preprocess_verts(view))
    {
        vector3d side = unit(cross(view.direction, view.up));
        vector3d up = unit(cross(side, view.direction));
        vector3d forward = unit(view.direction);

        // start with orthographic defaults
        double x;
        double y;

        if (view.distance[0] != 0)
        {
            // perspective x
            vector3d eye = view.center - view.distance[0] * forward;
            vector3d offset = v - eye;
            double z_x = dot(offset, forward);
            double scale = inverse ? (z_x / view.distance[0]) : (view.distance[0] / z_x);
            x = (dot(offset, side) * scale) +
                dot(eye, side);
        }
        else
        {
            // orthographic x
            x = dot(v, side);
        }

        if (view.distance[1] != 0)
        {
            // perspective y
            vector3d eye = view.center - view.distance[1] * forward;
            vector3d offset = v - eye;
            double z_y = dot(offset, forward);
            double scale = inverse ? (z_y / view.distance[1]) : (view.distance[1] / z_y);
            y = (dot(offset, up) * scale) +
                dot(eye, up);
        }
        else
        {
            // orthographic y
            y = dot(v, up);
        }

        double z = dot(v, forward);

        // scale the x and y components and put them back together
        return (x * side) +
               (y * up) +
               (z * forward);
    }
    //else // either orthographic or the projection matrix can handle the perspective for us.
    //{
    //    return v;
    //}
}

// preprocess a point to account for the effects of multiple virtual source points
// (or leaves it alone and uses normal OpenGL transforms instead)
vector3d preprocess_point(multiple_source_view const& view, vector3d const& v)
{
    return _preprocess_point(view, v, false);
}

// inverse of preprocess_point
vector3d preprocess_point_inverse(multiple_source_view const& view, vector3d const& v)
{
    return _preprocess_point(view, v, true);
}

// add translation to a transformation matrix
void static
translate(matrix<4,4,double> & m, vector3d const & v)
{
    m(0, 3) += m(0, 0) * v[0] + m(0, 1) * v[1] + m(0, 2) * v[2];
    m(1, 3) += m(1, 0) * v[0] + m(1, 1) * v[1] + m(1, 2) * v[2];
    m(2, 3) += m(2, 0) * v[0] + m(2, 1) * v[1] + m(2, 2) * v[2];
    m(3, 3) += m(3, 0) * v[0] + m(3, 1) * v[1] + m(3, 2) * v[2];
}

// calculate a modelview matrix for a view
matrix<4,4,double>
create_modelview(multiple_source_view const& view)
{
    // no special preprocessing needed. a normal lookat matrix will suffice

    // Compute side and recompute up vector
    vector3d side = unit(cross(view.direction, view.up));
    vector3d up = cross(side, view.direction);

    // Build modelview matrix
    matrix<4,4,double> mat = make_matrix<double>(
        side[0],            side[1],            side[2], 0.0,
        up[0],              up[1],              up[2], 0.0,
        -view.direction[0], -view.direction[1], -view.direction[2], 0.0,
        0.0,                0.0,                0.0, 1.0);
    double d = modelview_translation(view);
    translate(mat, -(view.center - d * view.direction));
    return mat;
}

// move the camera around
cradle::multiple_source_view pan_view(
    cradle::multiple_source_view const& view, vector<3,double> const& offset)
{
    cradle::multiple_source_view v = view;
    v.center = v.center + offset;
    return v;
}

// center the view on a location
cradle::multiple_source_view center_on(
    cradle::multiple_source_view const& view, vector<3,double> const& center)
{
    cradle::multiple_source_view v = view;
    v.center = center;
    return v;
}

cradle::multiple_source_view zoom_in(
    cradle::multiple_source_view const& view, double zoom_factor)
{
    cradle::multiple_source_view v = view;
    vector2d center = v.display_surface.corner + (0.5 * v.display_surface.size);
    v.display_surface.corner = ((v.display_surface.corner - center) / zoom_factor) + center;
    v.display_surface.size = ((v.display_surface.size - center) / zoom_factor) + center;
    return v;
}

// project a point from 3d world space to 2d coordinates in a plane containing view.center and lying perpendicular to view.direction
vector2d project(vector3d const& v, multiple_source_view const& view)
{
    vector2d result;
    vector3d view_direction = unit(view.direction);
    vector3d side = unit(cross(view_direction, view.up));
    vector3d up = unit(cross(side, view_direction));

    matrix<4,4,double> m = make_matrix<double>(
        side[0],            side[1],            side[2],            0.0,
        up[0],              up[1],              up[2],              0.0,
        -view_direction[0], -view_direction[1], -view_direction[2], 0.0,
        0.0,                0.0,                0.0,                1.0);
    translate(m, -view.center);
    vector3d transformed = transform_point(m, v);
    result[0] = transformed[0];
    result[1] = transformed[1];

    if (view.distance[0] != 0.)
    {
        double dist_from_cam = view.distance[0] - transformed[2];
        double scale = view.distance[0] / dist_from_cam;
        result[0] *= scale;
    }

    if (view.distance[1] != 0.)
    {
        double dist_from_cam = view.distance[1] - transformed[2];
        double scale = view.distance[1] / dist_from_cam;
        result[1] *= scale;
    }

    return result;
}

// project a point from 2d coordinates in the plane containing view.center and lying perpendicular to view.direction3d to 3d world coordinates
vector3d unproject(vector2d const& v, multiple_source_view const& view)
{
    vector3d view_direction = unit(view.direction);
    vector3d side = unit(cross(view_direction, view.up));
    vector3d up = unit(cross(side, view_direction));

    return view.center + v[0] * side + v[1] * up;
}

// project view bounds to the plane containing view.center and lying perpendicular to view.direction3d
box<2, double> make_2d_scene_box_from_view(vector3d const& origin, multiple_source_view const& view)
{
    vector2d offset = project(origin, view);
    return make_box(
        view.display_surface.corner - offset,
        view.display_surface.size);
}

// make a plane perpendicular to the view at some distance from the focus point
plane<double> make_plane(double distance, multiple_source_view const& view)
{
    return plane<double>(
        view.center - distance * view.direction,
        view.direction);
}

}
