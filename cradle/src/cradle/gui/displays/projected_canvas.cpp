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

#include <cradle/gui/displays/projected_canvas.hpp>
#include <cradle/external/opengl.hpp>

#include <alia/ui/utilities.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/shaders.hpp>
#include <cradle/geometry/angle.hpp>
#include <cradle/geometry/line_strip.hpp>
#include <cradle/geometry/multiple_source_view.hpp>
#include <cradle/geometry/transformations.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/imaging/contiguous.hpp>
#include <cradle/imaging/isolines.hpp>
#include <cradle/io/file.hpp>

#include <GL/GLU.h>

using namespace alia;

namespace cradle {

static const double CAM_UNZOOMED_DISPLAY_SURFACE_SIZE = 200.0; // the old camera produced a view 200 units wide/tall at a zoom of 1.0
static const angle<double, cradle::radians> CAM_FOVY = angle<double, cradle::radians>(alia::pi / 3.0);

// conversion from scene_box to a camera
camera3 make_default_camera(alia::box<3,double> const& scene_box)
{
    return camera3(
        1.0,
        get_center(scene_box) + make_vector(0., 0., 400.),
        make_vector(0., 0., -1.),
        make_vector(0., 1., 0.));
}

// conversion from camera to a multiple_source_view
multiple_source_view make_view_from_camera(camera3 const& camera)
{
    multiple_source_view view;
    view.center = camera.position + 0.5 * CAM_UNZOOMED_DISPLAY_SURFACE_SIZE * camera.zoom * cos(CAM_FOVY) * camera.direction;
    view.direction = camera.direction;
    view.up = camera.up;

    view.display_surface = make_box(
        make_vector(
            -0.5 * CAM_UNZOOMED_DISPLAY_SURFACE_SIZE / camera.zoom,
            -0.5 * CAM_UNZOOMED_DISPLAY_SURFACE_SIZE / camera.zoom),
        make_vector(
            CAM_UNZOOMED_DISPLAY_SURFACE_SIZE / camera.zoom,
            CAM_UNZOOMED_DISPLAY_SURFACE_SIZE / camera.zoom));
    view.distance = make_vector(camera.zoom, camera.zoom);
    return view;
}

// convert multiple_source_view to a camera (which makes it lose the difference in x/y)
camera3 make_camera_from_view(multiple_source_view const& view)
{
    camera3 camera;
    camera.zoom = view.display_surface.size[0] / CAM_UNZOOMED_DISPLAY_SURFACE_SIZE;
    camera.direction = view.direction;
    camera.position = view.center - 0.5 * CAM_UNZOOMED_DISPLAY_SURFACE_SIZE * camera.zoom * cos(CAM_FOVY) * camera.direction;
    camera.up = view.up;
    return camera;
}

// camera getter (derived from internal multiple_source_view, but will no longer have multiple virtual sources)
cradle::camera3 projected_canvas::camera() const
{
    return make_camera_from_view(view_);
}

// camera setter (sets the internal multiple_source_view (with both virtual sources at the same distance)
void projected_canvas::set_camera(cradle::camera3 const& cam)
{
    view_ = make_view_from_camera(cam);
}

// call before draw functions are done for the frame (and call end() when you're done)
void projected_canvas::begin()
{
    in_scene_coordinates_ = false;

    auto& ctx = embedded_canvas_.context();

    // This loads the two shaders into a shader program.
    // They're stored here as escaped strings, but if you want to actively
    // develop them, you can unescape them to files and use the second block
    // of code instead.
    auto shader_program =
        gl_shader_program(ctx,
            gl_shader_object(ctx,
                in(GLenum(GL_VERTEX_SHADER)),
                text("vec4 Ambient;\r\nvec4 Diffuse;\r\nvec4 Specular;\r\n\r\nuniform vec3 view_center;\r\nuniform vec3 view_direction;\r\nuniform vec3 view_up;\r\nuniform vec2 view_distance;\r\n\r\nvec3 preprocess_point(vec3 v)\r\n{\r\n    vec3 side = normalize(cross(view_direction, view_up));\r\n    vec3 up = normalize(cross(side, view_direction));\r\n    vec3 forward = normalize(view_direction);        \r\n    \r\n    // start with orthographic defaults\r\n    float x;\r\n    float y;\r\n\r\n    if (view_distance[0] != 0.)\r\n    {\r\n        // perspective x\r\n        vec3 eye = view_center - view_distance[0] * forward;\r\n        vec3 offset = v - eye;\r\n        float z_x = dot(offset, forward);\r\n        float scale = view_distance[0] / z_x;\r\n        x = (dot(offset, side) * scale) + dot(eye, side);\r\n    }\r\n    else\r\n    {\r\n        // orthographic x\r\n        x = dot(v, side);\r\n    }\r\n\r\n    if (view_distance[1] != 0.)\r\n    {\r\n        // perspective y\r\n        vec3 eye = view_center - view_distance[1] * forward;\r\n        vec3 offset = v - eye;\r\n        float z_y = dot(offset, forward);\r\n        float scale = view_distance[1] / z_y;\r\n        y = (dot(offset, up) * scale) + dot(eye, up);\r\n    }\r\n    else\r\n    {\r\n        // orthographic y\r\n        y = dot(v, up);\r\n    }\r\n\r\n    float z = dot(v, forward);\r\n\r\n    // scale the x and y components and put them back together \r\n    return (x * side) + \r\n           (y * up) + \r\n           (z * forward);\r\n}\r\n\r\nvoid pointLight(in int i, in vec3 normal, in vec3 eye, in vec3 ecPosition3)\r\n{\r\n   float nDotVP;       // normal . light direction\r\n   float nDotHV;       // normal . light half vector\r\n   float pf;           // power factor\r\n   float attenuation;  // computed attenuation factor\r\n   float d;            // distance from surface to light source\r\n   vec3  VP;           // direction from surface to light position\r\n   vec3  halfVector;   // direction of maximum highlights\r\n\r\n   // Compute vector from surface to light position\r\n   VP = vec3 (gl_LightSource[i].position) - ecPosition3;\r\n\r\n   // Compute distance between surface and light position\r\n   d = length(VP);\r\n\r\n   // Normalize the vector from surface to light position\r\n   VP = normalize(VP);\r\n\r\n   // Compute attenuation\r\n   attenuation = 1.0 / (gl_LightSource[i].constantAttenuation +\r\n                        gl_LightSource[i].linearAttenuation * d +\r\n                        gl_LightSource[i].quadraticAttenuation * d * d);\r\n\r\n   halfVector = normalize(VP + eye);\r\n\r\n   nDotVP = max(0.0, dot(normal, VP));\r\n   nDotHV = max(0.0, dot(normal, halfVector));\r\n\r\n   if (nDotVP == 0.0)\r\n   {\r\n       pf = 0.0;\r\n   }\r\n   else\r\n   {\r\n       pf = pow(nDotHV, gl_FrontMaterial.shininess);\r\n\r\n   }\r\n   Ambient  += gl_LightSource[i].ambient * attenuation;\r\n   Diffuse  += gl_LightSource[i].diffuse * nDotVP * attenuation;\r\n   Specular += gl_LightSource[i].specular * pf * attenuation;\r\n}\r\n\r\nvec3 fnormal(void)\r\n{\r\n    //Compute the normal \r\n    vec3 normal = gl_NormalMatrix * gl_Normal;\r\n    normal = normalize(normal);\r\n    return normal;\r\n}\r\n\r\nvoid flight(in vec3 normal, in vec4 ecPosition, float alphaFade)\r\n{\r\n    vec4 color;\r\n    vec3 ecPosition3;\r\n    vec3 eye;\r\n\r\n    ecPosition3 = (vec3 (ecPosition)) / ecPosition.w;\r\n    eye = vec3 (0.0, 0.0, 1.0);\r\n\r\n    // Clear the light intensity accumulators\r\n    Ambient  = vec4 (0.0);\r\n    Diffuse  = vec4 (0.0);\r\n    Specular = vec4 (0.0);\r\n\r\n    pointLight(0, normal, eye, ecPosition3);\r\n\r\n    color = gl_FrontLightModelProduct.sceneColor +\r\n            Ambient  * gl_FrontMaterial.ambient +\r\n            Diffuse  * gl_FrontMaterial.diffuse;\r\n    color += Specular * gl_FrontMaterial.specular;\r\n    color = clamp( color, 0.0, 1.0 );\r\n    gl_FrontColor = color;\r\n\r\n    gl_FrontColor.a *= alphaFade;\r\n}\r\n\r\n\r\nvoid main (void)\r\n{\r\n    vec3  transformedNormal;\r\n    float alphaFade = gl_Color.a;\r\n\r\n    vec4 vertex = vec4(preprocess_point(vec3(gl_Vertex)), 1);\r\n\r\n    // Eye-coordinate position of vertex, needed in various calculations\r\n    vec4 ecPosition = gl_ModelViewMatrix * vertex;\r\n\r\n    // Do fixed functionality vertex transform\r\n    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vertex;\r\n    transformedNormal = fnormal();\r\n    flight(transformedNormal, ecPosition, alphaFade);\r\n}\r\n")),
            gl_shader_object(ctx,
                in(GLenum(GL_FRAGMENT_SHADER)),
                text("void main (void) \r\n{\r\n    gl_FragColor = gl_Color;\r\n}\r\n")));
    // Load the shaders from files.
    //auto shader_program =
    //    gl_shader_program(ctx,
    //        gl_shader_object(ctx,
    //            in(GLenum(GL_VERTEX_SHADER)),
    //            gui_apply(ctx,
    //                get_file_contents,
    //                text("vertex.vert"))),
    //        gl_shader_object(ctx,
    //            in(GLenum(GL_FRAGMENT_SHADER)),
    //            gui_apply(ctx,
    //                get_file_contents,
    //                text("fragment.frag"))));

    shader_program_ = 0; // Shaders disabled. They seem to have inconsistent behavior depending on computer/graphics card/drivers
        //is_gettable(shader_program) ? get(shader_program) : 0;

    multiple_source_view fixed_view =
        scale_view_to_canvas(embedded_canvas_, view_);

    gl_uniform(ctx, shader_program, text("view_center"),
        in(fixed_view.center));
    gl_uniform(ctx, shader_program, text("view_direction"),
        in(fixed_view.direction));
    gl_uniform(ctx, shader_program, text("view_up"),
        in(fixed_view.up));
    gl_uniform(ctx, shader_program, text("view_distance"),
        in(fixed_view.distance));

    if (embedded_canvas_.context().event->category != REFRESH_CATEGORY)
        this->set_scene_coordinates();

    active_ = true;
}

// call after draw functions are done for the frame (and begin() should have been called before)
void projected_canvas::end()
{
    if (active_)
    {
        set_canvas_coordinates();
        embedded_canvas_.set_scene_coordinates();
        active_ = false;
    }
}

double projected_canvas::get_zoom_level()
{
    return view_.distance[0];
}

// transpose a matrix
void static transpose(double *m, double *t)
{
    t[0] = m[0];
    t[1] = m[4];
    t[2] = m[8];
    t[3] = m[12];
    t[4] = m[1];
    t[5] = m[5];
    t[6] = m[9];
    t[7] = m[13];
    t[8] = m[2];
    t[9] = m[6];
    t[10] = m[10];
    t[11] = m[14];
    t[12] = m[3];
    t[13] = m[7];
    t[14] = m[11];
    t[15] = m[15];
}


// helper for begin(). Sets up OpenGL transformations.
void projected_canvas::set_scene_coordinates()
{
    if (!in_scene_coordinates_)
    {
        embedded_canvas_.set_canvas_coordinates();
        auto & ctx = embedded_canvas_.context();
        if (is_render_pass(embedded_canvas_.context()))
        {
            auto region = box<2,double>(embedded_canvas_.region());
            auto surface_region = region_to_surface_coordinates(ctx, region);

            glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_DEPTH_WRITEMASK);

            glViewport(
                int(surface_region.corner[0] + 0.5),
                int(ctx.system->surface_size[1] -
                    get_high_corner(surface_region)[1] + 0.5),
                int(surface_region.size[0] + 0.5),
                int(surface_region.size[1] + 0.5));
            glDepthMask(GL_TRUE);

            // Select/Reset The Projection Matrix
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();

            double mat[16];
            multiple_source_view fixed_view = scale_view_to_canvas(embedded_canvas_, view_);
            matrix<4,4,double> projection = create_projection_matrix(fixed_view);
            transpose(projection.begin(), mat);
            glLoadMatrixd(mat);

            // Select/Reset The Modelview Matrix
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadIdentity();

            GLfloat ambientColor[] = {0.3f,0.3f,0.3f,1.0f}; //Color (0.2, 0.2, 0.2)
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

            //Add positioned light
            GLfloat lightAmbient0[] = {0.0f, 0.0f, 0.0f, 1.0f};
            GLfloat lightColor0[] = {1.0f, 1.0f, 1.0f, 1.0f};
            GLfloat lightPos0[] = {-1.f, 1.f, 1.0f, 0.0f};
            //GLfloat lightPos0[] = { camera_.position[0], camera_.position[1], camera_.position[2], 1.0f};
            glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
            glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor0);
            glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient0);
            glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);


            matrix<4,4,double> look_at = create_modelview(fixed_view);
            transpose(look_at.begin(), mat);
            glLoadMatrixd(mat);

            glEnable(GL_CULL_FACE);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_COLOR_MATERIAL);
            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);
            //glEnable(GL_LIGHT1);
            glEnable(GL_NORMALIZE);
            //glShadeModel(GL_SMOOTH);

            ////Add directed light
            //GLfloat lightColor1[] = {0.5f, 0.2f, 0.2f, 1.0f}; //Color (0.5, 0.2, 0.2)
            ////Coming from the direction (-1, 0.5, 0.5)
            //GLfloat lightPos1[] = {-1.0f, 0.5f, 0.5f, 0.0f};
            //glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
            //glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

            //GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
         //   GLfloat mat_shininess[] = { 50.0 };
         //   GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

         //   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
         //   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

            glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE) ;

            //glColorMaterial (GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

            glUseProgram(shader_program_);

            //glEnable(GL_COLOR);
            //glEnable(GL_COLOR_MATERIAL);
        }

        in_scene_coordinates_ = true;
    }
}

