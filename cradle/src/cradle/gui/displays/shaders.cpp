#include <cradle/gui/displays/shaders.hpp>
#include <cradle/gui/internals.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/backends/opengl.hpp>

namespace cradle {

// UNIFORM VARIABLES

keyed_data_accessor<GLint>
gl_get_uniform_location(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name)
{
    keyed_data<GLint>* data;
    get_cached_data(ctx, &data);
    alia_untracked_if (is_render_pass(ctx))
    {
        auto& surface = static_cast<opengl_surface&>(*ctx.system->surface);
        refresh_keyed_data(*data,
            combine_ids(combine_ids(ref(&program.id()), ref(&name.id())),
                ref(&surface.context_id())));
        if (!is_valid(*data) && is_gettable(program) && is_gettable(name))
        {
            set(*data, glGetUniformLocation(get(program), get(name).c_str()));
            check_opengl_errors();
        }
    }
    alia_end
    return make_accessor(data);
}

template<class Value>
void gl_uniform_generic(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<Value> const& value)
{
    auto location = gl_get_uniform_location(ctx, program, name);

    owned_id* cached_id;
    get_cached_data(ctx, &cached_id);

    // Only interrogate the value on render passes because we don't really
    // want to require OpenGL values to be valid on other passes.
    alia_untracked_if (is_render_pass(ctx))
    {
        auto& surface = static_cast<opengl_surface&>(*ctx.system->surface);

        auto combined_id =
            combine_ids(combine_ids(ref(&location.id()), ref(&value.id())),
                ref(&surface.context_id()));

        if (!cached_id->matches(combined_id) && is_gettable(program) &&
            is_gettable(location) && is_gettable(value))
        {
            // TODO: Minimize these changes.
            glUseProgram(get(program));
            set_uniform_value(get(location), get(value));
            glUseProgram(0);
            check_opengl_errors();
            cached_id->store(combined_id);
        }
    }
    alia_end
}

// double
void static
set_uniform_value(GLint location, double value)
{
    glUniform1f(location, float(value));
}
void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<double> const& value)
{
    gl_uniform_generic(ctx, program, name, value);
}

// vector2d
void static
set_uniform_value(GLint location, vector2d const& value)
{
    glUniform2f(location, float(value[0]), float(value[1]));
}
void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<vector2d> const& value)
{
    gl_uniform_generic(ctx, program, name, value);
}

// vector3d
void static
set_uniform_value(GLint location, vector3d const& value)
{
    glUniform3f(location, float(value[0]), float(value[1]), float(value[2]));
}
void gl_uniform(gui_context& ctx, accessor<GLuint> const& program,
    accessor<string> const& name, accessor<vector3d> const& value)
{
    gl_uniform_generic(ctx, program, name, value);
}

// SHADER PROGRAMS

struct shader_deletion : opengl_action_interface
{
    shader_deletion(GLuint shader) : shader(shader) {}
    GLuint shader;
    void execute() { glDeleteShader(shader); }
};

struct opengl_shader : noncopyable
{
    opengl_shader() : is_valid_(false) {}
    ~opengl_shader() { reset(); }
    void reset()
    {
        if (is_valid_)
        {
            ctx_.schedule_action(new shader_deletion(shader_));
            is_valid_ = false;
        }
    }
    bool is_valid() const
    {
        return is_valid_;
    }
    // Get the OpenGL ID of the shader.
    GLuint get() const { return shader_; }
    // Call during render passes to update the shader if necessary.
    bool refresh(opengl_context* ctx, GLenum type)
    {
        // If the shader exists but is outdated, reset it.
        if (is_valid_ && (!ctx_.is_current() || type_ != type))
            reset();
        // If the shader doesn't exist, create it.
        if (!is_valid_)
        {
            ctx_.reset(ctx);
            shader_ = glCreateShader(type);
            type_ = type;
            is_valid_ = true;
            return true;
        }
        return false;
    }
 private:
    bool is_valid_;
    GLuint shader_;
    GLenum type_;
    opengl_context_ref ctx_;
};

void static
check_compilation_status(GLuint shader)
{
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> info_log;
        info_log.resize(log_length);
            glGetShaderInfoLog(shader, log_length, &log_length, &info_log[0]);

        throw exception("GLSL compilation failed:\n" +
            string(info_log.begin(), info_log.end()));
    }
}

