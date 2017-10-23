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
#include <cassert>
#include <cstring>
#include <float.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <GL/glu.h>

#include <alia/geometry.hpp>
#include <alia/ui/backends/glext.h>
#ifdef _WIN32
#include <alia/ui/backends/wglext.h>
#elif defined(__linux)
#include <X11/X.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/osmesa.h>
#endif
#include <cradle/geometry/clipper.hpp>
#include <cradle/geometry/common.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/geometry/transformations.hpp>



#include "project_via_render_to_texture.hpp"

using namespace alia;
using namespace cradle;

namespace cradle {

// upper limit on buffer size for performance reasons
const int max_buffer_size = 2 * 1024; // TODO: Not sure if this is optimal (maybe make this a setting somewhere)

// marching squares needs empty spaces on the edges
const unsigned renderbuffer_margin = 1;

// amount to scale our geometry by to make it sized correctly for clipper
static const double geometry_scale_factor = 1.0 / cradle::clipper_integer_precision;

// transpose a 4x4 matrix of doubles
void static transpose(const double *original, double *output)
{
    output[0]  = original[0];
    output[1]  = original[4];
    output[2]  = original[8];
    output[3]  = original[12];
    output[4]  = original[1];
    output[5]  = original[5];
    output[6]  = original[9];
    output[7]  = original[13];
    output[8]  = original[2];
    output[9]  = original[6];
    output[10] = original[10];
    output[11] = original[14];
    output[12] = original[3];
    output[13] = original[7];
    output[14] = original[11];
    output[15] = original[15];
}

// calculate the bounding box for a set of triangle meshes under a transform
box<3, double> calc_bounding_box(std::vector<triangle_mesh> const& meshes, matrix<4,4,double> const& transform)
{
    vector3d min = make_vector( DBL_MAX,   DBL_MAX,   DBL_MAX);
    vector3d max = make_vector(-DBL_MAX,  -DBL_MAX,  -DBL_MAX);

    for (auto mesh_iter = meshes.begin(); mesh_iter != meshes.end(); ++mesh_iter)
    {
        for (auto vertex_iter = mesh_iter->vertices.begin(); vertex_iter != mesh_iter->vertices.end(); ++vertex_iter)
        {
            vector3d transformed_vertex = transform_point(transform, *vertex_iter);
            for (unsigned i = 0; i<3; ++i)
            {
                double value = transformed_vertex[i];
                if (value < min[i]) { min[i] = value; }
                if (value > max[i]) { max[i] = value; }
            }
        }
    }
    return make_box(min, max-min);
}

// constants defining color<->index mapping
static const uint32_t color_first = 0x000001FF;
static const uint32_t color_last  = 0xFFFFFFFF;
static const uint32_t color_step  = 0x00000100;
static const uint32_t index_min   = 0x000000;
static const uint32_t index_max   = (color_last - color_first) / color_step;
// used as part of color<->index conversion to try and make nearby indices easy to tell apart for easy debugging.
// is it's own inverse
uint32_t color_transform(uint32_t color)
{
    // swap the bits so that the first few colors will be very different.
    color = (color & 0x7ffffeff) | ((color & 0x80000000) >> 23) | ((color & 0x00000100) << 23);
    color = (color & 0xff7ffdff) | ((color & 0x00800000) >> 14) | ((color & 0x00000200) << 14);
    color = (color & 0xffff7bff) | ((color & 0x00008000) >>  5) | ((color & 0x00000400) <<  5);
    color = (color & 0xbffff7ff) | ((color & 0x40000000) >> 19) | ((color & 0x00000800) << 19);
    color = (color & 0xffbfefff) | ((color & 0x00400000) >> 10) | ((color & 0x00001000) << 10);
    color = (color & 0xffffafff) | ((color & 0x00004000) >>  1) | ((color & 0x00002000) <<  1);
    color = (color & 0xdfffbfff) | ((color & 0x20000000) >> 15) | ((color & 0x00004000) << 15);
    color = (color & 0xffdfbfff) | ((color & 0x00200000) >>  6) | ((color & 0x00008000) <<  6);
    color = (color & 0xfffedfff) | ((color & 0x00002000) <<  3) | ((color & 0x00010000) >>  3);

    return color;
}

// map a mesh index to a color (inverse of get_index_for_color)
uint32_t get_color_for_index(uint32_t i)
{
    assert(i >= index_min);
    assert(i <= index_max);

    uint32_t c = (i - index_min) * color_step + color_first;

    // mix it up
    return color_transform(c);
}

// map a mesh index to an alia::rgba8 (inverse of get_index_for_alia_color)
rgba<uint8_t> get_alia_color_for_index(uint32_t i)
{
    uint32_t color = get_color_for_index(i);
    uint8_t r = (color >> 24) & 0xFF;
    uint8_t g = (color >> 16) & 0xFF;
    uint8_t b = (color >>  8) & 0xFF;
    return rgba8(r, g, b, 0xFF);
}

// map a mesh color to an index (inverse of get_color_for_index)
uint32_t get_index_for_color(uint32_t color)
{
    // unmix it up
    color = color_transform(color);

    assert(color >= color_first);
    assert(color <= color_last);
    return (color - color_first) / color_step + index_min;
}


// map a mesh color to an index (inverse of get_alia_color_for_index)
uint32_t get_index_for_alia_color(rgba<uint8_t> const& alia_color)
{
    uint32_t color = ((uint32_t)alia_color.r << 24) |
                       ((uint32_t)alia_color.g << 16) |
                       ((uint32_t)alia_color.b <<  8) |
                       0xFF;
    return get_index_for_color(color);
}

// when reading the bytes from the buffer, they are in the opposite order, this converts them
uint32_t reverse_bytes(uint32_t x)
{
    // reverse the bits in each byte
    // (comments using letters to indicate bits)
    x = ((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8);
    return  ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16);
}

// object returned by render_to_texture
// takes texture coords and turns them into
// the original coordinate system's coords
class render_to_texture_transform
{
private:
    matrix<4, 4, double> modelview_perspective_matrix;
    matrix<4, 4, double> modelview_perspective_matrix_inverse;
    unsigned texture_width;
    unsigned texture_height;
    unsigned margin;
    multiple_source_view view;
    double downstream_edge;
public:
    render_to_texture_transform(
        matrix<4, 4, double> const& modelview_perspective_matrix,
        unsigned texture_width,
        unsigned texture_height,
        unsigned margin,
        multiple_source_view const& view,
        double downstream_edge)
    {
        this->modelview_perspective_matrix = modelview_perspective_matrix;
        this->modelview_perspective_matrix_inverse = inverse(modelview_perspective_matrix);
        this->texture_width = texture_width;
        this->texture_height = texture_height;
        this->margin = margin;
        this->view = view;
        this->downstream_edge = downstream_edge;
    }
    // input is texture coords in clipper's integer version of things
    vector3d texture_to_scene(ClipperLib::IntPoint p) const
    {
        // MIKE-TODO: this seems like a roundabout way of calculating the right depth value for the point.
        // Look for a way to simplify?
        vector3d plane_point_object_space = view.center - downstream_edge * view.direction;
        plane_point_object_space = preprocess_point(view, plane_point_object_space);
        vector4d plane_point_clip_space = transform_point(
            modelview_perspective_matrix,
            make_vector(
                plane_point_object_space[0],
                plane_point_object_space[1],
                plane_point_object_space[2],
                1.0));
        double w = plane_point_clip_space[3];
        vector3d plane_point_device_coords = make_vector(
            plane_point_clip_space[0] / w,
            plane_point_clip_space[1] / w,
            plane_point_clip_space[2] / w);

        // texure coord in 2d
        vector2d p2 = from_clipper(p);
        // texture -> clip space 2d
        p2[0] = 2.0 * (p2[0] - 0.5 * (texture_width - 2.0 * double(margin))) / (texture_width - 2.0 * double(margin));
        p2[1] = 2.0 * (p2[1] - 0.5 * (texture_height - 2.0 * double(margin))) / (texture_height - 2.0 * double(margin));
        // -> clip space 3d
        vector4d p3 = make_vector(p2[0], p2[1], plane_point_device_coords[2], 1.0);
        // screen -> homogenous
        p3 *= w;

        // homogenous -> after preprocessing
        vector4d p4 = transform_point(modelview_perspective_matrix_inverse, p3);
        vector3d p5 = make_vector(
            p4[0],
            p4[1],
            p4[2]);

        // preprocessed -> scene
        return preprocess_point_inverse(view, p5);
    }