void projected_canvas::set_canvas_coordinates()
{
    if (in_scene_coordinates_)
    {
        auto & ctx = embedded_canvas_.context();
        // clean up after set_scene_coordinates
        if (is_render_pass(ctx))
        {
            glUseProgram(0);

            glMatrixMode(GL_PROJECTION);
            glPopMatrix();

            glMatrixMode(GL_MODELVIEW);

            // clear any user-pushed transformations
            while (transforms_to_pop_--)
            {
                glPopMatrix();
            }
            // and one for the camera transform
            glPopMatrix();

            glPopAttrib();
        }

        in_scene_coordinates_ = false;
    }
}

void projected_canvas::draw_polyset_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polyset> const& set,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up) const
{
    if (is_render_pass(ctx)           &&
        is_gettable(color)            &&
        is_gettable(style)            &&
        is_gettable(set)              &&
        is_gettable(draw_plane) &&
        is_gettable(draw_plane_up))
    {
        auto const& _set = get(set);
        if (is_render_pass(ctx) && is_gettable(color))
        {
            for (auto const& i : _set.polygons)
                draw_poly_outline(ctx, color, style, in_ptr(&i), draw_plane, draw_plane_up);
            for (auto const& i : _set.holes)
                draw_poly_outline(ctx, color, style, in_ptr(&i), draw_plane, draw_plane_up);
        }
    }
}

