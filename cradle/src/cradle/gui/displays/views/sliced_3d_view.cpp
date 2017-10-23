#include <cradle/gui/displays/views/sliced_3d_view.hpp>
#include <cradle/gui/displays/sliced_3d_canvas.hpp>

#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/miscellany.hpp>

namespace cradle {

// Creates a viewport for displaying 2D slices of 3D objects with appropriate user interaction tools
void
do_sliced_3d_view(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis,
    layout const& layout_spec,
    vector<3,canvas_flag_set> const& view_flags)
{
    alia_if (is_gettable(scene_geometry) && is_gettable(view_axis))
    {
        sliced_3d_canvas c3;

        canvas_flag_set flags = view_flags[get(view_axis)];

        c3.initialize(ctx, scene_geometry, get(view_axis),
            storage(ref(&state)));

        canvas c2;
        c2.initialize(ctx, get_sliced_scene_box(c3),
            base_zoom_type::FIT_SCENE, none, flags);

        side_rulers rulers(ctx, c2, BOTTOM_RULER | LEFT_RULER, layout_spec);

        layered_layout layering(ctx, GROW);

        c2.begin(layout(size(30, 30, EM), GROW | UNPADDED)); // TODO: Determine if these sizes (30x30) are ok

        clear_canvas(c2, rgb8(0x00, 0x00, 0x00));

        controller.do_content(ctx, c3, c2);

        // Reset overlay if mouse entering new canvas
        if(detect_mouse_motion(c2.context(), c2.id()))
        {
            clear_active_overlay(ctx);
        }

        apply_panning_tool(c2, MIDDLE_BUTTON);
        apply_double_click_reset_tool(c2, LEFT_BUTTON);
        apply_zoom_drag_tool(ctx, c2, RIGHT_BUTTON);

        apply_slice_line_tool(ctx, c3, c2, LEFT_BUTTON,
            rgba8(0xc0, 0xc0, 0xf0, 0xff), line_style(1, solid_line));
        apply_slice_wheel_tool(c3, c2);

        c2.end();

        controller.do_overlays(ctx, c3, c2);
    }
    alia_else
    {
        do_empty_display_panel(ctx, layout_spec);
    }
    alia_end
}


// Creates a viewport for displaying 2D slices of 3D objects locked
// to the set slice with appropriate user interaction tools
void
do_locked_sliced_3d_view(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis,
    accessor<double> const& slice_position,
    layout const& layout_spec,
    vector<3,canvas_flag_set> const& view_flags)
{
    alia_if (is_gettable(scene_geometry) && is_gettable(view_axis))
    {
        sliced_3d_canvas c3;

        canvas_flag_set flags = view_flags[get(view_axis)];

        c3.initialize(ctx, scene_geometry, get(view_axis),
            storage(ref(&state)));

        canvas c2;
        c2.initialize(ctx, get_sliced_scene_box(c3),
            base_zoom_type::FIT_SCENE, none, flags);

        side_rulers rulers(ctx, c2, BOTTOM_RULER | LEFT_RULER, layout_spec);

        layered_layout layering(ctx, GROW);

        c2.begin(layout(size(30, 30, EM), GROW | UNPADDED)); // TODO: Determine if these sizes (30x30) are ok

        clear_canvas(c2, rgb8(0x00, 0x00, 0x00));

        controller.do_content(ctx, c3, c2);

        // Reset overlay if mouse entering new canvas
        if(detect_mouse_motion(c2.context(), c2.id()))
        {
            clear_active_overlay(ctx);
        }

        apply_panning_tool(c2, MIDDLE_BUTTON);
        apply_double_click_reset_tool(c2, LEFT_BUTTON);
        apply_zoom_drag_tool(ctx, c2, RIGHT_BUTTON);

        //apply_slice_wheel_tool(c3, c2);

        c2.end();

        controller.do_overlays(ctx, c3, c2);
    }
    alia_else
    {
        do_empty_display_panel(ctx, layout_spec);
    }
    alia_end
}

void
do_sliced_3d_view_controls(
    gui_context& ctx,
    sliced_3d_view_controller const& controller,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<sliced_3d_view_state> const& state,
    accessor<unsigned> const& view_axis)
{
}

}
