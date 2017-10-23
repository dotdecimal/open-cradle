#include <cradle/gui/displays/sliced_3d_canvas.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/gui/displays/canvas.hpp>

namespace cradle {

sliced_3d_view_state
make_default_view_state(sliced_scene_geometry<3> const& scene)
{
    sliced_3d_view_state state;
    for (unsigned i = 0; i != 3; ++i)
    {
        state.slice_positions[i] =
            round_slice_position(scene.slicing[i],
                get_center(get_slice_list_bounds(scene.slicing[i]))[0]);
    }
    return state;
}

struct sliced_3d_canvas::data
{
    keyed_data<sliced_scene_geometry<3> > scene;
    sliced_3d_view_state default_state;
};

widget_id
sliced_3d_canvas::get_state_id() const
{
    return &data_->default_state;
}

void sliced_3d_canvas::initialize(
    gui_context& ctx,
    accessor<sliced_scene_geometry<3> > const& scene,
    unsigned slice_axis,
    optional_storage<sliced_3d_view_state> const& state)
{
    ctx_ = &ctx;

    assert(is_gettable(scene));

    if (get_data(ctx, &data_))
        data_->default_state = make_default_view_state(get(scene));

    slice_axis_ = slice_axis;

    refresh_keyed_data(data_->scene, scene.id());
    if (!is_valid(data_->scene))
        set(data_->scene, get(scene));

    auto resolved_state = resolve_storage(state, &data_->default_state);

    handle_set_value_events(ctx, this->get_state_id(), resolved_state);

    // If the state is somehow uninitialized, initialize it.
    if (ctx.event->type == REFRESH_EVENT && !is_gettable(resolved_state))
        set(resolved_state, make_default_view_state(get(scene)));

    state_ = is_gettable(resolved_state) ? get(resolved_state) :
        make_default_view_state(get(scene));

    vector<3,double> rounded_slice_positions;
    for (unsigned i = 0; i != 3; ++i)
    {
        rounded_slice_positions[i] =
            round_slice_position(get(scene).slicing[i],
                state_.slice_positions[i]);
    }
    // If the slice positions are almost equal to the rounded positions, don't
    // bother setting them.
    if (!almost_equal(rounded_slice_positions, state_.slice_positions,
            0.00001))
    {
        state_.slice_positions = rounded_slice_positions;
        set(resolved_state, state_);
    }
}

box2d get_sliced_scene_box(sliced_3d_canvas& canvas)
{
    return slice(get_bounding_box(canvas.scene()), canvas.slice_axis());
}

keyed_data_accessor<sliced_scene_geometry<3> >
sliced_3d_canvas::scene_accessor() const
{
    return make_accessor(&data_->scene);
}

sliced_scene_geometry<3> const& sliced_3d_canvas::scene() const
{
    return get(data_->scene);
}

unsigned sliced_3d_canvas_axis_to_scene_axis(
    unsigned camera_axis, unsigned canvas_axis)
{
    assert(camera_axis < 3 && canvas_axis < 3);
    switch (camera_axis)
    {
     case 0:
        switch (canvas_axis)
        {
         case 0:
            return 1;
         case 1:
            return 2;
         case 2:
            return 0;
        }
     case 1:
        switch (canvas_axis)
        {
         case 0:
            return 0;
         case 1:
            return 2;
         case 2:
            return 1;
        }
     case 2:
        switch (canvas_axis)
        {
         case 0:
            return 0;
         case 1:
            return 1;
         case 2:
            return 2;
        }
    }
    // to avoid warnings
    return 0;
}

unsigned sliced_3d_scene_axis_to_canvas_axis(
    unsigned camera_axis, unsigned scene_axis)
{
    assert(camera_axis < 3 && scene_axis < 3);
    switch (camera_axis)
    {
     case 0:
        switch (scene_axis)
        {
         case 0:
            return 2;
         case 1:
            return 0;
         case 2:
            return 1;
        }
     case 1:
        switch (scene_axis)
        {
         case 0:
            return 0;
         case 1:
            return 2;
         case 2:
            return 1;
        }
     case 2:
        switch (scene_axis)
        {
         case 0:
            return 0;
         case 1:
            return 1;
         case 2:
            return 2;
        }
    }
    // to avoid warnings
    return 0;
}

void set_slice_position(sliced_3d_canvas& canvas, vector3d const& positions)
{
    sliced_3d_view_state state = canvas.state();
    state.slice_positions = positions;
    issue_set_value_event(canvas.context(), canvas.get_state_id(), state);
}

void set_slice_position(
    sliced_3d_canvas& canvas, unsigned axis, double position)
{
    sliced_3d_view_state state = canvas.state();
    state.slice_positions[axis] = position;
    issue_set_value_event(canvas.context(), canvas.get_state_id(), state);
}

image_interface_2d const&
get_image_slice(gui_context& ctx, sliced_3d_canvas& canvas,
    image_interface_3d const& img)
{
    return img.get_slice(ctx, in(canvas.slice_axis()),
        in(get_slice_position(canvas)));
}

void apply_slice_wheel_tool(
    sliced_3d_canvas& canvas, widget_id id, layout_box const& region)
{
    dataless_ui_context& ctx = canvas.context();

    hit_test_box_region(ctx, id, region, HIT_TEST_WHEEL);

    float movement;
    if (detect_wheel_movement(ctx, &movement, id))
    {
        unsigned axis = canvas.slice_axis();
        set_slice_position(canvas, axis,
            advance_slice_position(
                canvas.scene().slicing[axis],
                get_slice_positions(canvas)[axis],
                int(std::floor(movement + 0.5))));
    }
}

void apply_slice_wheel_tool(sliced_3d_canvas& canvas3, canvas& canvas2)
{
    canvas2.set_canvas_coordinates();
    apply_slice_wheel_tool(canvas3, canvas2.id(), canvas2.region());
}

void apply_slice_line_tool(
    gui_context& ctx, sliced_3d_canvas& canvas3, canvas& canvas2,
    mouse_button button, rgba8 const& color, line_style const& style,
    int principal_axis)
{
    widget_id ids[3];
    for (int i = 0; i < 3; ++i)
        ids[i] = get_widget_id(ctx);

    box2d sr = canvas2.scene_box();

    unsigned axes[2];
    axes[0] = sliced_3d_canvas_axis_to_scene_axis(canvas3.slice_axis(), 0);
    axes[1] = sliced_3d_canvas_axis_to_scene_axis(canvas3.slice_axis(), 1);

    if (principal_axis >= 0 && principal_axis != axes[0] &&
        principal_axis != axes[1])
    {
        return;
    }

    canvas2.set_canvas_coordinates();

    vector3d sp = get_slice_positions(canvas3);

    vector2d p;
    p[0] = sp[axes[0]];
    p[1] = sp[axes[1]];
    p = vector2d(scene_to_canvas(canvas2, p));

    vector2d sc = scene_to_canvas(canvas2, sr.corner);
    vector2d shc = scene_to_canvas(canvas2, get_high_corner(sr));

    vector2i ip(p);
    int const margin = 4;

    for (unsigned i = 0; i != 2; ++i)
    {
        unsigned j = 1 - i;

        if (principal_axis == axes[j])
            continue;

        if (ctx.event->type == RENDER_EVENT)
        {
            vector2d q, r;
            q[i] = p[i];
            q[j] = sc[j];
            r[i] = p[i];
            r[j] = shc[j];
            draw_line(ctx, color, style, q, r);
        }

        box2i region;
        region.corner[i] = ip[i] - margin;
        region.corner[j] = int(sc[j]);
        region.size[i] = margin * 2;
        region.size[j] = int(shc[j] - sc[j]);
        if (region.size[j] < 0)
        {
            region.corner[j] += region.size[j];
            region.size[j] = -region.size[j];
        }
        do_box_region(ctx, ids[i], region);

        if (is_click_possible(ctx, ids[i]) || is_region_active(ctx, ids[i]))
        {
            override_mouse_cursor(ctx, ids[i],
                (i == 0) ? LEFT_RIGHT_ARROW_CURSOR : UP_DOWN_ARROW_CURSOR);
        }
        if (detect_drag(ctx, ids[i], button))
        {
            vector2d q = canvas_to_scene(canvas2, get_mouse_position(ctx));
            set_slice_position(canvas3, axes[i],
                round_slice_position(canvas3.scene().slicing[axes[i]], q[i]));
        }
    }

    if (principal_axis < 0)
    {
        box2i region(ip - make_vector(margin, margin),
            make_vector(margin * 2, margin * 2));
        do_box_region(ctx, ids[2], region);

        if (is_click_possible(ctx, ids[2]) || is_region_active(ctx, ids[2]))
        {
            override_mouse_cursor(ctx, ids[2], FOUR_WAY_ARROW_CURSOR);
        }
        if (detect_drag(ctx, ids[2], button))
        {
            vector2d q = canvas_to_scene(canvas2, get_mouse_position(ctx));
            vector3d sp = get_slice_positions(canvas3);
            sp[axes[0]] =
                round_slice_position(canvas3.scene().slicing[axes[0]], q[0]);
            sp[axes[1]] =
                round_slice_position(canvas3.scene().slicing[axes[1]], q[1]);
            set_slice_position(canvas3, sp);
        }
    }
}

void draw_slice_line(
    sliced_3d_canvas& canvas3, canvas& canvas2, unsigned principal_axis,
    rgba8 const& color, line_style const& style)
{
    draw_scene_line(canvas2, color, style,
        sliced_3d_scene_axis_to_canvas_axis(
            canvas3.slice_axis(), principal_axis),
        get_slice_positions(canvas3)[principal_axis]);
}

}
