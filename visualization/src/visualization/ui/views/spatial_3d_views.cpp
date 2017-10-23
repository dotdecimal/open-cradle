#include <visualization/ui/views/spatial_3d_views.hpp>

#include <alia/ui/utilities.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/canvas.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>
#include <cradle/gui/displays/inspection.hpp>
#include <cradle/gui/displays/views/sliced_3d_view.hpp>

namespace visualization {

// This is part of the scene graph but isn't a normal object.
// It specifies the geometry of the scene, so there's only one per graph.
struct spatial_3d_scene_geometry_object
{
    keyed_data<patient_position_type> patient_position;
    keyed_data<sliced_scene_geometry<3> > geometry;
};

struct spatial_3d_scene_graph
{
    // pointer to the geometry for this scene
    spatial_3d_scene_geometry_object* scene_geometry;
    // linked list of objects for rendering on projected views
    spatial_3d_scene_graph_projected_3d_object* projected_3d_objects;
    // linked list of objects for rendering on sliced views
    spatial_3d_scene_graph_sliced_object* sliced_objects;
    // linked list of inspectable objects
    spatial_3d_scene_graph_inspectable_object* inspectable_objects;
};

void static
reset_scene_graph(spatial_3d_scene_graph& scene_graph)
{
    scene_graph.scene_geometry = nullptr;
    scene_graph.projected_3d_objects = nullptr;
    scene_graph.sliced_objects = nullptr;
    scene_graph.inspectable_objects = nullptr;
}

void
set_scene_geometry(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    accessor<patient_position_type> const& patient_position)
{
    spatial_3d_scene_geometry_object* object;
    get_cached_data(ctx, &object);
    // Only update the graph on refresh passes.
    if (is_refresh_pass(ctx))
    {
        scene_graph.scene_geometry = object;
        refresh_accessor_clone(object->geometry, scene_geometry);
        refresh_accessor_clone(object->patient_position, patient_position);
    }
}

void
add_projected_3d_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_projected_3d_object* object)
{
    object->next_ = scene_graph.projected_3d_objects;
    scene_graph.projected_3d_objects = object;
}

void
add_sliced_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_sliced_object* object,
    canvas_layer layer)
{
    object->layer_ = layer;
    object->next_ = scene_graph.sliced_objects;
    scene_graph.sliced_objects = object;
}

void
add_inspectable_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_inspectable_object* object)
{
    object->next_ = scene_graph.inspectable_objects;
    scene_graph.inspectable_objects = object;
}

struct spatial_3d_inspection_overlay_data
{
    // current inspection_position, in scene coordinates
    optional<vector3d> inspection_position;
    // used for positioning/rendering the actual overlay
    popup_positioning positioning;
    value_smoother<float> popup_intensity;
};

void static
do_inspection_overlay(
    gui_context& ctx, spatial_3d_scene_graph const& scene_graph,
    sliced_3d_canvas& c3d, embedded_canvas& c2d)
{
    spatial_3d_inspection_overlay_data* overlay;
    if (get_data(ctx, &overlay))
        reset_smoothing(overlay->popup_intensity, 0.f);

    alia_untracked_if (is_refresh_pass(ctx))
    {
        auto position = c2d.mouse_position();
        overlay->inspection_position =
            position
          ? some(unslice(canvas_to_scene(c2d, get(position)), c3d.slice_axis(),
                get_slice_position(c3d)))
          : none;
    }
    alia_end

    auto inspection_position =
        unwrap_optional(in(overlay->inspection_position));

    // Collect inspection reports from all the inspectable objects in the
    // scene graph.
    auto reports =
        gui_map_scene_graph<optional<spatial_3d_inspection_report> >(ctx,
            [&](gui_context& ctx,
                spatial_3d_scene_graph_inspectable_object const& object)
            {
                return object.inspect(ctx, inspection_position);
            },
            scene_graph.inspectable_objects);

    // Filter out empty reports.
    auto valid_reports =
        gui_apply(ctx,
            filter_optionals<spatial_3d_inspection_report>,
            reports);

    // We only want to show the overlay if we have valid reports.
    bool show_overlay =
        is_gettable(valid_reports) && !get(valid_reports).empty();

    alia_untracked_if (show_overlay)
    {
        alia_untracked_if (is_refresh_pass(ctx))
        {
            set_active_overlay(ctx, overlay);
        }
        alia_end

        alia_untracked_if (is_render_pass(ctx))
        {
            scoped_transformation st(ctx);
            c2d.set_canvas_coordinates();
            auto box =
                make_box(get_integer_mouse_position(ctx),
                    make_vector(as_layout_size(0), as_layout_size(0)));
            position_overlay(ctx, overlay->positioning, box);
        }
        alia_end
    }
    alia_end

    float popup_intensity =
        smooth_raw_value(ctx, overlay->popup_intensity,
            overlay->inspection_position ? 1.f : 0.f,
            animated_transition(default_curve, 250));

    alia_if (popup_intensity > 0 && show_overlay)
    {
        scoped_transformation st(ctx);
        c2d.set_canvas_coordinates();
        {
            nonmodal_popup popup(ctx, overlay, overlay->positioning);
            scoped_surface_opacity scoped_opacity(ctx, popup_intensity);
            panel panel(ctx, text("transparent-overlay"));
            grid_layout grid(ctx);
            for_each(ctx,
                [&](gui_context& ctx, size_t index,
                    accessor<spatial_3d_inspection_report> const& report)
                {
                    grid_row row(grid);
                    do_text(ctx, _field(report, label));
                    do_styled_text(ctx,
                        text("value"),
                        _field(report, value),
                        RIGHT);
                    do_styled_text(ctx,
                        text("units"),
                        _field(report, units),
                        LEFT);
                },
                valid_reports);
        }
    }
    alia_end
}