    // input is texture coords in clipper's integer version of things
    ClipperLib::IntPoint texture_to_shape(ClipperLib::IntPoint p) const
    {
        vector3d p3 = texture_to_scene(p);

        vector3d side = unit(cross(view.direction, view.up));
        vector3d up = unit(cross(side, view.direction));
        vector3d forward = unit(view.direction);

        vector3d plane_point = view.center - downstream_edge * view.direction;
        matrix<4, 4, double> m = make_matrix<double>(
            side[0], up[0], forward[0], plane_point[0],
            side[1], up[1], forward[1], plane_point[1],
            side[2], up[2], forward[2], plane_point[2],
                0.0,   0.0,        0.0,            1.0);
        m = inverse(m);
        p3 = transform_point(m, p3);
        return ClipperLib::IntPoint((ClipperLib::cInt)(p3[0] / clipper_integer_precision), (ClipperLib::cInt)(p3[1] / clipper_integer_precision));
    }
};

/// \fn void push_projection(matrix<4,4,double> const& m)
/// \ingroup geometry
/// get the projection matrix ready for rendering the meshes
void push_projection(matrix<4,4,double> const& m)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    double matrix[16];
    transpose(m.begin(), matrix);
    glLoadMatrixd(matrix);
}

/// \fn void pop_projection()
/// \ingroup geometry
/// undo push_projection
void pop_projection()
{
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

/// \fn void push_modelview(matrix<4,4,double> const& m)
/// \ingroup geometry
/// get modelview patrix ready for rendering
void push_modelview(matrix<4,4,double> const& m)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    double matrix[16];
    transpose(m.begin(), matrix);
    glLoadMatrixd(matrix);
}