indirect_accessor<GLuint>
gl_shader_object(gui_context& ctx, accessor<GLenum> const& shader_type,
    accessor<string> const& source)
{
    opengl_shader* shader;
    get_cached_data(ctx, &shader);
    owned_id* source_id;
    get_cached_data(ctx, &source_id);
    alia_untracked_if (is_render_pass(ctx))
    {
        auto& surface = static_cast<opengl_surface&>(*ctx.system->surface);
        if (!is_gettable(shader_type) || !is_gettable(source) ||
            !source_id->matches(source.id()))
        {
            shader->reset();
        }
        if (is_gettable(shader_type) && is_gettable(source))
        {
            if (shader->refresh(&surface.context(), get(shader_type)))
            {
                source_id->store(source.id());
                GLchar const* source_ptr = get(source).c_str();
                GLint source_length = GLint(get(source).length());
                glShaderSource(shader->get(), 1, &source_ptr, &source_length);
                glCompileShader(shader->get());
                check_compilation_status(shader->get());
                check_opengl_errors();
            }
        }
    }
    alia_end
    return
        make_indirect(ctx,
            unwrap_optional(
                in(shader->is_valid() ? some(shader->get()) : none)));
}

void static
check_link_status(GLuint program)
{
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
            GLint log_length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

            std::vector<GLchar> info_log;
        info_log.resize(log_length);
            glGetProgramInfoLog(program, log_length, &log_length, &info_log[0]);

        throw exception("GLSL compilation failed:\n" +
            string(info_log.begin(), info_log.end()));
    }
}

struct program_deletion : opengl_action_interface
{
    program_deletion(GLuint program) : program(program) {}
    GLuint program;
    void execute() { glDeleteProgram(program); }
};

struct opengl_program : noncopyable
{
    opengl_program() : is_valid_(false) {}
    ~opengl_program() { reset(); }
    void reset()
    {
        if (is_valid_)
        {
            ctx_.schedule_action(new program_deletion(program_));
            is_valid_ = false;
        }
    }
    bool is_valid() const
    {
        return is_valid_;
    }
    // Get the OpenGL ID of the program.
    GLuint get() const { return program_; }
    // Call during render passes to update the program if necessary.
    bool refresh(opengl_context* ctx)
    {
        // If the program is outdated, reset it.
        if (is_valid_ && !ctx_.is_current())
            reset();
        // If the program doesn't exist, create it.
        if (!is_valid_)
        {
            ctx_.reset(ctx);
            program_ = glCreateProgram();
            is_valid_ = true;
            return true;
        }
        return false;
    }
 private:
    bool is_valid_;
    GLuint program_;
    opengl_context_ref ctx_;
};

indirect_accessor<GLuint>
gl_shader_program(gui_context& ctx, accessor<GLuint> const& vertex_shader,
    accessor<GLuint> const& fragment_shader)
{
    opengl_program* program;
    get_cached_data(ctx, &program);
    owned_id* cached_id;
    get_cached_data(ctx, &cached_id);
    alia_untracked_if (is_render_pass(ctx))
    {
        auto shader_ids =
            combine_ids(
                ref(&vertex_shader.id()),
                ref(&fragment_shader.id()));
        auto& surface = static_cast<opengl_surface&>(*ctx.system->surface);
        if (!is_gettable(vertex_shader) || !is_gettable(fragment_shader) ||
            !cached_id->matches(shader_ids))
        {
            program->reset();
        }
        if (is_gettable(vertex_shader) && is_gettable(fragment_shader))
        {
            if (program->refresh(&surface.context()))
            {
                cached_id->store(shader_ids);

                auto program_id = program->get();

                glAttachShader(program_id, get(vertex_shader));
                glAttachShader(program_id, get(fragment_shader));

                glLinkProgram(program_id);
                check_link_status(program_id);

                glDetachShader(program_id, get(vertex_shader));
                glDetachShader(program_id, get(fragment_shader));

                check_opengl_errors();
            }
        }
    }
    alia_end
    return
        make_indirect(ctx,
            unwrap_optional(
                in(program->is_valid() ? some(program->get()) : none)));
}

}
