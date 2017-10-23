#include <cradle/gui/displays/drawing.hpp>
#include <cradle/external/opengl.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/imaging/contiguous.hpp>
#include <cradle/imaging/level_window.hpp>
#include <cradle/imaging/isolines.hpp>
#include <cradle/imaging/isobands.hpp>
#include <cradle/geometry/line_strip.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/gui/background.hpp>
#include <cradle/gui/requests.hpp>

namespace cradle {

// COLOR UTILITIES

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<uint8_t> const& alpha)
{
    return gui_apply(ctx, apply_uint8_alpha_to_rgb8, color, alpha);
}

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<float> const& alpha)
{
    return gui_apply(ctx, apply_float_alpha_to_rgb8, color, alpha);
}

gui_apply_accessor<rgba8>
apply_alpha(
    gui_context& ctx,
    accessor<rgb8> const& color,
    accessor<double> const& alpha)
{
    return gui_apply(ctx, apply_double_alpha_to_rgb8, color, alpha);
}

// LINE DRAWING

void set_line_style(line_style const& style)
{
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
}

void set_color(rgba8 const& color)
{
    glColor4ub(color.r, color.g, color.b, color.a);
}

void draw_line(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    vector<2,double> const& p0,
    vector<2,double> const& p1)
{
    assert(is_render_pass(ctx));
    if (style.width > 0)
    {
        set_line_style(style);
        set_color(color);
        glBegin(GL_LINES);
        glVertex2d(p0[0], p0[1]);
        glVertex2d(p1[0], p1[1]);
        glEnd();
    }
}

void draw_line(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<line_segment<2,double> > const& line)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(style) &&
        is_gettable(line))
    {
        draw_line(ctx, get(color), get(style), get(line)[0], get(line)[1]);
    }
}

// POLYGON DRAWING

void draw_poly_outline_3d(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polygon2 const& poly,
    double const& z)
{
    assert(is_render_pass(ctx));
    set_line_style(style);
    set_color(color);
    glDisable(GL_LIGHTING);
    glBegin(GL_LINE_LOOP);
    vector<2, double> const* end =
        poly.vertices.elements + poly.vertices.n_elements;
    for (vector<2, double> const* i = poly.vertices.elements; i != end; ++i)
        glVertex3d((*i)[0], (*i)[1], z);
    glEnd();
    glEnable(GL_LIGHTING);
}

void draw_poly_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polygon2 const& poly)
{
    assert(is_render_pass(ctx));
    set_line_style(style);
    set_color(color);
    glBegin(GL_LINE_LOOP);
    vector<2, double> const* end =
        poly.vertices.elements + poly.vertices.n_elements;
    for (vector<2, double> const* i = poly.vertices.elements; i != end; ++i)
        glVertex2d((*i)[0], (*i)[1]);
    glEnd();
}

void draw_poly_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polygon2> const& poly)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(style) &&
        is_gettable(poly))
    {
        draw_poly_outline(ctx, get(color), get(style), get(poly));
    }
}

void draw_filled_poly(
    dataless_ui_context& ctx,
    rgba8 const& color,
    polygon2 const& poly)
{
    assert(is_render_pass(ctx));
    set_color(color);
    glBegin(GL_POLYGON);
    vector<2,double> const* end =
        poly.vertices.elements + poly.vertices.n_elements;
    for (vector<2,double> const* i = poly.vertices.elements; i != end; ++i)
        glVertex2d((*i)[0], (*i)[1]);
    glEnd();
}

void draw_filled_poly(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<polygon2> const& poly)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(poly))
    {
        draw_filled_poly(ctx, get(color), get(poly));
    }
}

// BOX DRAWING

void draw_box_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<box<2,double> > const& box)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(style) &&
        is_gettable(box))
    {
        draw_box_outline(ctx, get(color), get(style), get(box));
    }
}

