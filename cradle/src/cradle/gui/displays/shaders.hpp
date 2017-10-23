#ifndef CRADLE_GUI_DISPLAYS_SHADERS_HPP
#define CRADLE_GUI_DISPLAYS_SHADERS_HPP

#include <cradle/gui/common.hpp>
#include <cradle/geometry/common.hpp>
#include <cradle/external/opengl.hpp>

namespace cradle {

// UNIFORM VARIABLES

// The following functions all synchronize a uniform shader variable with a UI
// variable. In other words, whenever the given accessor changes, glUniform is
// called to update the shader variable.

void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<double> const& value);

void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<vector2d> const& value);

void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<vector3d> const& value);

// SHADER PROGRAMS

// Given the type and source code for a shader, this will ensure that the
// given shader has been loaded and compiled. It returns an accessor to the
// OpenGL ID for the shader.
indirect_accessor<GLuint>
gl_shader_object(gui_context& ctx, accessor<GLenum> const& shader_type,
    accessor<string> const& source);

// Given a vertex shader object, this yields a program that has been linked
// against it.
indirect_accessor<GLuint>
gl_shader_program(gui_context& ctx, accessor<GLuint> const& vertex_shader,
    accessor<GLuint> const& fragment_shader);

}

#endif