/// \fn void pop_modelview()
/// \ingroup geometry
/// undo push_modelview
void pop_modelview()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/// \fn void draw_triangle_mesh(multiple_source_view const& view, rgba8 const& color, triangle_mesh const& mesh)
/// \ingroup geometry
/// Draws triangle mesh to gl using color
void draw_triangle_mesh(multiple_source_view const& view, rgba8 const& color, triangle_mesh const& mesh)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_TRIANGLES);
    for (auto const& face : mesh.faces)
    {
        {
            auto const& v = preprocess_point(view, mesh.vertices[face[0]]);
            glVertex3d(v[0], v[1], v[2]);
        }
        {
            auto const& v = preprocess_point(view, mesh.vertices[face[1]]);
            glVertex3d(v[0], v[1], v[2]);
        }
        {
            auto const& v = preprocess_point(view, mesh.vertices[face[2]]);
            glVertex3d(v[0], v[1], v[2]);
        }
    }
    glEnd();
}

#ifdef _WIN32
/// \fn void win32_error_check()
/// \ingroup geometry
/// handy for debugging purposes to call this after win32 operations
void win32_error_check()
{
    DWORD err = GetLastError();
    if (err != ERROR_SUCCESS)
    {
        LPTSTR errMsg = NULL;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &errMsg,
            0, NULL );
        assert(0);
    }
}
#endif

// handy for debugging purposes to check this after opengl oeprations
void gl_err_check()
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        #ifdef _DEBUG
            const GLubyte* errstr = gluErrorString(err);
            assert(0);
        #endif
    }
}

// todo: possibly use is_opengl_extension_in_list in alia\ui\backends\opengl.hpp
bool extension_supported(const char* ext_name)
{
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    gl_err_check();
    return (NULL != strstr(extensions, ext_name));
}

struct opengl_setup
{
    PFNGLGENFRAMEBUFFERSPROC         glGenFramebuffers;
    PFNGLDELETEFRAMEBUFFERSPROC      glDeleteFramebuffers;
    PFNGLBINDFRAMEBUFFERPROC         glBindFramebuffer;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC  glCheckFramebufferStatus;
    PFNGLFRAMEBUFFERTEXTURE2DPROC    glFramebufferTexture2D;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
    PFNGLGENRENDERBUFFERSPROC        glGenRenderbuffers;
    PFNGLDELETERENDERBUFFERSPROC     glDeleteRenderbuffers;
    PFNGLBINDRENDERBUFFERPROC        glBindRenderbuffer;
    PFNGLRENDERBUFFERSTORAGEPROC     glRenderbufferStorage;

    unsigned buffer_width;
    unsigned buffer_height;
    unsigned framebuffer_id;
    unsigned texture_id;
    unsigned depthbuffer_id;
    unsigned bytes_per_pixel;

    bool created_new_context;
#ifdef _WIN32
    HWND window_handle;
    HDC device_context;
    HGLRC render_context;
    std::string window_class_name;
#elif defined(__linux)
#endif
};

// for some reason there are a few things that
// can't be pushed/popped in OpenGL
struct opengl_restore_info
{
    int pixel_unpack_alignment;
    int depth_func;
};

#ifdef _WIN32
// wndproc for the temp window if we have to create it
LRESULT CALLBACK wndproc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
    return DefWindowProc(hwnd, msg, wparam, lparam);
}
#endif