void draw_box_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    box<2,double> const& box)
{
    assert(is_render_pass(ctx));
    set_color(color);
    set_line_style(style);
    glBegin(GL_LINE_LOOP);
    glVertex2d(box.corner[0], box.corner[1]);
    glVertex2d(box.corner[0] + box.size[0], box.corner[1]);
    glVertex2d(box.corner[0] + box.size[0], box.corner[1] + box.size[1]);
    glVertex2d(box.corner[0], box.corner[1] + box.size[1]);
    glEnd();
}

void draw_filled_box(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<box<2,double> > const& box)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(box))
    {
        draw_filled_box(ctx, get(color), get(box));
    }
}

void draw_filled_box(
    dataless_ui_context& ctx,
    rgba8 const& color,
    box<2,double> const& box)
{
    assert(is_render_pass(ctx));
    set_color(color);
    glBegin(GL_QUADS);
    glVertex2d(box.corner[0], box.corner[1]);
    glVertex2d(box.corner[0] + box.size[0], box.corner[1]);
    glVertex2d(box.corner[0] + box.size[0], box.corner[1] + box.size[1]);
    glVertex2d(box.corner[0], box.corner[1] + box.size[1]);
    glEnd();
}

// POLYSET DRAWING

void draw_polyset_outline(
    dataless_ui_context& ctx,
    rgba8 const& color,
    line_style const& style,
    polyset const& set)
{
    assert(is_render_pass(ctx));
    {
        for (auto const& i : set.polygons)
            draw_poly_outline(ctx, color, style, i);
        for (auto const& i : set.holes)
            draw_poly_outline(ctx, color, style, i);
    }
}
void draw_polyset_outline(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<polyset> const& set)
{
    if (is_render_pass(ctx) &&
        is_gettable(set) && is_gettable(color) && is_gettable(style))
    {
        draw_polyset_outline(ctx, get(color), get(style), get(set));
    }
}
void draw_polyset_outlines(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    std::vector<polyset> const& sets)
{
    if ( is_render_pass(ctx) &&
         is_gettable(color) &&
         is_gettable(style) )
    {
        for (auto const& set : sets)
        {
            for (auto const& i : set.polygons)
                draw_poly_outline(ctx, color, style, in_ptr(&i));
            for (auto const& i : set.holes)
                draw_poly_outline(ctx, color, style, in_ptr(&i));
        }
    }
}
void draw_polyset_outlines(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<polyset>> const& sets)
{
    if ( is_render_pass(ctx) &&
         is_gettable(color) &&
         is_gettable(style) &&
         is_gettable(sets) )
    {
        draw_polyset_outlines(ctx, color, style, get(sets));
    }
}

static void
draw_triangle_list(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<std::vector<triangle<2,double> > > const& triangles)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(triangles))
    {
        set_color(get(color));
        glBegin(GL_TRIANGLES);
        for (auto const& tri : get(triangles))
        {
            glVertex2d(tri[0][0], tri[0][1]);
            glVertex2d(tri[1][0], tri[1][1]);
            glVertex2d(tri[2][0], tri[2][1]);
        }
        glEnd();
    }
}

static void
draw_colored_triangle_list(
    dataless_ui_context& ctx,
    accessor<std::vector<colored_triangle<2,double> > > const& triangles)
{
    if (is_render_pass(ctx) && is_gettable(triangles))
    {
        glBegin(GL_TRIANGLES);
        for (auto const& tri : get(triangles))
        {
            for (unsigned i = 0; i != 3; ++i)
            {
                set_color(tri[i].color);
                glVertex2d(tri[i].position[0], tri[i].position[1]);
            }
        }
        glEnd();
    }
}

void draw_filled_polyset(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<polyset> const& set)
{
    draw_triangle_list(ctx, color,
        gui_apply(ctx, triangulate_polyset, set));
}

// Draw images

struct image_drawing_data
{
    owned_id image_id;
    cached_image_ptr cached_image;
    matrix<3,3,double> transform;
};

