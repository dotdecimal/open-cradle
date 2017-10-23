#include <visualization/ui/rendering/geometry_rendering.hpp>

#include <alia/ui/utilities/rendering.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>

namespace visualization {

// STRUCTURES

auto static
get_structure_slice(
    gui_context& ctx,
    sliced_3d_canvas& c3d,
    accessor<gui_structure> const& structure)
{
    return
        gui_request(ctx,
            gui_apply(ctx,
                compose_structure_slice_request,
                _field(structure, geometry),
                c3d.scene_accessor(),
                in(c3d.slice_axis()),
                in(get_slice_position(c3d))));
}

struct filled_structure_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<gui_structure> structure;
    keyed_data<float> opacity;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto structure = make_accessor(&this->structure);
        draw_filled_polyset(ctx,
            apply_alpha(ctx,
                _field(structure, color),
                make_accessor(&this->opacity)),
            get_structure_slice(ctx, c3d, structure));
    }
};

void
add_sliced_filled_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<float> const& opacity,
    canvas_layer layer)
{
    filled_structure_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->structure, structure);
        refresh_accessor_clone(object->opacity, opacity);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

struct outlined_structure_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<gui_structure> structure;
    keyed_data<spatial_region_outline_parameters> rendering;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto structure = make_accessor(&this->structure);
        auto rendering = make_accessor(&this->rendering);
        auto slice = get_structure_slice(ctx, c3d, structure);
        alia_untracked_if (is_render_pass(ctx) && is_gettable(structure) &&
            is_gettable(rendering) && is_gettable(slice))
        {
            draw_polyset_outline(ctx,
                apply_float_alpha_to_rgb8(
                    get(structure).color,
                    get(rendering).opacity),
                make_line_style(get(rendering).type, get(rendering).width),
                get(slice));
        }
        alia_end
    }
};

void
add_sliced_outlined_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_outline_parameters> const& rendering,
    canvas_layer layer)
{
    outlined_structure_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->structure, structure);
        refresh_accessor_clone(object->rendering, rendering);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

void
add_projected_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_projected_rendering_parameters> const& rendering,
    accessor<optional<double>> const& transverse_position)
{
    add_projected_structure(ctx,
        scene_graph,
        structure,
        _field(structure, color),
        rendering,
        transverse_position);
}

// MESHES

struct filled_mesh_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<request<triangle_mesh> > mesh;
    keyed_data<rgb8> color;
    keyed_data<float> opacity;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        draw_filled_mesh_slice(ctx,
            c3d,
            make_accessor(&this->mesh),
            rq_in(identity_matrix<4,double>()),
            apply_alpha(ctx,
                make_accessor(&this->color),
                make_accessor(&this->opacity)));
    }
};

void
add_sliced_filled_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<float> const& opacity,
    canvas_layer layer)
{
    filled_mesh_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->mesh, mesh);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->opacity, opacity);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

struct outlined_mesh_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<request<triangle_mesh> > mesh;
    keyed_data<rgb8> color;
    keyed_data<spatial_region_outline_parameters> rendering;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto rendering = make_accessor(&this->rendering);
        draw_mesh_slice_outline(ctx,
            c3d,
            make_accessor(&this->mesh),
            rq_in(identity_matrix<4,double>()),
            apply_alpha(ctx,
                make_accessor(&this->color),
                _field(rendering, opacity)),
            gui_apply(ctx,
                make_line_style,
                _field(rendering, type),
                _field(rendering, width)));
    }
};

void
add_sliced_outlined_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_outline_parameters> const& rendering,
    canvas_layer layer)
{
    outlined_mesh_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->mesh, mesh);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->rendering, rendering);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

void static
draw_geometry_slices(
    dataless_ui_context& ctx,
    accessor<rgba8> const& color,
    accessor<cradle::line_style> const& line,
    accessor<cradle::structure_geometry> const& geometry)
{
    if(is_gettable(geometry) && is_gettable(color) && is_gettable(line)
        && is_render_pass(ctx))
    {
        for (auto const& g : get(geometry).slices)
        {
            for (auto const& p : g.second.polygons)
            {
                draw_poly_outline_3d(ctx, get(color), get(line), p, g.first);
            }
            for (auto const& h : g.second.holes)
            {
                draw_poly_outline_3d(ctx, get(color), get(line), h, g.first);
            }
        }
    }
}

void static
highlight_transverse_slice(
    dataless_ui_context& ctx,
    accessor<cradle::structure_geometry> const& geometry,
    accessor<optional<double>> const& transverse_position)
{
    if (is_gettable(geometry) && has_value(transverse_position) && is_render_pass(ctx))
    {
        auto slice =
            get_structure_slice(get(geometry), get(unwrap_optional(transverse_position)));
        if (slice)
        {
            for (auto const& poly : get(slice).region.polygons)
            {
                draw_poly_outline_3d(ctx,
                    rgba8(0xff, 0xff, 0xff, 0xff),
                    make_line_style(line_stipple_type::SOLID, 1.),
                    poly,
                    get(slice).position);
            }
        }
    }
}

struct projected_mesh_scene_object : spatial_3d_scene_graph_projected_3d_object
{
    keyed_data<request<triangle_mesh> > mesh;
    keyed_data<rgb8> color;
    keyed_data<spatial_region_projected_rendering_parameters> rendering;