// helper for several draw functions
template <class VertexContainer>
void draw_helper(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up,
    GLenum mode,
    VertexContainer const& vertices,
    embedded_canvas const& canvas,
    multiple_source_view const& view)
{
    if (is_render_pass(ctx)     &&
        is_gettable(color)      &&
        is_gettable(style)      &&
        is_gettable(draw_plane) &&
        is_gettable(draw_plane_up))
    {
        glUseProgram(0);
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);

        set_line_style(get(style));
        set_color(get(color));
        glBegin(mode);
        plane<double> const& _plane = get(draw_plane);
        vector3d _up = get(draw_plane_up);
        vector3d side = unit(cross(_plane.normal, _up));
        vector3d up = unit(cross(side, _plane.normal));
        for (vector<2,double> const& i : vertices)
        {
            vector3d v = _plane.point + (i[0] * side) + (i[1] * up);

            multiple_source_view fixed_view = scale_view_to_canvas(canvas, view);
            v = preprocess_point(fixed_view, v);
            glVertex3d(v[0], v[1], v[2]);
        }
        glEnd();

        glPopAttrib();
    }
}

// Draws polygon2 exterior verticies
void projected_canvas::draw_poly_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polygon2> const& poly,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up) const
{
    if (is_gettable(poly))
    {
        auto vertices = _field(ref(&poly), vertices);
        draw_helper(ctx, color, style, draw_plane, draw_plane_up, GL_LINE_LOOP, get(vertices), embedded_canvas_, view_);
        if (is_render_pass(ctx))
        {
            glUseProgram(shader_program_);
        }
    }
}