void draw_image_with_transformation(
    surface& surface, cached_image_ptr const& image,
    matrix<3,3,double> const& transform,
    rgba8 const& color = rgba8(0xff, 0xff, 0xff, 0xff))
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    auto const& m = transform;
    double gl_matrix[16] = {
        m(0,0), m(1,0), 0, m(2,0),
        m(0,1), m(1,1), 0, m(2,1),
             0,      0, 1,      0,
        m(0,2), m(1,2), 0, m(2,2) };
    glMultMatrixd(gl_matrix);
    draw_full_image(surface, image, make_vector(0., 0.), color);
    glPopMatrix();
}

void static
draw_gray8_image(
    dataless_ui_context& ctx, image_drawing_data& data,
    accessor<image<2,uint8_t,shared> > const& image, rgba8 color)
{
    if (!data.image_id.matches(image.id()) || !is_valid(data.cached_image))
    {
        cradle::image<2,uint8_t,shared> storage;
        cradle::image<2,uint8_t,const_view> contiguous_view =
            get_contiguous_view(get(image), storage);
        alia::image_interface alia_image;
        alia_image.pixels = contiguous_view.pixels;
        alia_image.format = GRAY;
        alia_image.size = contiguous_view.size;
        alia_image.stride = unsigned(contiguous_view.step[1]);
        ctx.surface->cache_image(data.cached_image, alia_image);
        data.transform = get_spatial_mapping(get(image));
        std::ostringstream buf;
        data.image_id.store(image.id());
    }
    draw_image_with_transformation(
        *ctx.surface, data.cached_image, data.transform, color);
}

void draw_gray8_image(
    gui_context& ctx,
    accessor<image<2,uint8_t,shared> > const& image,
    accessor<rgba8> const& color)
{
    ALIA_GET_CACHED_DATA(image_drawing_data)
    if (is_render_pass(ctx) && is_gettable(image) && is_gettable(color))
        draw_gray8_image(ctx, data, image, get(color));
}

void static
draw_rgba8_image(
    dataless_ui_context& ctx, image_drawing_data& data,
    accessor<image<2,rgba8,shared> > const& image, rgba8 const& color)
{
    if (!data.image_id.matches(image.id()) || !is_valid(data.cached_image))
    {
        cradle::image<2,rgba8,shared> storage;
        cradle::image<2,rgba8,const_view> contiguous_view =
            get_contiguous_view(get(image), storage);
        alia::image_interface alia_image;
        alia_image.pixels = contiguous_view.pixels;
        alia_image.format = RGBA;
        alia_image.size = contiguous_view.size;
        alia_image.stride = unsigned(contiguous_view.step[1]);
        ctx.surface->cache_image(data.cached_image, alia_image);
        data.transform = get_spatial_mapping(get(image));
        data.image_id.store(image.id());
    }
    draw_image_with_transformation(
        *ctx.surface, data.cached_image, data.transform, color);
}

void draw_rgba8_image(
    gui_context& ctx,
    accessor<image<2,rgba8,shared> > const& image,
    accessor<rgba8> const& color)
{
    ALIA_GET_CACHED_DATA(image_drawing_data)
    if (is_render_pass(ctx) && is_gettable(image) && is_gettable(color))
        draw_rgba8_image(ctx, data, image, get(color));
}

static void
draw_gray_image(
    gui_context& ctx,
    accessor<image2> const& image,
    accessor<gray_image_display_options> const& options,
    accessor<rgba8> const& color)
{
    draw_gray8_image(ctx,
        gui_apply(ctx,
            apply_level_window_2d_api,
            image,
            _field(ref(&options), level),
            _field(ref(&options), window)),
        color);
}

void draw_gray_image(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<gray_image_display_options> const& options,
    accessor<rgba8> const& color)
{
    draw_gray_image(ctx, image.get_regularly_spaced_image(ctx),
        options, color);
}

