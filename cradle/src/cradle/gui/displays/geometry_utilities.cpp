#include <cradle/gui/displays/geometry_utilities.hpp>

#include <alia/ui/utilities.hpp>

#include <cradle/external/opengl.hpp>
#include <cradle/geometry/meshing.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/gui/displays/sliced_3d_canvas.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/requests.hpp>
#include <cradle/gui/widgets.hpp>
#include <cradle/imaging/isobands.hpp>

namespace cradle {

void draw_point(
    gui_context& ctx, sliced_3d_canvas& canvas,
    accessor<point_rendering_options> const& options_accessor,
    accessor<gui_point> const& point)
{
    auto position = gui_request(ctx, _field(ref(&point), position));
    alia_untracked_if (
        is_render_pass(ctx) && is_gettable(options_accessor) &&
        is_gettable(point) && is_gettable(position))
    {
        unsigned axis = canvas.slice_axis();
        double p = get(position)[axis];
        double center_of_points_slice  =
            round_slice_position(canvas.scene().slicing[axis], p);
        double center_of_current_slice =
            round_slice_position(canvas.scene().slicing[axis],
                canvas.state().slice_positions[axis]);
        bool is_in_slice = (center_of_points_slice == center_of_current_slice);

        alia_untracked_if (is_in_slice)
        {
            vector2d p0 = slice(get(position), axis);
            auto const& options = get(options_accessor);
            alia_untracked_if (options.line_type != line_stipple_type::NONE)
            {
                auto x_step = make_vector(options.size, 0.0);
                auto y_step = make_vector(0.0, options.size);

                draw_line( ctx,
                           apply_alpha(get(point).color, 0xFF),
                           make_line_style(options.line_type,
                            float(options.line_thickness)),
                           p0 - x_step,
                           p0 + x_step );
                draw_line( ctx,
                           apply_alpha(get(point).color, 0xFF),
                           make_line_style(options.line_type,
                            float(options.line_thickness)),
                           p0 - y_step,
                           p0 + y_step );
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

// Sets the line style based on type
line_style make_line_style(line_stipple_type type, float width)
{
    line_style style;
    switch (type)
    {
     case line_stipple_type::SOLID:
        style.stipple = solid_line;
        break;
     case line_stipple_type::DASHED:
        style.stipple = dashed_line;
        break;
     case line_stipple_type::DOTTED:
        style.stipple = dotted_line;
        break;
     case line_stipple_type::NONE:
        style.stipple = no_line;
        break;
    }
    style.width = width;
    return style;
}

// UIs for geometry display options

void static
do_spatial_region_fill_controls(
    gui_context& ctx,
    alia::grid_layout& g,
    accessor<spatial_region_fill_options> const& options)
{
    alia_if (is_gettable(options))
    {
        {
            grid_row r(g);
            do_text(ctx, text("fill interior:"));
            do_spacer(ctx, GROW);
            do_check_box(ctx, _field(ref(&options), enabled));
        }
        {
            collapsible_content fill_controls(ctx, get(options).enabled);
            alia_if (fill_controls.do_content())
            {
                {
                    grid_row r(g);
                    do_text(ctx, text("fill opacity:"));
                    do_spacer(ctx, GROW);
                    do_text_control(
                        ctx,
                        enforce_max(enforce_min(
                            _field(ref(&options), opacity),
                            in(0.)), in(1.)),
                        width(8, CHARS));
                }
                do_slider(
                    ctx,
                    accessor_cast<double>(
                        _field(ref(&options), opacity)),
                    0, 1, 0.01, FILL_X);
            }
            alia_end
        }
    }
    alia_end
}

void static
do_spatial_region_outline_controls(
    gui_context& ctx,
    alia::grid_layout& g,
    accessor<spatial_region_outline_options> const& options)
{
    alia_if (is_gettable(options))
    {
        {
            grid_row r(g);
            do_text(ctx, text("outline type:"));
            do_spacer(ctx, GROW);
            do_enum_drop_down(
                ctx,
                _field(ref(&options), type),
                width(12, CHARS));
        }
        {
            collapsible_content outline_controls(ctx,
                get(options).type != line_stipple_type::NONE,
                default_transition, 1.);
            alia_if (outline_controls.do_content())
            {
                {
                    grid_row r(g);
                    do_text(ctx, text("outline opacity:"));
                    do_spacer(ctx, GROW);
                    do_text_control(
                        ctx,
                        enforce_max(enforce_min(
                            _field(ref(&options), opacity),
                            in(0.)), in(1.)),
                        width(8, CHARS));
                }
                do_slider(
                    ctx,
                    accessor_cast<double>(_field(ref(&options), opacity)),
                    0, 1, 0.01, FILL_X);

                {
                    grid_row r(g);
                    do_text(ctx, text("outline width:"));
                    do_spacer(ctx, GROW);
                    do_text_control(
                        ctx,
                        _field(ref(&options), width),
                        width(8, CHARS));
                }
            }
            alia_end
        }
    }
    alia_end
}

void do_spatial_region_display_controls(
    gui_context& ctx,
    alia::grid_layout& g,
    accessor<spatial_region_display_options> const& options)
{
    do_spatial_region_fill_controls(ctx, g, _field(ref(&options), fill));
    do_spatial_region_outline_controls(ctx, g, _field(ref(&options), outline));
}

void static
draw_polyset(
    gui_context& ctx,
    accessor<polyset> const& set,
    accessor<rgb8> const& color,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags)
{
    alia_if (is_gettable(options) && is_gettable(color))
    {
        alia_if (get(options).fill.enabled)
        {
            draw_filled_polyset(ctx,
                apply_alpha(ctx, color,
                    _field(_field(ref(&options), fill), opacity)),
                set);
        }
        alia_end

        auto const& outline = get(options).outline;
        alia_untracked_if (is_render_pass(ctx) && is_gettable(set))
        {
            if (outline.type != line_stipple_type::NONE)
            {
                if (flags & SPATIAL_REGION_HIGHLIGHTED)
                {
                    draw_polyset_outline(ctx,
                        apply_float_alpha_to_rgb8(
                            interpolate(get(color), rgb8(0xff, 0xff, 0xff), 0.4f),
                            0.6f),
                        make_line_style(line_stipple_type::SOLID,
                            outline.width + 5),
                        get(set));
                }
                draw_polyset_outline(ctx,
                    apply_float_alpha_to_rgb8(get(color), outline.opacity),
                    make_line_style(outline.type, outline.width),
                    get(set));
            }
            else
            {
                if (flags & SPATIAL_REGION_HIGHLIGHTED)
                {
                    draw_polyset_outline(ctx,
                        get(color),
                        make_line_style(line_stipple_type::SOLID, 2),
                        get(set));
                }
            }
        }
        alia_end
    }
    alia_end
}

indirect_accessor<request<triangle_mesh> >
remove_normals(
    gui_context& ctx,
    accessor<request<triangle_mesh_with_normals> > const& mesh)
{
    return make_indirect(ctx,
        gui_apply(ctx, rq_remove_mesh_normals, mesh));
}

// Overload helper to draw defined mesh on an image slice
void draw_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<rgb8> const& color,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags)
{
    draw_mesh_slice(
        ctx, canvas, mesh, rq_in(identity_matrix<4,double>()), color,
        options, flags);
}

gui_apply_accessor<request<polyset> > static
get_mesh_slice_request(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform)
{
    auto slice_positions = canvas.state().slice_positions;
    auto slice_axis = canvas.slice_axis();

    plane<double> cross_section_plane;
    vector3d up;
    switch (slice_axis)
    {
     case 0:
        cross_section_plane.normal = make_vector(1., 0., 0.);
        up = make_vector(0., 0., 1.);
        break;
     case 1:
        cross_section_plane.normal = make_vector(0., -1., 0.);
        up = make_vector(0., 0., 1.);
        break;
     case 2:
        cross_section_plane.normal = make_vector(0., 0., 1.);
        up = make_vector(0., 1., 0.);
        break;
    }
    cross_section_plane.point =
        unslice(make_vector(0., 0.), slice_axis, slice_positions[slice_axis]);

    // [open-cradle] This functionality has been removed in the open-cradle repo
    throw exception("[open-cradle] This functionality has been removed in the open-cradle repo: " + string(__FUNCTION__));
    //return
    //    gui_apply(ctx,
    //        rq_foreground<polyset>,
    //        gui_apply(ctx,
    //            rq_sliced_transformed_mesh,
    //            rq_in(cross_section_plane),
    //            rq_in(up),
    //            mesh,
    //            transform));
    return
        gui_apply(ctx,
            [ ](// The plane along which the mesh is being sliced
                request<plane<double> > const& cross_section_plane,
                // The up vector used to determine how the polyset is being viewed
                request<vector3d > const& up,
                // The mesh that is to be sliced
                request<triangle_mesh > const& mesh,
                // The matrix used to determine the position of the polyset
                request<matrix<4, 4, double> > const& transform)
            {
                return rq_value(polyset());
            },
            rq_in(cross_section_plane),
            rq_in(up),
            mesh,
            transform);
}

// Overload helper to draw defined mesh on an image slice
void draw_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgb8> const& color,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags)
{
    draw_polyset(ctx,
        gui_request(ctx,
            get_mesh_slice_request(ctx, canvas, mesh, transform)),
        color, options, flags);
}

void draw_filled_mesh_slice(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgba8> const& color)
{
    draw_filled_polyset(ctx,
        color,
        gui_request(ctx,
            get_mesh_slice_request(ctx, canvas, mesh, transform)));
}

void draw_mesh_slice_outline(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<request<triangle_mesh> > const& mesh,
    accessor<request<matrix<4,4,double> > > const& transform,
    accessor<rgba8> const& color,
    accessor<line_style> const& options)
{
    draw_polyset_outline(ctx,
        color, options,
        gui_request(ctx,
            get_mesh_slice_request(ctx, canvas, mesh, transform)));
}

request<polyset>
compose_structure_slice_request(
    request<structure_geometry> const& structure,
    sliced_scene_geometry<3> const& scene,
    unsigned slice_axis, double slice_position)
{
    // [open-cradle] This functionality has been removed in the open-cradle repo
    throw exception("[open-cradle] This functionality has been removed in the open-cradle repo: " + string(__FUNCTION__));
    //// Use the original slice contours if slicing along the same axis.
    //// Otherwise, reslice it.
    //auto aligned_structure =
    //    slice_axis == 2 ?
    //    structure :
    //    rq_mesh_as_structure(
    //        rq_compute_triangle_mesh_from_structure(structure),
    //        rq_value(slice_axis),
    //        rq_value(scene.slicing[slice_axis]));
    //// Now get the right slice.
    //return rq_get_structure_slice_as_polyset(
    //    aligned_structure, rq_value(slice_position));

    return rq_value(polyset());
}

void draw_structure_slice(
    gui_context& ctx, sliced_3d_canvas& canvas,
    accessor<gui_structure> const& structure,
    accessor<spatial_region_display_options> const& options,
    spatial_region_drawing_flag_set flags)
{
    auto slice =
        gui_apply(ctx,
            compose_structure_slice_request,
            _field(ref(&structure), geometry),
            canvas.scene_accessor(),
            in(canvas.slice_axis()),
            in(get_slice_position(canvas)));
    draw_polyset(ctx,
        gui_request(ctx, slice),
        _field(ref(&structure), color), options, flags);
}

void do_structure_selection_controls(gui_context& ctx,
    accessor<std::map<string,gui_structure> > const& structures,
    accessor<std::map<string,bool> > const& structure_visibility)
{
    do_separator(ctx);
    do_heading(ctx, text("section-heading"), text("Structures"));
    for_each(ctx,
        [&](gui_context& ctx, accessor<string> const& id,
            accessor<gui_structure> const& structure) -> void
        {
            row_layout row(ctx);
            do_color(ctx, _field(ref(&structure), color));
            auto selected =
                select_map_index(ref(&structure_visibility), ref(&id));
            {
                auto id = get_widget_id(ctx);
                row_layout row(ctx, GROW_X | BASELINE_Y);
                do_check_box(ctx, selected, default_layout, NO_FLAGS, id);
                do_flow_text(ctx, _field(ref(&structure), label), GROW_X);
                do_box_region(ctx, id, row.region());
            }
        },
        structures);
}

}