// Draws a solid filled polygon2
void projected_canvas::draw_filled_poly(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polygon2> const& poly,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up) const
{
    if (is_gettable(poly))
    {
        auto vertices = _field(ref(&poly), vertices);
        draw_helper(ctx, color, style, draw_plane, draw_plane_up, GL_POLYGON, get(vertices), embedded_canvas_, view_);
        if (is_render_pass(ctx))
        {
            glUseProgram(shader_program_);
        }
    }
}

// Draws a polyline edge
void projected_canvas::draw_polyline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<vector2d> > const& polyline,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up) const
{
    if (is_gettable(polyline))
    {
        draw_helper(ctx, color, style, draw_plane, draw_plane_up, GL_LINE_STRIP, get(polyline), embedded_canvas_, view_);
        if (is_render_pass(ctx))
        {
            glUseProgram(shader_program_);
        }
    }
}

// do a glMultMatrix to transform 2d cords to the plane
void do_plane_transform(plane<double> const& plane, vector3d const& plane_up)
{
    vector3d side = unit(cross(plane.normal, plane_up));
    vector3d up = unit(cross(side, plane.normal));
    vector3d z = unit(cross(side, up));
    vector3d translation = plane.point;

    double m[16] = {
         side[0],        side[1],        side[2],        0.0,
         up[0],          up[1],          up[2],          0.0,
         z[0],           z[1],           z[2],           0.0,
         translation[0], translation[1], translation[2], 1.0 };
    glMultMatrixd(m);
}