// spatial_3d_projected_view

void
spatial_3d_projected_view::initialize(
    spatial_3d_view_controller* controller,
    spatial_3d_scene_graph const* scene_graph)
{
    controller_ = controller;
    scene_graph_ = scene_graph;
}

string const&
spatial_3d_projected_view::get_type_id() const
{
    static string const type_id = "projected_view";
    return type_id;
}

string const&
spatial_3d_projected_view::get_type_label(
    null_display_context const& display_ctx)
{
    static string const type_label = "3D";
    return type_label;
}

indirect_accessor<string>
spatial_3d_projected_view::get_view_label(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id)
{
    return make_indirect(ctx, text("3D"));
}

// TODO: Understand all this and clean it up...

static double max = 0.;

void static
compute_view_vectors(vector3d &view_dir, vector3d &view_up,
    patient_position_type patient_position)
{
    view_dir = make_vector(1., 0., 0.);
    view_up = make_vector(0., 0., 1.);
    switch (patient_position)
    {
        case patient_position_type::HFS:
            // TODO: Why is this commented out?!
            //break;
        case patient_position_type::HFP:
            view_dir = make_vector(-1., 0., 0.);
            view_up = make_vector(0., 0., -1.);
            break;
        case patient_position_type::FFS:
            view_dir = make_vector(-1., 0., 0.);
            view_up = make_vector(0., 0., -1.);
            break;
        case patient_position_type::FFP:
            view_dir = make_vector(1., 0., 0.);
            view_up = make_vector(0., 0., -1.);
            break;
        case patient_position_type::HFDR:
            view_dir = make_vector(0., -1., 0.);
            view_up = make_vector(0., 0., 1.);
            break;
        case patient_position_type::HFDL:
            view_dir = make_vector(0., 1., 0.);
            view_up = make_vector(0., 0., 1.);
            break;
        case patient_position_type::FFDR:
            view_dir = make_vector(0., -1., 0.);
            view_up = make_vector(0., 0., -1.);
            break;
        case patient_position_type::FFDL:
            view_dir = make_vector(0., 1., 0.);
            view_up = make_vector(0., 0., -1.);
            break;
        default:
            break;
    }
}

// Constructor for setting up the default 3d view
multiple_source_view static
make_projected_3d_view(
    box3d const& scene_box,
    vector3d const& direction,
    vector3d const& up,
    vector3d const& center,
    box<2,double> const& display_surface)
{
    return
        multiple_source_view(
            center,
            display_surface,
            direction,
            make_vector(0., 0.),
            up);
}

// Sets the initial zoom level to the extents of the scene
void fit_zoom_to_view(
    box<3,double> const& scene_box,
    accessor<box<2,double> > const& display_surface)
{
    //double max = 0.;
    std::vector<vector3d> corners;
    corners.push_back(scene_box.corner);
    corners.push_back(scene_box.corner + scene_box.size);
    for (auto &v : corners)
    {
        for (int i=0; i<2; ++i)
        {
            if (-v[i] > max)
            {
                max = -v[i];
            }
            if (v[i] > max)
            {
                max = v[i];
            }
        }
    }

    // Transform view
    set(display_surface, make_box(make_vector(-max, -max), make_vector(2. * max, 2. * max)));
}

// Tool for rotating the display within 3d view
void static
apply_rotating_tool(
    projected_canvas& canvas,
    multiple_source_view const& view,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    mouse_button button)
{
    // Get gui context, widget id, and set mouse cursor
    gui_context& ctx = canvas.get_embedded_canvas().context();
    widget_id id = canvas.get_embedded_canvas().id();
    if (is_drag_in_progress(ctx, id, button))
        override_mouse_cursor(ctx, id, FOUR_WAY_ARROW_CURSOR);

    // Perform rotation
    if (detect_drag(ctx, id, button))
    {
        // Get surface region
        auto surface_region =
            region_to_surface_coordinates(ctx,
                box<2,double>(canvas.get_embedded_canvas().region()));
        double aspect = surface_region.size[0] / surface_region.size[1];
        multiple_source_view fixed_view =
            scale_view_to_canvas(canvas.get_embedded_canvas(), view);

        vector2d drag_delta = get_drag_delta(ctx);
        //auto scene_size = canvas.get_embedded_canvas().scene_box().size;
        //double scale = alia::pi / std::min(scene_size[0], scene_size[1]);
        double scale = alia::pi / 120.;

        double x_rot = drag_delta[1] * scale;
        double y_rot = -drag_delta[0] * scale;

        vector3d right = unit(cross(fixed_view.direction, fixed_view.up));

        auto rotation =
            rotation_about_axis(right, angle<double,radians>(x_rot)) *
            rotation_about_axis(fixed_view.up, angle<double,radians>(y_rot));

        //auto rotation =
        //    rotation_about_axis(right, angle<double,radians>(x_rot)) *
        //    rotation_about_axis(fixed_view.up, angle<double,radians>(y_rot));

        // Transform view
        multiple_source_view v = fixed_view;
        set(direction, transform_vector(rotation, v.direction));
        set(up, unit(cross(cross(v.direction, v.up), v.direction)));
    }
}

