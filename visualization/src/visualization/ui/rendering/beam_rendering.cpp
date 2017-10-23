#include <visualization/ui/rendering/beam_rendering.hpp>

#include <alia/ui/utilities/rendering.hpp>

#include <visualization/ui/rendering/geometry_rendering.hpp>

#include <cradle/geometry/meshing.hpp>
#include <cradle/geometry/slice_mesh.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>

namespace visualization {

// BEAM AXIS


// sliced

static request<polyset>
compose_mesh_slice_request(
    request<triangle_mesh_with_normals> const& mesh,
    sliced_scene_geometry<3> const& scene,
    unsigned slice_axis,
    double slice_position)
{
    return
        rq_get_structure_slice_as_polyset(
            rq_mesh_as_structure(
                rq_remove_mesh_normals(mesh),
                rq_value(slice_axis),
                rq_value(scene.slicing[slice_axis])),
            rq_value(slice_position));
}

auto static
get_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& c3d,
    accessor<request<triangle_mesh_with_normals> > const& mesh)
{
    return
        gui_apply(ctx,
            compose_mesh_slice_request,
            mesh,
            c3d.scene_accessor(),
            in(c3d.slice_axis()),
            in(get_slice_position(c3d)));
}

struct sliced_beam_axis_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<beam_geometry> geometry;
    keyed_data<rgba8> color;
    keyed_data<optional<polyset> > field_shape;

    auto get_mesh(gui_context& ctx) const
    {
        return
            gui_apply(ctx,
                rq_make_beam_extents_mesh,
                as_value_request(make_accessor(&this->geometry)),
                as_value_request(make_accessor(&this->field_shape)),
                rq_in(min_max<double>(-0.08, 0.25)));
    }

    void render(gui_context& ctx, sliced_3d_canvas& c3d, embedded_canvas& c2d) const
    {
        auto slice = gui_request(ctx, get_mesh_slice(ctx, c3d, this->get_mesh(ctx)));
        alia_untracked_if(is_render_pass(ctx) && is_gettable(slice))
        {
            draw_polyset_outline(ctx,
                this->color.value,
                make_line_style(line_stipple_type::SOLID, 1.),
                get(slice));
        }
        alia_end
    }
};

void
add_sliced_beam_axis(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<dosimetry::beam_geometry> const& geometry,
    accessor<rgba8> const& color,
    accessor<optional<polyset> > const& field_shape,
    canvas_layer layer)
{
    sliced_beam_axis_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->geometry, geometry);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->field_shape, field_shape);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

// projected

struct projected_beam_axis_scene_object : spatial_3d_scene_graph_projected_3d_object
{
    keyed_data<beam_geometry> geometry;
    keyed_data<rgba8> color;
    keyed_data<bool> highlighted;
    keyed_data<optional<polyset> > field_shape;

    auto get_mesh(gui_context& ctx) const
    {
        return
            gui_request(ctx,
                gui_apply(ctx,
                    rq_make_beam_extents_mesh,
                    as_value_request(make_accessor(&this->geometry)),
                    as_value_request(make_accessor(&this->field_shape)),
                    rq_in(min_max<double>(-0.08, 0.25))));
    }

    void
    render(gui_context& ctx, projected_canvas& canvas) const
    {
        alia_if (is_true(make_accessor(&this->highlighted)))
        {
            draw_outlined_triangle_mesh(ctx,
                make_accessor(&this->color),
                in(rgba8(0xff, 0xff, 0xff, 0xff)),
                this->get_mesh(ctx));
        }
        alia_else
        {
            draw_triangle_mesh(ctx,
                make_accessor(&this->color),
                this->get_mesh(ctx));
        }
        alia_end
    }

    virtual indirect_accessor<double>
    get_z_depth(gui_context& ctx, projected_canvas& canvas) const
    {
        auto z_depth =
            gui_apply(ctx,
                [ ](auto const& camera_view, auto const& mesh)
                {
                    auto transformed_mesh =
                        remove_normals(
                            transform_triangle_mesh(mesh, create_modelview(camera_view)));
                    return get_high_corner(bounding_box(transformed_mesh))[2];
                },
                in(canvas.view()),
                this->get_mesh(ctx));
        return make_indirect(ctx, z_depth);
    }

    virtual indirect_accessor<double>
    get_opacity(gui_context& ctx) const
    {
        return
            make_indirect(ctx,
                gui_apply(ctx,
                    [ ](rgba8 const& color)
                    {
                        return double(color.a) / 255.;
                    },
                    make_accessor(&this->color)));
    }
};

void
add_projected_beam_axis(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<beam_geometry> const& geometry,
    accessor<rgba8> const& color,
    accessor<optional<polyset> > const& field_shape,
    accessor<bool> const& highlighted)
{
    projected_beam_axis_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->geometry, geometry);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->highlighted, highlighted);
        refresh_accessor_clone(object->field_shape, field_shape);
        add_projected_3d_scene_object(scene_graph, object);
    }
}


}