// draw an image onto a plane in space somewhere
void projected_canvas::draw_image(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<gray_image_display_options> const& options,
    accessor<rgba8> const& color,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up) const
{
    alia_if (is_gettable(options) && is_gettable(color) && is_gettable(draw_plane) && is_gettable(draw_plane_up))
    {
        if (is_render_pass(ctx))
        {
            glPushAttrib(GL_ENABLE_BIT);
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glDisable(GL_CULL_FACE);
            glEnable(GL_TEXTURE_2D);
            set_color(get(color));
            glPushMatrix();
        }

        if (is_render_pass(ctx))
        {
            do_plane_transform(get(draw_plane), get(draw_plane_up));
        }

        // MIKE-TODO: need to preprocess because of the distortion from having multiple SADs
        draw_gray_image(
            ctx,
            image,
            options,
            color);

        if (is_render_pass(ctx))
        {
            glPopMatrix();
            glPopAttrib();
            glUseProgram(shader_program_);
        }
    }
    alia_end
}

// draw an onto a plane in space somewhere
void projected_canvas::draw_image_isoline(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up,
    accessor<double> const& level) const
{
    alia_if (is_render_pass(ctx))
    {
        glPushMatrix();
        do_plane_transform(get(draw_plane), get(draw_plane_up));
        glUseProgram(0);
    }
    alia_end
    // MIKE-TODO: need to preprocess because of the distortion from SADs
    cradle::draw_image_isoline(ctx, color, style, image, level);

    alia_if (is_render_pass(ctx))
    {
        glUseProgram(shader_program_);
        glPopMatrix();
    }
    alia_end
}