// Tool for panning the display within 3d view
void static
apply_panning_tool(
    projected_canvas& canvas,
    multiple_source_view const& view,
    accessor<vector3d> const& center,
    double& factor,
    mouse_button button)
{
    double move_factor = 1.5 * factor;
    double max_distance = 9999999999;
    // Get gui context, widget id, and set mouse cursor
    gui_context& ctx = canvas.get_embedded_canvas().context();
    widget_id id = canvas.get_embedded_canvas().id();
    if (is_drag_in_progress(ctx, id, button))
        override_mouse_cursor(ctx, id, FOUR_WAY_ARROW_CURSOR);

    // Pan camera location based on up vector
    if (detect_drag(ctx, id, button))
    {
        // Compute movement vector
        vector2d curr = get_mouse_position(ctx);
        vector2d prev = curr - get_drag_delta(ctx);
        vector3d p1 = canvas_to_world(canvas, prev);
        vector3d p2 = canvas_to_world(canvas, curr);
        vector3d move = p2 - p1;

        if (view.center[0] + -move[0] * move_factor <= max_distance &&
            view.center[1] + -move[1] * move_factor <= max_distance &&
            view.center[2] + -move[2] * move_factor <= max_distance)
        {
            if (view.center[0] + -move[0] * move_factor >= -max_distance &&
                view.center[1] + -move[1] * move_factor >= -max_distance &&
                view.center[2] + -move[2] * move_factor >= -max_distance)
            {
                //do_debug_notification(ctx, text(to_string(move).c_str()));
                // Update camera position by movement vector
                set(center, view.center + -move * move_factor);
            }
        }
    }
}

// Sets up a canvas to reset to the default zoom/pan/rotate on a double click of button
void static
apply_reset_tool(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    //set(direction, make_vector(1., 0., 0.));
    //set(up, make_vector(0., 0., 1.));
    set(center, get_center(scene_box));
}