// get the extension functions and create and link the framebuffer, color buffer, and depth buffers
opengl_setup do_opengl_setup()
{
    opengl_setup ogl_setup;

    ogl_setup.created_new_context = false;
#ifdef _WIN32
    win32_error_check();
    if (NULL == wglGetCurrentContext())
    {
        win32_error_check();
        std::ostringstream window_class_name;
        window_class_name << "do_opengl_setup_temp_window_class_" << GetCurrentThreadId();
        ogl_setup.window_class_name = window_class_name.str();
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
        WNDCLASS window_class;
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        window_class.lpfnWndProc = (WNDPROC) wndproc;
        window_class.cbClsExtra = 0;
        window_class.cbWndExtra = 0;
        window_class.hInstance = hInstance;
        window_class.hIcon = LoadIcon(NULL, IDI_WINLOGO);
        window_class.hCursor = 0;
        window_class.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        window_class.lpszMenuName = NULL;
        window_class.lpszClassName = ogl_setup.window_class_name.c_str();
        auto register_class_result = RegisterClass(&window_class);
        win32_error_check();
        assert(register_class_result);

        ogl_setup.window_handle = CreateWindowEx(
            WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
                ogl_setup.window_class_name.c_str(),
                "temp",
                WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                CW_USEDEFAULT,CW_USEDEFAULT,
                CW_USEDEFAULT,CW_USEDEFAULT,
                NULL,
                NULL,
                hInstance,
                0 );
        assert(ogl_setup.window_handle);
        ogl_setup.device_context = GetDC(ogl_setup.window_handle);
        assert(ogl_setup.device_context);
        PIXELFORMATDESCRIPTOR pfd =
         {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,
            PFD_DRAW_TO_WINDOW |
            PFD_SUPPORT_OPENGL |
            PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            24,
            0, 0, 0, 0, 0, 0, 0, 0,
            0,
            0, 0, 0, 0,
            0,
            0,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0
         };
        int pixel_format = ChoosePixelFormat(ogl_setup.device_context, &pfd);
        assert(pixel_format);
        auto set_pixel_format_result = SetPixelFormat(ogl_setup.device_context, pixel_format, &pfd);
        assert(set_pixel_format_result);
        ogl_setup.render_context = wglCreateContext(ogl_setup.device_context);
        assert(ogl_setup.render_context);
        auto make_current_result = wglMakeCurrent(ogl_setup.device_context, ogl_setup.render_context);
        assert(make_current_result);
        ogl_setup.created_new_context = true;

        wglMakeCurrent(NULL, NULL);
        wglMakeCurrent(ogl_setup.device_context, ogl_setup.render_context);
    }

    assert(extension_supported("GL_ARB_framebuffer_object"));

    int open_gl_max_buffer_size; // opengl will only support up to some size limit
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &open_gl_max_buffer_size);
    ogl_setup.buffer_width = std::min(max_buffer_size, open_gl_max_buffer_size);
    ogl_setup.buffer_height = ogl_setup.buffer_width;

    ogl_setup.glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
    ogl_setup.glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
    ogl_setup.glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
    ogl_setup.glCheckFramebufferStatus  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
    ogl_setup.glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
    ogl_setup.glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
    ogl_setup.glGenRenderbuffers        = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
    ogl_setup.glDeleteRenderbuffers     = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
    ogl_setup.glBindRenderbuffer        = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
    ogl_setup.glRenderbufferStorage     = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
#elif defined(__linux)
    // This section is used for generating apertures: DO NOT DELETE
    
    // Set up all the variables needed for making an OSMesa context
    GLint width;
    GLint height;
    GLint depth = 24;
    GLint stencil = 8;
    GLint accum = 2;

    OSMesaGetIntegerv(OSMESA_MAX_WIDTH, &width);
    OSMesaGetIntegerv(OSMESA_MAX_HEIGHT, &height);

    OSMesaContext osContext;
    osContext = OSMesaCreateContextExt(OSMESA_RGBA, depth, stencil, accum, NULL);

    // Initialize buffer to be used by OSMesa
    void *buffer = malloc(width*height*4*sizeof(GLubyte));

    OSMesaMakeCurrent(osContext, buffer, GL_UNSIGNED_BYTE, width, height);
    //auto result = OSMesaMakeCurrent(osContext, buffer, GL_UNSIGNED_BYTE, width, height);
    //assert(result);

    int open_gl_max_buffer_size; // opengl will only support up to some size limit
    OSMesaGetIntegerv(OSMESA_MAX_HEIGHT, &open_gl_max_buffer_size);
    ogl_setup.buffer_width = std::min(max_buffer_size, open_gl_max_buffer_size);
    ogl_setup.buffer_height = ogl_setup.buffer_width;

    ogl_setup.glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenFramebuffers");
    ogl_setup.glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteFramebuffers");
    ogl_setup.glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindFramebuffer");
    ogl_setup.glCheckFramebufferStatus  = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)glXGetProcAddress((const GLubyte*)"glCheckFramebufferStatus");
    ogl_setup.glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)glXGetProcAddress((const GLubyte*)"glFramebufferTexture2D");
    ogl_setup.glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)glXGetProcAddress((const GLubyte*)"glFramebufferRenderbuffer");
    ogl_setup.glGenRenderbuffers        = (PFNGLGENRENDERBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glGenRenderbuffers");
    ogl_setup.glDeleteRenderbuffers     = (PFNGLDELETERENDERBUFFERSPROC)glXGetProcAddress((const GLubyte*)"glDeleteRenderbuffers");
    ogl_setup.glBindRenderbuffer        = (PFNGLBINDRENDERBUFFERPROC)glXGetProcAddress((const GLubyte*)"glBindRenderbuffer");
    ogl_setup.glRenderbufferStorage     = (PFNGLRENDERBUFFERSTORAGEPROC)glXGetProcAddress((const GLubyte*)"glRenderbufferStorage");
#endif

    assert( ogl_setup.glGenFramebuffers         &&
            ogl_setup.glDeleteFramebuffers      &&
            ogl_setup.glBindFramebuffer         &&
            ogl_setup.glCheckFramebufferStatus  &&
            ogl_setup.glFramebufferTexture2D    &&
            ogl_setup.glFramebufferRenderbuffer &&
            ogl_setup.glGenRenderbuffers        &&
            ogl_setup.glDeleteRenderbuffers     &&
            ogl_setup.glBindRenderbuffer        &&
            ogl_setup.glRenderbufferStorage );



    ogl_setup.texture_id = 0;
    ogl_setup.framebuffer_id = 0;
    ogl_setup.depthbuffer_id = 0;
    ogl_setup.bytes_per_pixel = 4;

    glGenTextures(1, &ogl_setup.texture_id);
    glBindTexture(GL_TEXTURE_2D, ogl_setup.texture_id);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ogl_setup.buffer_width, ogl_setup.buffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    ogl_setup.glGenFramebuffers(1, &ogl_setup.framebuffer_id);
    ogl_setup.glBindFramebuffer(GL_FRAMEBUFFER, ogl_setup.framebuffer_id);

    ogl_setup.glGenRenderbuffers(1, &ogl_setup.depthbuffer_id);
    ogl_setup.glBindRenderbuffer(GL_RENDERBUFFER, ogl_setup.depthbuffer_id);
    ogl_setup.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ogl_setup.buffer_width, ogl_setup.buffer_height);
    ogl_setup.glBindRenderbuffer(GL_RENDERBUFFER, 0);

    ogl_setup.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ogl_setup.texture_id, 0);
    ogl_setup.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ogl_setup.depthbuffer_id);

    assert(GL_FRAMEBUFFER_COMPLETE == ogl_setup.glCheckFramebufferStatus(GL_FRAMEBUFFER));

    return ogl_setup;
}