void draw_line_strips(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<std::vector<line_strip> > const& strips)
{
    if (is_render_pass(ctx) && is_gettable(color) && is_gettable(style) &&
        is_gettable(strips))
    {
        set_color(get(color));
        set_line_style(get(style));
        for (auto const& i : get(strips))
        {
            glBegin(GL_LINE_STRIP);
            for (auto const& j : i.vertices)
            {
                glVertex2d(j[0], j[1]);
            }
            glEnd();
        }
    }
}

static void
draw_image_isoline(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    accessor<image2> const& image,
    accessor<double> const& level)
{
    draw_line_strips(ctx, color, style,
        gui_apply(ctx,
            connect_line_segments,
            gui_apply(ctx, compute_isolines_api, image, level),
            in(0.)));
}

void draw_image_isoline(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<line_style> const& style,
    image_interface_2d const& image,
    accessor<double> const& level)
{
    draw_image_isoline(ctx, color, style,
        image.get_regularly_spaced_image(ctx), level);
}

static void
draw_image_isoband(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<image2> const& image,
    accessor<double> const& low_level,
    accessor<double> const& high_level)
{
    draw_triangle_list(ctx, color,
        gui_apply(ctx,
            compute_isobands_api,
            image, low_level, high_level));
}

void draw_image_isoband(
    gui_context& ctx,
    accessor<rgba8> const& color,
    image_interface_2d const& image,
    accessor<double> const& low_level,
    accessor<double> const& high_level)
{
    draw_image_isoband(ctx, color, image.get_regularly_spaced_image(ctx),
        low_level, high_level);
}

static void
draw_shaded_image_isoband(
    gui_context& ctx,
    accessor<image2> const& image,
    accessor<rgba8> const& low_color,
    accessor<double> const& low_level,
    accessor<rgba8> const& high_color,
    accessor<double> const& high_level)
{
    draw_colored_triangle_list(ctx,
        gui_apply(ctx,
            compute_shaded_isobands_api,
            image, low_color, low_level, high_color, high_level));
}

void draw_shaded_image_isoband(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<rgba8> const& low_color,
    accessor<double> const& low_level,
    accessor<rgba8> const& high_color,
    accessor<double> const& high_level)
{
    draw_shaded_image_isoband(ctx, image.get_regularly_spaced_image(ctx),
        low_color, low_level, high_color, high_level);
}

void draw_color_mapped_image(
    gui_context& ctx,
    image_interface_2d const& image,
    accessor<color_map> const& map)
{
    draw_rgba8_image(ctx,
        gui_apply(ctx,
            apply_color_map_2d_api,
            image.get_regularly_spaced_image(ctx),
            map));
}

// MESH DRAWING

struct vbo_deletion : opengl_action_interface
{
    vbo_deletion(GLuint vertices, GLuint normals)
      : vertices(vertices), normals(normals)
    {}

    GLuint vertices, normals;

    void execute()
    {
        glDeleteBuffers(1, &vertices);
        glDeleteBuffers(1, &normals);
    }
};

struct opengl_vbo : noncopyable
{
    opengl_vbo() : is_valid_(false) {}
    ~opengl_vbo() { reset(); }
    void reset()
    {
        if (is_valid_)
        {
            ctx_.schedule_action(new vbo_deletion(vertices_, normals_));
            is_valid_ = false;
        }
    }
    bool is_valid() const { return is_valid_; }
    // Get the OpenGL IDs of the buffers.
    GLuint vertices() const { return vertices_; }
    GLuint normals() const { return normals_; }
    // Call during render passes to update if necessary.
    bool refresh(opengl_context* ctx)
    {
        // If the program is outdated, reset it.
        if (is_valid_ && !ctx_.is_current())
            reset();
        // If the buffers don't exist, create them.
        if (!is_valid_)
        {
            ctx_.reset(ctx);
            glGenBuffers(1, &vertices_);
            glGenBuffers(1, &normals_);
            is_valid_ = true;
            return true;
        }
        return false;
    }
 private:
    bool is_valid_;
    GLuint vertices_, normals_; // buffer objects
    opengl_context_ref ctx_;
};