void static
view_reset_front(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(0., 1., 0.));
        set(up, make_vector(0., 0., 1.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_left(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(-1., 0., 0.));
        set(up, make_vector(0., 0., 1.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_right(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(1., 0., 0.));
        set(up, make_vector(0., 0., 1.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_back(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(0., -1., 0.));
        set(up, make_vector(0., 0., 1.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_top(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(0., 0., -1.));
        set(up, make_vector(0., -1., 0.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_bottom(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(0., 0., 1.));
        set(up, make_vector(0., -1., 0.));
        set(center, get_center(scene_box));
    }
}

void static
view_reset_iso(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<vector3d> const& direction,
    accessor<vector3d> const& up,
    accessor<vector3d> const& center,
    accessor<box<2,double> > const& display_surface)
{
    // Transform view
    fit_zoom_to_view(scene_box, display_surface);
    if(is_gettable(direction))
    {
        set(direction, make_vector(0.612375, 0.612375, -0.5));
        set(up, make_vector(0., 0., 1.));
        set(center, get_center(scene_box));
    }
}

// Tool for zooming the display within 3d view
void static
apply_zooming_tool(
    projected_canvas& canvas,
    multiple_source_view const& view,
    box<3,double> const& scene_box,
    accessor<box<2,double> > const& display_surface,
    accessor<vector3d> const& center,
    mouse_button button)
{
    gui_context& ctx = canvas.get_embedded_canvas().context();
    auto zoom_factor = get_state(ctx, double(1.2));
    widget_id id = canvas.get_embedded_canvas().id();
    multiple_source_view fixed_view = scale_view_to_canvas(canvas.get_embedded_canvas(), view);

    alia_if (detect_drag(ctx, id, button))
    {
        // Disabled due to assertion failures.
        auto surface_region = canvas.get_embedded_canvas().region();
        double max_zoom = surface_region.size[0] / 60;

        auto bsize = get(zoom_factor) * scene_box.size;

        //auto center = get_state(ctx, vector3d(scene_box.corner + 0.5 * scene_box.size));
        //auto center = get_state(ctx, view.new_center);

        auto expanded_scene = make_box(get(center) - 0.5 * bsize, bsize);
        if(is_gettable(center))
        {
            auto expanded_scene = make_box(get(center) - 0.5 * bsize, bsize);

            auto fit_view = fit_view_to_scene(expanded_scene, view);
            set(display_surface, fit_view.display_surface);
        }

        // Prevent zoom locking when window is resized smaller and canvas becomes too large
        if (fixed_view.display_surface.corner[0] < -max * max_zoom && fixed_view.display_surface.corner[1] < -max * max_zoom)
        {
            fit_zoom_to_view(scene_box, display_surface);
        }

        // Perform rotation
        {
            vector2d drag_delta = -get_drag_delta(ctx);

            double y_mov = drag_delta[1] * alia::pi / 60.;
            if(get(zoom_factor) + y_mov > .001 && get(zoom_factor) + y_mov < 10)
            {
                set(zoom_factor, get(zoom_factor) + y_mov);
            }
        }
    }
    alia_end
}

void
apply_view_resets(
    gui_context& ctx, projected_canvas& pc, multiple_source_view const& view,
    box3d const& scene_box, accessor<projected_3d_view_state> const& state)
{
    if(detect_key_press(ctx, key_code('1'), KMOD_CTRL))
        view_reset_front(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('2'), KMOD_CTRL))
        view_reset_left(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('3'), KMOD_CTRL))
        view_reset_right(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('4'), KMOD_CTRL))
        view_reset_back(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('5'), KMOD_CTRL))
        view_reset_top(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('6'), KMOD_CTRL))
        view_reset_bottom(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
    if(detect_key_press(ctx, key_code('7'), KMOD_CTRL))
        view_reset_iso(pc, view, scene_box, _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));
}

struct sortable_projected_object
{
    spatial_3d_scene_graph_projected_3d_object const* ptr;
    double z;
};

void static
do_projected_view_3d_content(
    gui_context& ctx,
    projected_canvas& pc,
    spatial_3d_scene_graph const& scene_graph)
{
    // First render any fully opaque objects.
    {
        naming_context nc(ctx);
        for (auto const* object = scene_graph.projected_3d_objects; object;
            object = object->next_)
        {
            named_block nb(nc, get_id(object->id_));
            auto opacity = object->get_opacity(ctx);
            alia_if (opacity == in(1))
            {
                object->render(ctx, pc);
            }
            alia_end
        }
    }

    // Now collect pointers to all the partial transparent objects and sort them according
    // to their distance from the camera. (This is an imperfect sort and there's really no
    // way to do this perfectly without potentially splitting individual triangles. This
    // is just an attempt to make things look good enough until depth peeling can be
    // implemented.)
    std::vector<sortable_projected_object> transparent_objects;
    {
        naming_context nc(ctx);
        for (auto const* object = scene_graph.projected_3d_objects; object;
            object = object->next_)
        {
            named_block nb(nc, get_id(object->id_));
            auto opacity = object->get_opacity(ctx);
            auto z = object->get_z_depth(ctx, pc);
            if (opacity.is_gettable() && opacity.get() != 1 && z.is_gettable())
            {
                sortable_projected_object sortable;
                sortable.ptr = object;
                sortable.z = z.get();
                transparent_objects.push_back(sortable);
            }
        }
    }
    std::sort(
        transparent_objects.begin(), transparent_objects.end(),
        [ ](auto const& a, auto const& b)
        {
            return a.z < b.z;
        });

    // Disable depth writes because we don't actually want the objects fully occluding
    // each other.
    if (is_render_pass(ctx))
        pc.disable_depth_write();

    // Render the transparent objects.
    {
        naming_context nc(ctx);
        for (auto const& sortable : transparent_objects)
        {
            auto const* object = sortable.ptr;
            named_block nb(nc, get_id(object->id_));
            object->render(ctx, pc);
        }
    }

    if (is_render_pass(ctx))
        pc.enable_depth_write();
}

void
do_projected_3d_view(
    gui_context& ctx,
    spatial_3d_view_controller& controller,
    spatial_3d_scene_graph const& scene_graph,
    accessor<projected_3d_view_state> const& state,
    layout const& layout_spec)
{
    auto geometry = make_accessor(&scene_graph.scene_geometry->geometry);

    auto scene_box =
        gui_apply(ctx,
            [ ](auto const& geometry)
            {
                return get_bounding_box(geometry);
            },
            geometry);

    auto view =
        gui_apply(ctx,
            make_projected_3d_view,
            scene_box,
            _field(state, direction),
            _field(state, up),
            _field(state, center),
            _field(state, display_surface));

    alia_if(is_gettable(view))
    {
        embedded_canvas ec;
        auto projected_scene_box =
            make_2d_scene_box_from_view(get(view).center, get(view));
        ec.initialize(
            ctx, projected_scene_box, base_zoom_type::FIT_SCENE,
            storage(in(camera(1., get_center(projected_scene_box)))),
            CANVAS_FLIP_Y);
        ec.begin(layout_spec);

        clear_canvas(ec, rgb8(0x00, 0x00, 0x00));

        projected_canvas pc(ec, get(view));
        clear_depth(pc);
        pc.begin();
        do_projected_view_3d_content(ctx, pc, scene_graph);
        controller.do_projected_tools(ctx, pc);
        pc.end();

        controller.do_2d_tools(ctx, ec);

        widget_id id = pc.get_embedded_canvas().id();
        double magic_num = 600;

        //panning
        {
            if (is_drag_in_progress(ctx, id, MIDDLE_BUTTON))
                override_mouse_cursor(ctx, id, FOUR_WAY_ARROW_CURSOR);

            // Pan camera location based on up vector
            if (detect_drag(ctx, id, MIDDLE_BUTTON))
            {
                // Compute movement vector
                vector2d curr = get_mouse_position(ctx);
                vector2d prev = curr - get_drag_delta(ctx);
                vector3d p1 = canvas_to_world(pc, prev);
                vector3d p2 = canvas_to_world(pc, curr);
                vector3d move = p2 - p1;

                // Turned off pan limiting. Double click reset mitigates the need for this. The limiting causes issues
                // when zoomed in close and the scene box is much larger than the canvas size.
                //if((get(view).center + -move * ec.get_scale_factor()[0])[1] < ec.scene_box().size[0]
                //    && (get(view).center + -move * ec.get_scale_factor()[0])[1] > -ec.scene_box().size[0]
                //    && (get(view).center + -move * ec.get_scale_factor()[0])[2] < ec.scene_box().size[1]
                //    && (get(view).center + -move * ec.get_scale_factor()[0])[2] > -ec.scene_box().size[1])
                {
                    set(_field(state, center), get(view).center + -move * ec.get_scale_factor()[0]);
                }
            }
        }

        //zooming
        {
            if (is_drag_in_progress(ctx, id, RIGHT_BUTTON))
                override_mouse_cursor(ctx, id, UP_DOWN_ARROW_CURSOR);

            if (detect_drag(ctx, id, RIGHT_BUTTON) && is_gettable(state))
            {
                auto y_mov = get_drag_delta(ctx)[1];
                auto tmp = get(_field(state, display_surface));

                tmp.size[0] -= ((magic_num/100) * y_mov);
                tmp.size[1] -= ((magic_num/100) * y_mov);
                tmp.corner[0] += (((magic_num/100) * y_mov) / 2);
                tmp.corner[1] += (((magic_num/100) * y_mov) / 2);

                if((magic_num / tmp.size[0]) < 1000
                    && (magic_num / tmp.size[0]) > .1)
                {
                    set(_field(state, display_surface), tmp);
                }
            }
        }

        //rotating
        apply_rotating_tool(pc, get(view), _field(state, direction), _field(state, up), LEFT_BUTTON);

        //view resets
        apply_view_resets(ctx, pc, get(view), get(scene_box), state);
        if (detect_double_click(ec.context(), ec.id(), LEFT_BUTTON))
            apply_reset_tool(pc, get(view), get(scene_box), _field(state, direction), _field(state, up), _field(state, center), _field(state, display_surface));

        ec.end();

        controller.do_projected_layered_ui(ctx);
    }
    alia_else
    {
        do_empty_display_panel(ctx);
    }
    alia_end
}

void
spatial_3d_projected_view::do_view_content(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id,
    bool is_preview)
{
    auto* scene_geometry = scene_graph_->scene_geometry;
    alia_if (scene_geometry)
    {
        auto geometry = make_accessor(&scene_geometry->geometry);

        auto center =
            gui_apply(ctx,
                [ ](auto const& geometry)
                {
                    return get_center(get_bounding_box(geometry));
                },
                geometry);

        alia_if (is_gettable(center))
        {
            vector3d view_dir = make_vector(0.612375, 0.612375, -0.5);
            vector3d view_up = make_vector(0., 0., 1.);

            // TODO: Figure this stuff out and clean it up...
            auto max = 600.; // Default value to make sure the zoom level isnt ridiculously close. 3d_view does/should override this.
            auto view_state =
                get_state(ctx,
                    projected_3d_view_state(
                        view_dir,
                        view_up,
                        get(center),
                        make_box(
                            make_vector(-max, -max),
                            make_vector(2. * max, 2. * max))));

            do_projected_3d_view(ctx,
                *controller_,
                *scene_graph_,
                view_state,
                GROW | UNPADDED);
        }
        alia_end
    }
    alia_end
}

// spatial_3d_sliced_view

struct view_side_labels
{
    string left;
    string right;
    string upper;
    string lower;
};

// Get the labels for the sides of a view based on the patient orientation and
// the axis that the view is looking down.
view_side_labels static
get_anatomical_side_labels(patient_position_type position, unsigned view_axis)
{
    vector<3,char const*> left_labels =  make_vector<char const*>("A", "R", "R");
    vector<3,char const*> right_labels = make_vector<char const*>("P", "L", "L" );
    vector<3,char const*> upper_labels = make_vector<char const*>("S", "S", "A" );
    vector<3,char const*> lower_labels = make_vector<char const*>("I", "I", "P" );
    switch (position)
    {
        case patient_position_type::HFS:
            // Do nothing
            break;
        case patient_position_type::HFP:
            left_labels =  make_vector<char const*>("P", "L", "L" );
            right_labels = make_vector<char const*>("A", "R", "R" );
            upper_labels = make_vector<char const*>("S", "S", "P" );
            lower_labels = make_vector<char const*>("I", "I", "A" );
            break;
        case patient_position_type::FFS:
            left_labels =  make_vector<char const*>("A", "L", "L" );
            right_labels = make_vector<char const*>("P", "R", "R" );
            upper_labels = make_vector<char const*>("I", "I", "A" );
            lower_labels = make_vector<char const*>("S", "S", "P" );
            break;
        case patient_position_type::FFP:
            left_labels =  make_vector<char const*>("P", "R", "R" );
            right_labels = make_vector<char const*>("A", "L", "L" );
            upper_labels = make_vector<char const*>("I", "I", "P" );
            lower_labels = make_vector<char const*>("S", "S", "A" );
            break;
        case patient_position_type::HFDR:
        case patient_position_type::HFDL:
        case patient_position_type::FFDR:
        case patient_position_type::FFDL:
        default:
            break;
    }

    assert(view_axis < 3);
    view_side_labels labels;
    labels.left = left_labels[view_axis];
    labels.right = right_labels[view_axis];
    labels.upper = upper_labels[view_axis];
    labels.lower = lower_labels[view_axis];
    return labels;
}

string static
get_anatomical_axis_label(unsigned axis)
{
    char const* axis_labels[] = { "Sagittal", "Coronal", "Transverse" };
    return axis_labels[axis];
}

void
do_anatomical_slice_overlay_label(
    gui_context& ctx,
    sliced_3d_canvas& canvas,
    accessor<patient_position_type> const& position)
{
    {
        panel p(ctx, text("transparent-overlay"), TOP | LEFT);
        do_styled_text(ctx,
            text("heading"),
            gui_apply(ctx,
                get_anatomical_axis_label,
                in(canvas.slice_axis())),
            LEFT);
        do_text(ctx,
            printf(ctx, " %.1f mm", in(get_slice_position(canvas))),
            LEFT);
    }
    auto side_labels =
        gui_apply(ctx,
            get_anatomical_side_labels,
            position,
            in(canvas.slice_axis()));
    {
        panel p(ctx, text("clear-letter-overlay"), CENTER_Y | LEFT);
        do_styled_text(ctx,
            text("heading"),
            _field(side_labels, left),
            LEFT);
    }
    {
        panel p(ctx, text("clear-letter-overlay"), CENTER_Y | RIGHT);
        do_styled_text(ctx,
            text("heading"),
            _field(side_labels, right),
            RIGHT);
    }
    {
        panel p(ctx, text("clear-letter-overlay"), TOP | CENTER_X);
        do_styled_text(ctx,
            text("heading"),
            _field(side_labels, upper),
            CENTER_X);
    }
    {
        panel p(ctx, text("clear-letter-overlay"), BOTTOM | CENTER_X);
        do_styled_text(ctx,
            text("heading"),
            _field(side_labels, lower),
            CENTER_X);
    }
}

struct spatial_3d_sliced_view_controller : sliced_3d_view_controller
{
    void do_content(gui_context& ctx,
        sliced_3d_canvas& c3d, embedded_canvas& c2d) const
    {
        // Collect pointers to all the sliced scene graph objects, sort them
        // by layer, and render them. - There's some potential for performance
        // improvement here by not recomputing this each pass, but I think
        // it's relatively minor, and there needs to be some consideration of
        // how to properly track the inputs, so I'm not bothering with it.
        std::vector<spatial_3d_scene_graph_sliced_object const*> objects;
        for (auto const* object = scene_graph_->sliced_objects; object;
            object = object->next_)
        {
            objects.push_back(object);
        }
        // We need to reverse them (because items are added at the front) and
        // then do a stable sort to preserve the original order within each
        // layer.
        std::reverse(objects.begin(), objects.end());
        std::stable_sort(
            objects.begin(), objects.end(),
            [ ](auto const* a, auto const& b)
            {
                return a->layer_ < b->layer_;
            });
        {
            naming_context nc(ctx);
            for (auto const* object : objects)
            {
                named_block nb(nc, get_id(object->id_));
                object->render(ctx, c3d, c2d);
            }
        }

        controller_->do_sliced_tools(ctx, c3d, c2d);

        alia_if (is_true(_field(state_, show_hu_overlays)))
        {
            // Do the inspection overlay.
            do_inspection_overlay(ctx, *scene_graph_, c3d, c2d);
        }
        alia_end
    }
    void do_overlays(gui_context& ctx,
        sliced_3d_canvas& c3d, embedded_canvas& c2d) const
    {
        do_anatomical_slice_overlay_label(ctx, c3d,
            make_accessor(&scene_graph_->scene_geometry->patient_position));
        controller_->do_sliced_layered_ui(ctx);
    }
    indirect_accessor<sliced_3d_view_state> state_;
    spatial_3d_view_controller* controller_;
    spatial_3d_scene_graph const* scene_graph_;
};

void
spatial_3d_sliced_view::initialize(
    spatial_3d_view_controller* controller,
    spatial_3d_scene_graph const* scene_graph,
    indirect_accessor<sliced_3d_view_state> state,
    unsigned view_axis)
{
    controller_ = controller;
    scene_graph_ = scene_graph;
    state_ = state;
    view_axis_ = view_axis;
}

string const& spatial_3d_sliced_view::get_type_id() const
{
    static string const type_ids[3] =
        { "sliced_view_0", "sliced_view_1", "sliced_view_2" };
    return type_ids[view_axis_];
}

string const&
spatial_3d_sliced_view::get_type_label(
    null_display_context const& display_ctx)
{
    static string type_labels[3] =
        {
            get_anatomical_axis_label(0),
            get_anatomical_axis_label(1),
            get_anatomical_axis_label(2)
        };
    return type_labels[view_axis_];
}

indirect_accessor<string>
spatial_3d_sliced_view::get_view_label(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id)
{
    return
        make_indirect(ctx,
            gui_apply(ctx,
                get_anatomical_axis_label,
                in(view_axis_)));
}

void
spatial_3d_sliced_view::do_view_content(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id,
    bool is_preview)
{
    auto* scene_geometry = scene_graph_->scene_geometry;
    alia_if (scene_geometry)
    {
        spatial_3d_sliced_view_controller view_controller;
        view_controller.scene_graph_ = scene_graph_;
        view_controller.controller_ = controller_;
        view_controller.state_ = state_;
        do_sliced_3d_view(ctx,
            view_controller,
            make_accessor(&scene_geometry->geometry),
            state_,
            in(view_axis_),
            GROW | UNPADDED,
            get_view_flags(make_accessor(&scene_geometry->patient_position)));
    }
    alia_end
}

void
add_spatial_3d_views(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    spatial_3d_views& views,
    spatial_3d_view_controller* controller,
    indirect_accessor<sliced_3d_view_state> state,
    spatial_3d_flag_set flags)
{
    spatial_3d_scene_graph* scene_graph;
    get_cached_data(ctx, &scene_graph);
    // Reset the scene_graph on refresh passes.
    alia_untracked_if (is_refresh_pass(ctx))
    {
        reset_scene_graph(*scene_graph);
    }
    alia_end
    // Ask the controller to generate the contents for the display.
    controller->generate_scene(ctx, *scene_graph);

    // Initialize and add the views.
    for (unsigned i = 0; i != 3; ++i)
    {
        auto* view = &views.sliced[i];
        view->initialize(controller, scene_graph, state, i);
        provider.add_view(view);
    }

    alia_if (!(flags & SPATIAL_3D_NO_PROJECTED_VIEW))
    {
        views.projected.initialize(controller, scene_graph);
        provider.add_view(&views.projected);
    }
    alia_end
}

void
add_sliced_3d_views(display_view_instance_list& views)
{
    views.push_back(display_view_instance("sliced_view_2", "sliced_view_2"));
    views.push_back(display_view_instance("sliced_view_0", "sliced_view_0"));
    views.push_back(display_view_instance("sliced_view_1", "sliced_view_1"));
}

display_view_composition
make_default_spatial_3d_view_composition(string const& label)
{
    display_view_instance_list views;
    add_sliced_3d_views(views);
    views.push_back(display_view_instance("projected_view", "projected_view"));
    return
        display_view_composition(
            "default", label, views,
            display_layout_type::TWO_COLUMNS);
}

display_view_composition
make_default_ortho_bev_view_composition(string const& label)
{
    display_view_instance_list views;
    add_sliced_3d_views(views);
    views.push_back(
        display_view_instance("fixed_projected_view", "fixed_projected_view"));
    return
        display_view_composition(
            "default", label, views,
            display_layout_type::TWO_COLUMNS);
}

display_view_composition
make_spatial_3d_view_composition(string const& label)
{
    display_view_instance_list views;
    add_sliced_3d_views(views);
    views.push_back(display_view_instance("projected_view", "projected_view"));
    return
        display_view_composition(
            "ortho_3d", label, views,
            display_layout_type::TWO_COLUMNS);
}

display_view_composition
make_ortho_bev_view_composition(string const& label)
{
    display_view_instance_list views;
    add_sliced_3d_views(views);
    views.push_back(
        display_view_instance("fixed_projected_view", "fixed_projected_view"));
    return
        display_view_composition(
            "ortho_bev", label, views,
            display_layout_type::TWO_COLUMNS);
}

display_view_composition
make_3d_bev_view_composition(string const& label)
{
    display_view_instance_list views;
    views.push_back(display_view_instance("sliced_view_2", "sliced_view_2"));
    views.push_back(display_view_instance("projected_view", "projected_view"));
    views.push_back(display_view_instance("fixed_projected_view", "fixed_projected_view"));
    return
        display_view_composition(
            "3d_bev", label, views, display_layout_type::COLUMN_PLUS_MAIN);
}

// spatial_3d_fixed_projected_view

void
spatial_3d_fixed_projected_view::initialize(
    fixed_projected_view_controller* controller,
    spatial_3d_scene_graph const* scene_graph)
{
    controller_ = controller;
    scene_graph_ = scene_graph;
}

string const&
spatial_3d_fixed_projected_view::get_type_id() const
{
    return type_id_;
}

string const&
spatial_3d_fixed_projected_view::get_type_label(
    null_display_context const& display_ctx)
{
    return type_label_;
}

indirect_accessor<string>
spatial_3d_fixed_projected_view::get_view_label(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id)
{
    return make_indirect(ctx, text(type_label_.c_str()));
}

void static
do_fixed_projected_view(
    gui_context& ctx,
    fixed_projected_view_controller& controller,
    spatial_3d_scene_graph const& scene_graph)
{
    auto view = controller.get_view_geometry(ctx);

    auto zoom_factor = get_state(ctx, double(1.2));

    auto* scene_geometry = scene_graph.scene_geometry;

    alia_if (scene_geometry)
    {
        auto geometry = make_accessor(&scene_geometry->geometry);

        alia_if (is_gettable(geometry) && is_gettable(view) &&
            is_gettable(zoom_factor))
        {
            auto scene_box = get_bounding_box(get(geometry));

            auto bsize = get(zoom_factor) * scene_box.size;

            auto center = get_state(ctx, vector3d(scene_box.corner + 0.5 * scene_box.size));

            alia_if(is_gettable(center))
            {
                auto expanded_scene = make_box(get(center) - 0.5 * bsize, bsize);

                auto fit_view =
                    gui_apply(ctx, fit_view_to_scene, in(expanded_scene), view);

                embedded_canvas ec;
                auto projected_scene_box =
                    make_2d_scene_box_from_view(get(view).center, get(fit_view));
                ec.initialize(
                    ctx, projected_scene_box, base_zoom_type::FIT_SCENE,
                    storage(in(camera(1., get_center(projected_scene_box)))),
                    CANVAS_FLIP_Y);
                side_rulers rulers(ctx, ec, BOTTOM_RULER | LEFT_RULER,
                    GROW | UNPADDED);

                layered_layout layering(ctx, GROW);
                ec.begin(layout(size(30, 30, EM), GROW | UNPADDED));
                clear_canvas(ec, rgb8(0x00, 0x00, 0x00));

                projected_canvas pc(ec, get(fit_view));
                clear_depth(pc);
                pc.enable_depth_write();
                pc.begin();
                do_projected_view_3d_content(ctx, pc, scene_graph);
                controller.do_projected_tools(ctx, pc);
                pc.end();

                controller.do_2d_tools(ctx, ec);

                //zooming
                alia_untracked_if (detect_drag(ec.context(), ec.id(), RIGHT_BUTTON))
                {
                    vector2d drag_delta = -get_drag_delta(ec.context());
                    double y_mov = drag_delta[1] * alia::pi / 60.;
                    if(get(zoom_factor) + y_mov > .001 && get(zoom_factor) + y_mov < 10)
                        set(zoom_factor, get(zoom_factor) + y_mov);
                }
                alia_end

                alia_untracked_if (detect_double_click(ec.context(), ec.id(), LEFT_BUTTON))
                {
                    // Transform view
                    set(zoom_factor, 1.2);
                    set(center, vector3d(scene_box.corner + 0.5 * scene_box.size));
                }
                alia_end

                widget_id id = pc.get_embedded_canvas().id();

                //panning
                {
                    if (is_drag_in_progress(ctx, id, MIDDLE_BUTTON))
                        override_mouse_cursor(ctx, id, FOUR_WAY_ARROW_CURSOR);

                    // Pan camera location based on up vector
                    if (detect_drag(ctx, id, MIDDLE_BUTTON))
                    {
                        // Compute movement vector
                        vector2d curr = get_mouse_position(ctx);
                        vector2d prev = curr - get_drag_delta(ctx);
                        vector3d p1 = canvas_to_world(pc, prev);
                        vector3d p2 = canvas_to_world(pc, curr);
                        vector3d move = p2 - p1;

                        // Turned off pan limiting. Double click reset mitigates the need for this. The limiting causes issues
                        // when zoomed in close and the scene box is much larger than the canvas size.
                        //if((get(view).center + -move * ec.get_scale_factor()[0])[1] < ec.scene_box().size[0]
                        //    && (get(view).center + -move * ec.get_scale_factor()[0])[1] > -ec.scene_box().size[0]
                        //    && (get(view).center + -move * ec.get_scale_factor()[0])[2] < ec.scene_box().size[1]
                        //    && (get(view).center + -move * ec.get_scale_factor()[0])[2] > -ec.scene_box().size[1])
                        {
                            set(center, get(center) + -move * ec.get_scale_factor()[0]);
                        }
                    }
                }

                alia_untracked_if (is_render_pass(ctx))
                {
                    draw_line(ctx, rgba8(0xa0, 0xa0, 0xa0, 0xff),
                        make_line_style(line_stipple_type::SOLID, 1),
                        make_vector(-1000., 0.), make_vector(1000., 0.));
                    draw_line(ctx, rgba8(0xa0, 0xa0, 0xa0, 0xff),
                        make_line_style(line_stipple_type::SOLID, 1),
                        make_vector(0., -1000.), make_vector(0., 1000.));
                }
                alia_end

                ec.end();

                controller.do_layered_ui(ctx);
            }
            alia_else
            {
                do_empty_display_panel(ctx, GROW);
            }
            alia_end
        }
        alia_else
        {
            layered_layout layering(ctx, GROW);
            do_empty_display_panel(ctx, GROW);
            controller.do_layered_ui(ctx);
        }
        alia_end
    }
    alia_else
    {
        do_empty_display_panel(ctx, GROW);
    }
    alia_end
}

void
spatial_3d_fixed_projected_view::do_view_content(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id,
    bool is_preview)
{
    do_fixed_projected_view(ctx, *controller_, *scene_graph_);
}

void
add_fixed_projected_3d_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    spatial_3d_fixed_projected_view& view,
    fixed_projected_view_controller* controller)
{
    spatial_3d_scene_graph* scene_graph;
    get_cached_data(ctx, &scene_graph);
    // Reset the scene_graph on refresh passes.
    alia_untracked_if (is_refresh_pass(ctx))
    {
        reset_scene_graph(*scene_graph);
    }
    alia_end
    // Ask the controller to generate the contents for the display.
    controller->generate_scene(ctx, *scene_graph);

    view.initialize(controller, scene_graph);
    provider.add_view(&view);
}

}