/// \fn void do_opengl_shutdown(opengl_setup & ogl_setup)
/// \ingroup geometry
/// opengl shutdown - clean up after do_opengl_setup
void do_opengl_shutdown(opengl_setup & ogl_setup)
{
    glFlush(); // mike-todo - probably unneccessary
    if (ogl_setup.created_new_context)
    {
#ifdef _WIN32
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(ogl_setup.render_context);
        ogl_setup.render_context = NULL;
        ReleaseDC(ogl_setup.window_handle, ogl_setup.device_context);
        ogl_setup.device_context = NULL;
        DestroyWindow(ogl_setup.window_handle);
        ogl_setup.window_handle = NULL;
        HINSTANCE hInstance = (HINSTANCE)GetModuleHandle(NULL);
        UnregisterClass(ogl_setup.window_class_name.c_str(), hInstance);
        ogl_setup.window_class_name = "";
        ogl_setup.created_new_context = false;
#elif defined(__linux)
#endif
    }
    if (ogl_setup.depthbuffer_id)
    {
        ogl_setup.glDeleteRenderbuffers(1, &ogl_setup.depthbuffer_id);
        ogl_setup.depthbuffer_id = 0;
    }
    if (ogl_setup.framebuffer_id)
    {
        ogl_setup.glDeleteFramebuffers(1, &ogl_setup.framebuffer_id);
        ogl_setup.framebuffer_id = 0;
    }
    if (ogl_setup.texture_id)
    {
        glDeleteTextures(1, &ogl_setup.texture_id);
        ogl_setup.texture_id = 0;
    }
    if (ogl_setup.created_new_context)
    {
        ogl_setup.created_new_context = false;
    }
}


/// \fn render_to_texture_transform begin_render_to_texture(const box<3, double> bounds, opengl_setup const& ogl_setup,        opengl_restore_info& ogl_restore, std::vector<triangle_mesh> const& meshes,        multiple_source_view const& view, double downstream_edge)
/// \ingroup geometry
// get ready to render the texture
// returns a struct with info needed to map the texture back to original coords
render_to_texture_transform begin_render_to_texture(
    const box<3, double> bounds,
    opengl_setup const& ogl_setup,
    opengl_restore_info& ogl_restore,
    std::vector<triangle_mesh> const& meshes,
    multiple_source_view const& view,
    double downstream_edge)
{
    matrix<4,4,double> modelview = create_modelview(view);
    matrix<4,4,double> projection = create_projection_matrix(fit_view_to_scene(bounds, view));

    render_to_texture_transform transform(
        projection * modelview,
        ogl_setup.buffer_width,
        ogl_setup.buffer_height,
        renderbuffer_margin,
        view,
        downstream_edge);

    // 4-byte pixel alignment
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &ogl_restore.pixel_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    // depth func
    glGetIntegerv(GL_DEPTH_FUNC, &ogl_restore.depth_func);
    glDepthFunc(GL_LEQUAL);

    // pushable attribs
    glPushAttrib(GL_VIEWPORT_BIT | GL_HINT_BIT | GL_ENABLE_BIT);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_STENCIL_TEST);

    glViewport(renderbuffer_margin, renderbuffer_margin, ogl_setup.buffer_width - 2 * renderbuffer_margin, ogl_setup.buffer_height - 2 * renderbuffer_margin);

    push_projection(projection);
    push_modelview(modelview);

    ogl_setup.glBindFramebuffer(GL_FRAMEBUFFER, ogl_setup.framebuffer_id);

    glClearColor(0.0f,0.0f,0.0f,1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return transform;
}