    void
    render(
        gui_context& ctx,
        projected_canvas& canvas) const
    {
        auto mesh = gui_request(ctx, make_accessor(&this->mesh));
        auto rendering = make_accessor(&this->rendering);
        auto color =
            apply_alpha(ctx,
                make_accessor(&this->color),
                _field(rendering, opacity));
        auto highlighted = _field(rendering, highlighted);
        alia_if (is_true(highlighted))
        {
            draw_outlined_triangle_mesh(ctx,
                color,
                in(rgba8(0xff, 0xff, 0xff, 0xff)),
                mesh);
        }
        alia_end
        alia_if (is_false(highlighted))
        {
            draw_triangle_mesh(ctx, color, mesh);
        }
        alia_end
    }

    virtual indirect_accessor<double>
    get_z_depth(
        gui_context& ctx,
        projected_canvas& canvas) const
    {
        auto mesh = gui_request(ctx, make_accessor(&this->mesh));
        auto z_depth =
            gui_apply(ctx,
                [ ](auto const& camera_view, auto const& mesh)
                {
                    auto transformed_mesh =
                        transform_triangle_mesh(mesh, create_modelview(camera_view));
                    return get_high_corner(bounding_box(transformed_mesh))[2];
                },
                in(canvas.view()),
                mesh);
        return make_indirect(ctx, z_depth);
    }

    virtual indirect_accessor<double>
    get_opacity(gui_context& ctx) const
    {
        auto rendering = make_accessor(&this->rendering);
        return make_indirect(ctx, _field(rendering, opacity));
    }
};

struct projected_structure_scene_object : spatial_3d_scene_graph_projected_3d_object
{
    keyed_data<gui_structure> structure;
    keyed_data<rgb8> color;
    keyed_data<spatial_region_projected_rendering_parameters> rendering;
    keyed_data<optional<double>> transverse_position;

    auto get_mesh(gui_context& ctx) const
    {
        auto structure = make_accessor(&this->structure);
        return
            gui_request(ctx,
                gui_apply(ctx,
                    rq_compute_triangle_mesh_from_structure_with_options,
                    _field(structure, geometry),
                    rq_in(0.5),
                    rq_in(25000)));
    }

    void render(gui_context& ctx, projected_canvas& canvas) const
    {
        auto structure = make_accessor(&this->structure);
        auto rendering = make_accessor(&this->rendering);
        auto color =
            apply_alpha(ctx,
                make_accessor(&this->color),
                _field(rendering, opacity));
        auto transverse_position = make_accessor(&this->transverse_position);

        alia_if (_field(rendering, render_mode) == in(structure_render_mode::SOLID))
        {
            auto highlighted = _field(rendering, highlighted);
            auto mesh = this->get_mesh(ctx);
            alia_if (highlighted)
            {
                draw_outlined_triangle_mesh(ctx,
                    color,
                    in(rgba8(0xff, 0xff, 0xff, 0xff)),
                    mesh);
            }
            alia_end
            alia_if (!highlighted)
            {
                draw_triangle_mesh(ctx, color, mesh);
            }
            alia_end
        }
        alia_else_if (_field(rendering, render_mode) == in(structure_render_mode::CONTOURS))
        {
            draw_geometry_slices(
                ctx,
                color,
                in(make_line_style(line_stipple_type::SOLID, 2.)),
                gui_request(ctx, _field(structure, geometry)));
        }
        alia_end

        highlight_transverse_slice(ctx,
            gui_request(ctx, _field(structure, geometry)),
            transverse_position);
    }

    virtual indirect_accessor<double>
    get_z_depth(
        gui_context& ctx,
        projected_canvas& canvas) const
    {
        auto z_depth =
            gui_apply(ctx,
                [ ](auto const& camera_view, auto const& mesh)
                {
                    auto transformed_mesh =
                        transform_triangle_mesh(mesh, create_modelview(camera_view));
                    return get_high_corner(bounding_box(transformed_mesh))[2];
                },
                in(canvas.view()),
                this->get_mesh(ctx));
        return make_indirect(ctx, z_depth);
    }


    virtual indirect_accessor<double>
    get_opacity(gui_context& ctx) const
    {
        auto rendering = make_accessor(&this->rendering);
        return make_indirect(ctx, _field(rendering, opacity));
    }
};

void
add_projected_mesh(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_projected_rendering_parameters> const& rendering)
{
    projected_mesh_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->mesh, mesh);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->rendering, rendering);
        add_projected_3d_scene_object(scene_graph, object);
    }
}

void
add_projected_structure(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_structure> const& structure,
    accessor<rgb8> const& color,
    accessor<spatial_region_projected_rendering_parameters> const& rendering,
    accessor<optional<double>> const& transverse_position)
{
    projected_structure_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->structure, structure);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->rendering, rendering);
        refresh_accessor_clone(object->transverse_position, transverse_position);
        add_projected_3d_scene_object(scene_graph, object);
    }
}

// POINTS

struct point_3d_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<gui_point> point;
    keyed_data<point_rendering_parameters> rendering;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        draw_point(ctx, c3d, make_accessor(&this->rendering),
            make_accessor(&this->point));
    }
};

void
add_sliced_point(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<gui_point> const& point,
    accessor<point_rendering_parameters> const& rendering,
    canvas_layer layer)
{
    point_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->point, point);
        refresh_accessor_clone(object->rendering, rendering);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

}