struct preprocessed_mesh
{
    std::vector<vector3f> vertices;
    std::vector<vector3f> normals;
};

preprocessed_mesh static
preprocess_mesh_with_normals(triangle_mesh_with_normals const& m)
{
    preprocessed_mesh pre;
    auto n_tris = m.face_position_indices.size();
    pre.vertices.reserve(n_tris * 3);
    pre.normals.reserve(n_tris * 3);
    for (size_t i = 0; i != n_tris; ++i)
    {
        face3 const f_p = m.face_position_indices[i];
        if (i < m.face_normal_indices.size())
        {
            for (int j = 0; j != 3; ++j)
            {
                auto const& f_n = m.face_normal_indices[i];
                auto const& n =
                    m.vertex_normals.size() > f_n[j] ?
                    m.vertex_normals[f_n[j]] : make_vector(0.0, 0.0, 0.0);
                pre.normals.push_back(vector3f(n));
            }
        }
        else
        {
            vector3d const& v0 = m.vertex_positions[f_p[0]];
            vector3d const& v1 = m.vertex_positions[f_p[1]];
            vector3d const& v2 = m.vertex_positions[f_p[2]];
            vector3d normal = unit(cross(v1 - v0, v2 - v0));
            for (int j = 0; j != 3; ++j)
                pre.normals.push_back(vector3f(normal));
        }
        for (int j = 0; j != 3; ++j)
        {
            vector3d const& p = m.vertex_positions[f_p[j]];
            pre.vertices.push_back(vector3f(p));
        }
    }
    assert(pre.vertices.size() == n_tris * 3);
    assert(pre.normals.size() == n_tris * 3);
    return pre;
}

preprocessed_mesh static
preprocess_mesh(triangle_mesh const& m)
{
    preprocessed_mesh pre;
    auto n_tris = m.faces.size();
    pre.vertices.reserve(n_tris * 3);
    pre.normals.reserve(n_tris * 3);
    for (size_t i = 0; i != n_tris; ++i)
    {
        face3 const& f_p = m.faces[i];
        vector3d const& v0 = m.vertices[f_p[0]];
        vector3d const& v1 = m.vertices[f_p[1]];
        vector3d const& v2 = m.vertices[f_p[2]];
        vector3d normal = unit(cross(v1 - v0, v2 - v0));
        for (int j = 0; j != 3; ++j)
            pre.normals.push_back(vector3f(normal));
        pre.vertices.push_back(vector3f(v0));
        pre.vertices.push_back(vector3f(v1));
        pre.vertices.push_back(vector3f(v2));
    }
    assert(pre.vertices.size() == n_tris * 3);
    assert(pre.normals.size() == n_tris * 3);
    return pre;
}

struct mesh_drawing_data
{
    opengl_vbo vbo;
    owned_id mesh_id;
    GLsizei n_vertices;
};

bool static inline
is_valid(mesh_drawing_data& data)
{ return data.vbo.is_valid(); }

static mesh_drawing_data&
get_mesh_drawing_data(
    gui_context& ctx, accessor<preprocessed_mesh> const& mesh)
{
    mesh_drawing_data* data;
    get_cached_data(ctx, &data);
    alia_untracked_if (is_render_pass(ctx))
    {
        if (!is_gettable(mesh) || !data->mesh_id.matches(mesh.id()))
            data->vbo.reset();
        if (is_gettable(mesh))
        {
            auto& surface = static_cast<opengl_surface&>(*ctx.system->surface);
            if (data->vbo.refresh(&surface.context()))
            {
                auto vertices = data->vbo.vertices();
                glBindBuffer(GL_ARRAY_BUFFER, vertices);
                glBufferData(GL_ARRAY_BUFFER,
                    get(mesh).vertices.size() * sizeof(vector3f),
                    &get(mesh).vertices[0], GL_STATIC_DRAW);

                auto normals = data->vbo.normals();
                glBindBuffer(GL_ARRAY_BUFFER, normals);
                glBufferData(GL_ARRAY_BUFFER,
                    get(mesh).normals.size() * sizeof(vector3f),
                    &get(mesh).normals[0], GL_STATIC_DRAW);

                data->mesh_id.store(mesh.id());

                data->n_vertices = GLsizei(get(mesh).vertices.size());

                check_opengl_errors();
            }
        }
    }
    alia_end
    return *data;
}