/// \fn void end_render_to_texture(opengl_setup const& ogl_setup, opengl_restore_info& ogl_restore)
/// \ingroup geometry
/// undo begin_render_to_texture
void end_render_to_texture(opengl_setup const& ogl_setup, opengl_restore_info& ogl_restore)
{
    pop_modelview();
    pop_projection();

    // restore pushable attribs
    glPopAttrib();
    // restore glDepthFunc
    if (0 != ogl_restore.depth_func)
    {
        glDepthFunc(ogl_restore.depth_func);
        ogl_restore.depth_func = 0;
    }

    // restore GL_UNPACK_ALIGNMENT
    if (0 != ogl_restore.pixel_unpack_alignment)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, ogl_restore.pixel_unpack_alignment);
        ogl_restore.pixel_unpack_alignment = 0;
    }
}

/// \fn render_to_texture_transform render_to_texture(box<3, double> const& bounds,        opengl_setup const& ogl_setup, std::vector<triangle_mesh> const& meshes, multiple_source_view const& view, double downstream_edge)
/// \ingroup geometry
/// render the meshes to the texture that was set up by do_opengl_setup
/// returns a struct with info needed to map the texture back to original coords
render_to_texture_transform render_to_texture(
    box<3, double> const& bounds,
    opengl_setup const& ogl_setup,
    std::vector<triangle_mesh> const& meshes,
    multiple_source_view const& view,
    double downstream_edge)
{
    opengl_restore_info ogl_restore;
    render_to_texture_transform transform = begin_render_to_texture(bounds, ogl_setup, ogl_restore, meshes, view, downstream_edge);

    uint32_t index = 0;
    for (auto iter = meshes.begin(); iter != meshes.end(); ++iter)
    {
        draw_triangle_mesh(view, get_alia_color_for_index(index), *iter);
        ++index;
    }

    end_render_to_texture(ogl_setup, ogl_restore);

    return transform;
}

/// \fn void determine_cases( const uint8_t *pixels, unsigned width, unsigned height, uint32_t object_color, uint8_t *out )
/// \ingroup geometry
/// helper for marching_squares
/// generate a mapping of which case each 2x2 block is
/// out should already be allocated and have (width-1)*(height-1) elements in it
void determine_cases( const uint8_t *pixels,
                      unsigned width,
                      unsigned height,
                      uint32_t object_color,
                      uint8_t *out )
{
    // todo: I think we actually need to special case edge pixels a bit to make sure shapes get closed
    uint8_t *write_head = out;

    // easier to check the whole 32 bit color at once
    const uint32_t *pixels32 = (uint32_t *)pixels;

    // look at each 2x2 block of pixels
    for (unsigned y = 0; y < height-1; ++y)
    {
        const uint32_t *lower_row = pixels32 + (y * width);
        const uint32_t *upper_row = lower_row + width;
        for (unsigned x = 0; x < width-1; ++x)
        {
            // colors in the 2x2 block
            uint32_t ul = reverse_bytes(upper_row[x]);
            uint32_t ur = reverse_bytes(upper_row[x + 1]);
            uint32_t ll = reverse_bytes(lower_row[x]);
            uint32_t lr = reverse_bytes(lower_row[x + 1]);

            // determine which of the sixteen cases it is
            *write_head = 0x0;
            if (ll == object_color) { *write_head |= 0x1; }
            if (lr == object_color) { *write_head |= 0x2; }
            if (ur == object_color) { *write_head |= 0x4; }
            if (ul == object_color) { *write_head |= 0x8; }
            write_head++;
        }
    }
}