// push the current (pre-camera /pre-view) transform onto the stack, to be restored later by pop_transform
void projected_canvas::push_transform()
{
    ++transforms_to_pop_;
    glPushMatrix();
}

// restore the last-pushed transform
void projected_canvas::pop_transform()
{
    if (transforms_to_pop_ > 0)
    {
        --transforms_to_pop_;
        glPopMatrix();
    }
}

// offset whatever is rendered after this
void projected_canvas::translate(vector3d const& offset)
{
    glTranslated(offset[0], offset[1], offset[2]);
}

// scale whatever is rendered after this by <scale>[i] along each axis
void projected_canvas::scale(vector3d const& scale)
{
    glScaled(scale[0], scale[1], scale[2]);
}

// rotate whatever is rendered after this by <angle_in_degrees> about the given axis
void projected_canvas::rotate(double angle_in_degrees, vector3d const& axis)
{
    glRotated(angle_in_degrees, axis[0], axis[1], axis[2]);
}

// disable writing to the depth buffer. Good for when rendering things with alpha < 1
void projected_canvas::disable_depth_write()
{
    glDepthMask(GL_FALSE);
}

// enable writing to the depth buffer
void projected_canvas::enable_depth_write()
{
    glDepthMask(GL_TRUE);
}

// convert a point in canvas coordinates to an associated point in the world
vector3d canvas_to_world(projected_canvas & c, vector2d const& p)
{
    auto region = (alia::box<2, double>)c.get_embedded_canvas().region();
    auto view = c.view();

    // Get viewport parameters
    double x = region.corner[0];
    double y = region.corner[1];
    double width = region.size[0];
    double height = region.size[1];

    multiple_source_view fixed_view = scale_view_to_canvas(c.get_embedded_canvas(), view);
    auto proj = create_projection_matrix(fixed_view);
    auto model = create_modelview(fixed_view);
    auto projmodel = proj * model;
    double w = projmodel(3, 0) * view.center[0] +
               projmodel(3, 1) * view.center[1] +
               projmodel(3, 2) * view.center[2] +
               projmodel(3, 3) * 1.0;
    vector3d temp = transform_point(projmodel, preprocess_point(fixed_view, fixed_view.center)) / w;
    double screen_z = temp[2];

    // now that screen-space point p needs to be run through the inverse of what we do for rendering

    // Convert screen coordinates to device coordinates (-1 to 1 on all axes for whatever is inside the frustum)
    vector4d v4;
    v4[0] = ((p[0] - x) / width) * 2.0 - 1.0;
    v4[1] = ((p[1] - y) / height) * 2.0 - 1.0;
    v4[2] = screen_z;
    v4[3] = 1;

    // ... to clip coordinates (-w to w on all axes)
    v4 *= w;

    // inverse projection will get us from clip coordinates to eye coordinates.
    // inverse modelview will get us from there to world coordinates.
    // doing both at once here as a shortcut
    auto inv = inverse(projmodel);
    v4 = transform_point(inv, v4);

    // in addition to all of that, points may be preprocessed to handle the effects of multiple virtual sources
    vector3d v3 = make_vector(v4[0], v4[1], v4[2]);
    v3 = preprocess_point_inverse(fixed_view, v3);

    // done
    return v3;
}

void clear_canvas(projected_canvas& canvas, rgb8 const& color)
{
    if (is_render_pass(canvas.get_embedded_canvas().context()))
    {
        glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
            1.0f);
        glClearDepth(1.0);
        glDepthMask(GL_TRUE);
        // Clear Screen And Depth Buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void clear_depth(projected_canvas& canvas)
{
    if (is_render_pass(canvas.get_embedded_canvas().context()))
    {
        glClearDepth(1.0);
        glDepthMask(GL_TRUE);
        // Clear Screen And Depth Buffer
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

}