void static
draw_triangle_mesh(mesh_drawing_data& data)
{
    glBindBuffer(GL_ARRAY_BUFFER, data.vbo.vertices());
    glVertexPointer(3, GL_FLOAT, 0, 0L);

    glBindBuffer(GL_ARRAY_BUFFER, data.vbo.normals());
    glNormalPointer(GL_FLOAT, 0, 0L);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glDrawArrays(GL_TRIANGLES, 0, data.n_vertices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void static
draw_outlined_triangle_mesh(
    rgba8 const& color,
    rgba8 const& outline_color,
    mesh_drawing_data& mesh)
{
    if (outline_color.a != 0)
    {
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);

        // Render the mesh into the stencil buffer.

        glEnable(GL_STENCIL_TEST);

        glStencilFunc(GL_ALWAYS, 1, -1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        set_color(color);
        draw_triangle_mesh(mesh);

        // Render the thick wireframe version.

        glStencilFunc(GL_NOTEQUAL, 1, -1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glLineWidth(2);
        glPolygonMode(GL_FRONT, GL_LINE);

        glDisable(GL_LIGHTING);

        set_color(outline_color);
        draw_triangle_mesh(mesh);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glEnable(GL_LIGHTING);
        glDisable(GL_STENCIL_TEST);
    }
    else
    {
        set_color(color);
        draw_triangle_mesh(mesh);
    }
}

void draw_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<triangle_mesh> const& mesh)
{
    auto preprocessed = gui_apply(ctx, preprocess_mesh, mesh);
    auto& data = get_mesh_drawing_data(ctx, preprocessed);
    if (is_render_pass(ctx) && is_gettable(color) && is_valid(data))
    {
        set_color(get(color));
        draw_triangle_mesh(data);
    }
}

void draw_outlined_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<rgba8> const& outline_color,
    accessor<triangle_mesh> const& mesh)
{
    auto preprocessed = gui_apply(ctx, preprocess_mesh, mesh);
    auto& data = get_mesh_drawing_data(ctx, preprocessed);
    if (is_render_pass(ctx) && is_gettable(color) &&
        is_gettable(outline_color) && is_valid(data))
    {
        draw_outlined_triangle_mesh(get(color), get(outline_color), data);
    }
}

void draw_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<triangle_mesh_with_normals> const& mesh)
{
    auto preprocessed = gui_apply(ctx, preprocess_mesh_with_normals, mesh);
    auto& data = get_mesh_drawing_data(ctx, preprocessed);
    if (is_render_pass(ctx) && is_gettable(color) && is_valid(data))
    {
        set_color(get(color));
        draw_triangle_mesh(data);
    }
}

void draw_outlined_triangle_mesh(
    gui_context& ctx,
    accessor<rgba8> const& color,
    accessor<rgba8> const& outline_color,
    accessor<triangle_mesh_with_normals> const& mesh)
{
    auto preprocessed = gui_apply(ctx, preprocess_mesh_with_normals, mesh);
    auto& data = get_mesh_drawing_data(ctx, preprocessed);
    if (is_render_pass(ctx) && is_gettable(color) &&
        is_gettable(outline_color) && is_valid(data))
    {
        draw_outlined_triangle_mesh(get(color), get(outline_color), data);
    }
}

}