/// \fn void add_edge( const clipper_point& start, const clipper_point& end, clipper_polyset & polygons )
/// \ingroup geometry
// helper function for build-polygons
// NOTE: start and end should be chosen such that if you are facing from start
//       to end, the inside of the shape is on your left.
void add_edge( const clipper_point& start,
               const clipper_point& end,
               clipper_polyset & polygons )
{
    // mike-todo: try and optimize while we add.
    // can detect if the new edge just goes the same direction as the
    // last and just modify the old vertex instead of adding a new one
    clipper_polyset::iterator ending_polygon = polygons.end();
    clipper_polyset::iterator starting_polygon = polygons.end();
    for (auto iter = polygons.begin(); iter != polygons.end(); ++iter)
    {
        clipper_poly& poly = (*iter);

        // the new edge's start lining up with an end?
        if ( (ending_polygon == polygons.end()) &&
             (poly.back() == start) )
        {
            ending_polygon = iter;
        }
        // the new edge's end lining up with a start?
        if ( (starting_polygon == polygons.end()) &&
             (poly.front() == end) )
        {
            starting_polygon = iter;
        }

        if ( (ending_polygon != polygons.end()) &&
             (starting_polygon != polygons.end()) )
        {
            break;
        }
    }

    // brand new polygon
    if ( (starting_polygon == polygons.end()) &&
         (ending_polygon == polygons.end()) )
    {
        polygons.push_back(clipper_poly());
        polygons.back().push_back(start);
        polygons.back().push_back(end);
    }
    // merge polygons?
    else if ( (ending_polygon != polygons.end()) &&
              (starting_polygon != polygons.end()) )
    {
        if (ending_polygon == starting_polygon)
        {
            // nothing to do. it is just plain finsihed
        }
        else
        {
            ending_polygon->insert(ending_polygon->end(), starting_polygon->begin(), starting_polygon->end());
            polygons.erase(starting_polygon);
        }
    }
    // add at end
    else if (ending_polygon != polygons.end())
    {
        ending_polygon->push_back(end);
    }
    // add at start
    else if (starting_polygon != polygons.end())
    {
        // mike-todo: Adding at the start is gonna be real slow for a vector.
        //            Look into using std::deque as we build polygons?
        starting_polygon->insert(starting_polygon->begin(), start);
    }
}

/// \fn void build_polygons( uint8_t *cases, unsigned width, unsigned height, clipper_polyset & out )
/// \ingroup geometry
/// given a big array of cases (as generated by determine_cases), start building polygons
/// and store them in out
/// NOTE width and height should be the width and height of "cases",
/// which are each one smaller than the width/height of the original image
/// output polygons will be built scaled to 1 pixel = <geometry_scale_factor> clipper units
/// NOTE this works in texture coordinates
void build_polygons( uint8_t *cases,
                     unsigned width,
                     unsigned height,
                     clipper_polyset & out )
{
    const ClipperLib::long64 pixel_size = (ClipperLib::long64)geometry_scale_factor;
    const ClipperLib::long64 half_pixel_size = pixel_size >> 1;

    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            uint8_t case_id = cases[y * width + x];
            clipper_point lower_left( (x * pixel_size) + half_pixel_size,
                                      (y * pixel_size) + half_pixel_size);
            clipper_point left_side(  lower_left.X +               0, lower_left.Y + half_pixel_size);
            clipper_point right_side( lower_left.X +      pixel_size, lower_left.Y + half_pixel_size);
            clipper_point top_side(   lower_left.X + half_pixel_size, lower_left.Y +      pixel_size);
            clipper_point bottom_side(lower_left.X + half_pixel_size, lower_left.Y +               0);
            switch (case_id)
            {
                case 0x0:
                case 0xf:
                    // entire outside or entirely inside the object, no edges to add
                    break;
                case 0x1: // only lower left inside
                    add_edge(bottom_side, left_side, out);
                    break;
                case 0x2: // only lower right inside
                    add_edge(right_side, bottom_side, out);
                    break;
                case 0x3: // bottom inside
                    add_edge(right_side, left_side, out);
                    break;
                case 0x4: // only upper right inside
                    add_edge(top_side, right_side, out);
                    break;
                case 0x5: // upper right inside, lower left inside
                    add_edge(bottom_side, left_side, out);
                    add_edge(top_side, right_side, out);
                    break;
                case 0x6: // right side inside
                    add_edge(top_side, bottom_side, out);
                    break;
                case 0x7: // only top left outside
                    add_edge(top_side, left_side, out);
                    break;
                case 0x8: // only top left inside
                    add_edge(left_side, top_side, out);
                    break;
                case 0x9: // left side inside
                    add_edge(bottom_side, top_side, out);
                    break;
                case 0xa: // upper left inside, lower right inside
                    add_edge(left_side, top_side, out);
                    add_edge(right_side, bottom_side, out);
                    break;
                case 0xb: // only top right outside
                    add_edge(right_side, top_side, out);
                    break;
                case 0xc: // top half in object
                    add_edge(left_side, right_side, out);
                    break;
                case 0xd: // only lower-right outside in object
                    add_edge(bottom_side, right_side, out);
                    break;
                case 0xe: // only lower-left outside in object
                    add_edge(left_side, bottom_side, out);
                    break;
            }
        }
    }
}

/// \fn void marching_squares( const uint8_t *pixels, unsigned width, unsigned height, uint32_t object_color, clipper_polyset & out)
/// \ingroup geometry
/// given a buffer full of colors and a color to consider "inside", fill out with the polygons that match those shapes
void marching_squares( const uint8_t *pixels,
                       unsigned width,
                       unsigned height,
                       uint32_t object_color,
                       clipper_polyset & out)
{
    uint8_t *cases = new uint8_t[(width-1)*(height-1)];
    determine_cases(pixels, width, height, object_color, cases);
    build_polygons(cases, width-1, height-1, out);
    delete [] cases;
}

/// \fn void marching_squares_all( const uint8_t *pixels, unsigned width, unsigned height, unsigned n, std::vector<clipper_polyset> & out)
/// \ingroup geometry
/// for the first n colors, do marching squares on its color and push a (possibly empty) Polygons(plural!) into out
void marching_squares_all( const uint8_t *pixels,
                           unsigned width,
                           unsigned height,
                           unsigned n,
                           std::vector<clipper_polyset> & out)
{
    for (unsigned i = 0; i<n; ++i)
    {
        out.push_back(clipper_polyset());
        marching_squares(pixels, width, height, get_color_for_index(i), out.back());
        // mike-todo: detect and deal with holes!!!
    }
}


/// \fn clipper_polyset project_mesh_via_render_to_texture(box<3, double> const& bounds, triangle_mesh const& mesh, multiple_source_view const& view, double downstream_edge)
/// \ingroup geometry
// project a triangle mesh onto a plane,
// rendering them all at once so you get the benefits of occlusion.
// An item at index i in the resulting vector contains unoccluded regions
// of the triangle mesh at index i in the "meshes" vector.
polyset project_mesh_via_render_to_texture(
    box<3, double> const& bounds,
    triangle_mesh const& mesh,
    multiple_source_view const& view,
    double downstream_edge)
{
    //return project_triangle_mesh(mesh, make_plane(downstream_edge, view), view.up);
    std::vector<triangle_mesh> meshes(1, mesh);
    return project_meshes_via_render_to_texture(bounds, meshes, view, downstream_edge)[0];

}

/// \fn std::vector<clipper_polyset> project_meshes_via_render_to_texture(box<3, double> const& bounds,        std::vector<triangle_mesh> const& meshes, multiple_source_view const& view,        double downstream_edge)
/// \ingroup geometry
/// given a list(std::vector) of triangle meshes, project them onto a plane,
/// rendering them all at once so you get the benefits of occlusion.
/// An item at index i in the resulting vector contains unoccluded regions
/// of the triangle mesh at index i in the "meshes" vector.
std::vector<polyset> project_meshes_via_render_to_texture(
    box<3, double> const& bounds,
    std::vector<triangle_mesh> const& meshes,
    multiple_source_view const& view,
    double downstream_edge)
{
    if (meshes.size() <= 0) { return std::vector<polyset>(); }

    // Temp code for bypassing opengl
    //std::vector<clipper_polyset> p;
    //for (int i = 0; i < meshes.size(); ++i)
    //{
    //    p.push_back(project_mesh_via_render_to_texture(bounds, meshes.at(i), view, downstream_edge));
    //}

    //return p;
    //end temp


        opengl_setup ogl_setup = do_opengl_setup();
    render_to_texture_transform transform = render_to_texture(bounds, ogl_setup, meshes, view, downstream_edge);
    uint8_t *pixels = new GLubyte[ogl_setup.buffer_width * ogl_setup.buffer_height * ogl_setup.bytes_per_pixel];
    glReadPixels(0, 0, ogl_setup.buffer_width, ogl_setup.buffer_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    do_opengl_shutdown(ogl_setup);


    // for each mesh, do marching squares on its color and push a (possibly empty) Polygons(plural!) into out
    std::vector<clipper_polyset> polysets;
    marching_squares_all(pixels, ogl_setup.buffer_width, ogl_setup.buffer_height, unsigned(meshes.size()), polysets);
    for (auto& polygons : polysets)
    {
        ClipperLib::CleanPolygons(polygons, 0.005 * geometry_scale_factor);
    }

    // map back from texture coords to the original space (* geometry_scale_factor)
    for (auto& polygons : polysets)
    {
        for (auto& polygon : polygons)
        {
            for (auto& vert : polygon)
            {
                vert = transform.texture_to_shape(vert);
            }
        }
    }

    // clean up
    std::vector<clipper_polyset> out;
    for (auto polygons : polysets)
    {
        clipper_polyset temp;
        clipper_polyset temp2;
        temp.resize(polygons.size());
        ClipperLib::CleanPolygons(polygons, temp, 0.005 * geometry_scale_factor);
        ClipperLib::SimplifyPolygons(temp, temp2);
        out.push_back(temp);
    }

    delete [] pixels;

    std::vector<polyset> out_targets;
    for (auto polygon : out)
    {
        out_targets.push_back(from_clipper(polygon));
    }
    return out_targets;
}


}
