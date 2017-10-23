#include <cradle/gui/displays/canvas.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/external/opengl.hpp>
#include <cradle/geometry/multiple_source_view.hpp>

namespace cradle {

camera make_default_camera(box<2,double> const& scene_box)
{
    return cradle::camera(1., get_center(scene_box));
}

double static
zoom_to_fit_scene(layout_vector const& canvas_size,
    vector<2,double> const& scene_size)
{
    return (std::min)(canvas_size[0] / scene_size[0],
        canvas_size[1] / scene_size[1]);
}
double static
zoom_to_fit_scene_height(layout_vector const& canvas_size,
    vector<2,double> const& scene_size)
{
    return canvas_size[1] / scene_size[1];
}
double static
zoom_to_fit_scene_width(layout_vector const& canvas_size,
    vector<2,double> const& scene_size)
{
    return canvas_size[0] / scene_size[0];
}
double static
zoom_to_fill_canvas(layout_vector const& canvas_size,
    vector<2,double> const& scene_size)
{
    return (std::max)(canvas_size[0] / scene_size[0],
        canvas_size[1] / scene_size[1]);
}

// Evaluate a base zoom level to a scale factor (in pixels per scene unit).
vector<2,double>
evaluate_base_zoom(base_zoom_type base_zoom,
    layout_vector const& canvas_size,
    vector<2,double> const& scene_size)
{
    switch (base_zoom)
    {
     case base_zoom_type::STRETCH_TO_FIT:
        return make_vector(
            canvas_size[0] / scene_size[0],
            canvas_size[1] / scene_size[1]);
     case base_zoom_type::FIT_SCENE:
     default:
      {
        auto zoom = zoom_to_fit_scene(canvas_size, scene_size);
        return make_vector(zoom, zoom);
      }
     case base_zoom_type::FIT_SCENE_HEIGHT:
      {
        auto zoom = zoom_to_fit_scene_height(canvas_size, scene_size);
        return make_vector(zoom, zoom);
      }
     case base_zoom_type::FIT_SCENE_WIDTH:
      {
        auto zoom = zoom_to_fit_scene_width(canvas_size, scene_size);
        return make_vector(zoom, zoom);
      }
     case base_zoom_type::FILL_CANVAS:
      {
        auto zoom = zoom_to_fill_canvas(canvas_size, scene_size);
        return make_vector(zoom, zoom);
      }
    }
}

vector<2,double> static
clamp_camera_position(
    embedded_canvas& canvas, vector<2,double> const& position, bool strict)
{
    vector<2,double> clamped;
    box<2,double> const& scene = canvas.scene_box();
    vector<2,double> scale = canvas.get_scale_factor();
    for (int i = 0; i < 2; ++i)
    {
        double margin =
            strict ? double(canvas.region().size[i]) / 2 / scale[i] : 0;
        if (margin <= scene.size[i] / 2)
        {
            clamped[i] = clamp(position[i], scene.corner[i] + margin,
                get_high_corner(scene)[i] - margin);
        }
        else
            clamped[i] = get_center(scene)[i];
    }
    return clamped;
}

/// \fn double static clamp_zoom_level(embedded_canvas& canvas, double zoom)
/// \ingroup gui_displays
/// Clamps the zoom in/out limits.
double static
clamp_zoom_level(embedded_canvas& canvas, double zoom)
{
    // Sets the max_zoom level relative to the canvas size, larger canvas can
    // zoom farther in. Larger view ports (single views/large monitors) can
    // zoom farther in.
    // This max_zoom value must match in the apply_zoom_drag_tool function.
    // Disabled because this is getting run when canvas.region() is not yet
    // valid.
    //auto surface_region = canvas.region();
    //double max_zoom = (surface_region.size[0] > 700) ?
    //    surface_region.size[0] / 100 : surface_region.size[0] / 75;
    double max_zoom = 20.;
    double min_zoom =
        (canvas.flags() & CANVAS_STRICT_CAMERA_CLAMPING) ? 1. : 0.8;
    return clamp(zoom, min_zoom, max_zoom);
}

struct embedded_canvas::data
{
    layout_leaf layout_node;
    cradle::camera default_camera;

    optional<vector<2,double> > next_mouse_position, mouse_position;
};

void embedded_canvas::initialize(
    gui_context& ctx, box<2,double> const& scene_box,
    base_zoom_type base_zoom, optional_storage<cradle::camera> const& camera,
    canvas_flag_set flags)
{
    ctx_ = &ctx;
    if (get_data(ctx, &data_))
        data_->default_camera = make_default_camera(scene_box);
    id_ = get_widget_id(ctx);
    scene_box_ = scene_box;
    base_zoom_ = base_zoom;
    flags_ = flags;
    active_ = false;

    auto resolved_camera = resolve_storage(camera, &data_->default_camera);

    handle_set_value_events(ctx, id_, resolved_camera);

    // If the camera is somehow uninitialized, initialize it.
    if (ctx.event->type == REFRESH_EVENT &&
        !resolved_camera.is_gettable())
    {
        set(resolved_camera, make_default_camera(scene_box));
    }

    camera_ = resolved_camera.is_gettable() ? get(resolved_camera) :
        make_default_camera(scene_box);

    auto clamped_camera_position =
        clamp_camera_position(*this, camera_.position,
            flags & CANVAS_STRICT_CAMERA_CLAMPING);
    // If the camera position is almost equal to the clamped position, don't
    // bother setting it.
    if (!almost_equal(clamped_camera_position, camera_.position, 0.00001))
    {
        camera_.position = clamped_camera_position;
        set(resolved_camera, camera_);
    }

    auto clamped_zoom_level = clamp_zoom_level(*this, camera_.zoom);
    // If the zoom level is almost equal to the clamped level, don't bother
    // setting it.
    if (!almost_equal(clamped_zoom_level, camera_.zoom, 0.00001))
    {
        camera_.zoom = clamped_zoom_level;
        if (is_settable(resolved_camera))
            set(resolved_camera, camera_);
    }
}

void embedded_canvas::begin(layout const& layout_spec)
{
    gui_context& ctx = *ctx_;

    if (is_refresh_pass(ctx))
    {
        data_->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            // What should the default size be?
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0),
            FILL | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data_->layout_node);
        // Update the mouse position.
        data_->mouse_position = data_->next_mouse_position;
    }
    else if (is_render_pass(ctx))
    {
        data_->next_mouse_position =
            is_mouse_inside_box(ctx, box<2,double>(this->region())) ?
            get_mouse_position(ctx) : optional<vector<2,double> >(none);
    }

    do_box_region(ctx, id_, this->region());

    scr_.begin(get_geometry_context(ctx));
    scr_.set(box<2,double>(this->region()));

    st_.begin(get_geometry_context(ctx));
    if (ctx.event->category != REFRESH_CATEGORY)
        this->set_scene_coordinates();

    active_ = true;
}

void embedded_canvas::begin(
    gui_context& ctx, box<2,double> const& scene_box,
    layout const& layout_spec,
    base_zoom_type base_zoom,
    optional_storage<cradle::camera> const& camera,
    canvas_flag_set flags)
{
    this->initialize(ctx, scene_box, base_zoom, camera, flags);
    this->begin(layout_spec);
}

void embedded_canvas::end()
{
    if (active_)
    {
        st_.end();
        scr_.end();
        active_ = false;
    }
}

layout_box const& embedded_canvas::region() const
{
    return data_->layout_node.assignment().region;
}

optional<vector<2,double> > const& embedded_canvas::mouse_position() const
{
    return data_->mouse_position;
}

// Evaluate that given zoom level on the given canvas, yielding a scale factor
// (canvas pixels per scene unit).
static vector<2,double>
evaluate_zoom_level_for_canvas(embedded_canvas const& canvas, double zoom)
{
    auto scale_factor =
        evaluate_base_zoom(canvas.base_zoom(),
            canvas.region().size, canvas.scene_box().size) * zoom;

    // If any scale factors were forced on this canvas, apply them here.
    for (unsigned i = 0; i != 2; ++i)
    {
        if (canvas.forced_scale()[i])
            scale_factor[i] = get(canvas.forced_scale()[i]);
    }

    return scale_factor;
}

vector<2,double> embedded_canvas::get_scale_factor() const
{
    return evaluate_zoom_level_for_canvas(*this, camera_.zoom);
}

void embedded_canvas::set_scene_coordinates()
{
    vector<2,double> absolute_scale = this->get_scale_factor();
    vector<2,double> scale = make_vector(
        flip_x() ? -absolute_scale[0] : absolute_scale[0],
        flip_y() ? -absolute_scale[1] : absolute_scale[1]);
    st_.set(
        translation_matrix(
            vector<2,double>(region().corner) +
            vector<2,double>(region().size) / 2) *
        scaling_matrix(scale) *
        translation_matrix(-vector<2,double>(camera_.position)));
}

void embedded_canvas::set_canvas_coordinates()
{
    st_.restore();
}

vector<2,double> canvas_to_scene(embedded_canvas& c, vector<2,double> const& p,
    vector<2,double> absolute_scale, vector<2,double> const& camera_position)
{
    vector<2,double> inverse_abs_scale = make_vector(
        1. / absolute_scale[0], 1. / absolute_scale[1]);
    vector<2,double> scale = make_vector(
        c.flip_x() ? -inverse_abs_scale[0] : inverse_abs_scale[0],
        c.flip_y() ? -inverse_abs_scale[1] : inverse_abs_scale[1]);
    return
        (p - (vector<2,double>(c.region().corner) +
        vector<2,double>(c.region().size) / 2)) * scale +
        camera_position;
}
vector<2,double> scene_to_canvas(embedded_canvas& c, vector<2,double> const& p,
    vector<2,double> absolute_scale, vector<2,double> const& camera_position)
{
    vector<2,double> scale = make_vector(
        c.flip_x() ? -absolute_scale[0] : absolute_scale[0],
        c.flip_y() ? -absolute_scale[1] : absolute_scale[1]);
    return
        (p - camera_position) * scale +
        (vector<2,double>(c.region().corner) +
        vector<2,double>(c.region().size) / 2);
}
vector<2,double> canvas_to_scene(embedded_canvas& c, vector<2,double> const& p)
{
    return canvas_to_scene(c, p, c.get_scale_factor(), c.camera().position);
}
vector<2,double> scene_to_canvas(embedded_canvas& c, vector<2,double> const& p)
{
    return scene_to_canvas(c, p, c.get_scale_factor(), c.camera().position);
}

void set_camera(embedded_canvas& canvas, cradle::camera const& new_camera)
{
    issue_set_value_event(canvas.context(), canvas.id(), new_camera);
}

static cradle::camera set_camera_position(
    cradle::camera const& camera, vector<2,double> const& position)
{
    cradle::camera c = camera;
    c.position = position;
    return c;
}

static cradle::camera set_camera_zoom(
    cradle::camera const& camera, double zoom)
{
    cradle::camera c = camera;
    c.zoom = zoom;
    return c;
}

/// \fn void apply_panning_tool(embedded_canvas& canvas, mouse_button button)
/// \ingroup gui_displays
/// Sets up a canvas to have click and drag panning functionality with button
void apply_panning_tool(embedded_canvas& canvas, mouse_button button)
{
    dataless_ui_context& ctx = canvas.context();
    widget_id id = canvas.id();
    if (is_drag_in_progress(ctx, id, button))
    {
        override_mouse_cursor(ctx, id, FOUR_WAY_ARROW_CURSOR);
    }
    if (detect_drag(ctx, id, button))
    {
        set_camera(canvas, set_camera_position(canvas.camera(),
            canvas.camera().position - get_drag_delta(ctx)));
    }
}

//void draw_checker_background(embedded_canvas& canvas, rgba8 const& color1,
//    rgba8 const& color2, double spacing)
//{
//    // TODO: This won't handle changes in the color arguments.
//    image<rgba8>* img;
//    if (get_data(canvas.context(), &img))
//    {
//        create_image(*img, make_vector(2, 2));
//        img->view.pixels[0] = img->view.pixels[3] = color1;
//        img->view.pixels[1] = img->view.pixels[2] = color2;
//    }
//    vector<2,double> p;
//    box<2,double> region;
//    scoped_transformation st;
//    if (is_render_pass(canvas.context()))
//    {
//        vector<2,double> const corner0 = canvas_to_scene(canvas,
//            vector<2,double>(canvas.region().corner));
//        vector<2,double> const corner1 = canvas_to_scene(canvas,
//            vector<2,double>(get_high_corner(canvas.region())));
//        for (int i = 0; i < 2; ++i)
//        {
//            p[i] = std::floor(std::min(corner0[i], corner1[i]) / spacing);
//            region.corner[i] = p[i];
//            region.size[i] = std::ceil(std::fabs((corner1 - corner0)[i]) /
//                spacing + 1);
//        }
//        st.begin(canvas.context().geometry);
//        st.set(scaling_transformation(vector<2,double>(spacing, spacing)));
//    }
//    draw_image_region<unsigned>(
//        canvas.context(),
//        p,
//        make_interface(img->view), 0,
//        region,
//        rgba8(0xff, 0xff, 0xff, 0xff),
//        surface::TILED_IMAGE);
//}

void draw_grid_lines_for_axis(
    embedded_canvas& canvas, box<2,double> const& box,
    rgba8 const& color, line_style const& style, unsigned axis, double spacing,
    unsigned skip)
{
    // TODO: skipping
    double start = std::ceil(box.corner[axis] / spacing) * spacing,
        end = get_high_corner(box)[axis];
    unsigned other_axis = 1 - axis;
    vector<2,double> p0, p1;
    p0[other_axis] = box.corner[other_axis];
    p0[axis] = start;
    p1[other_axis] = get_high_corner(box)[other_axis];
    p1[axis] = start;
    surface& surface = *canvas.context().surface;
    while (p0[axis] <= end)
    {
        draw_line(canvas.context(), color, style, p0, p1);
        p0[axis] += spacing;
        p1[axis] += spacing;
    }
}

void draw_grid_lines(embedded_canvas& canvas, box<2,double> const& box,
    rgba8 const& color, line_style const& style, double spacing,
    unsigned skip)
{
    if (is_render_pass(canvas.context()))
    {
        draw_grid_lines_for_axis(canvas, box, color, style, 0, spacing, skip);
        draw_grid_lines_for_axis(canvas, box, color, style, 1, spacing, skip);
    }
}

static void
draw_rotated_number(
    gui_context& ctx,
    vector<2,double> const& p, double angle,
    rgba8 const& color,
    double value)
{
    scoped_transformation st(get_geometry_context(ctx),
        translation_matrix(p) * rotation_matrix(angle));
    draw_text(ctx, printf(ctx, "%g", in(value)), make_vector(0., 0.),
        ALIGN_TEXT_TOP);
}

static void label_ruler(
    gui_context& ctx, int iterations,
    double initial_value, double value_increment,
    vector<2,double> const& initial_location,
    vector<2,double> const& location_increment,
    bool draw_tenth_ticks, bool label_half_ticks,
    vector<2,double> const& full_tick_offset,
    vector<2,double> const& half_tick_offset,
    vector<2,double> const& tenth_tick_offset,
    double text_rotation_angle,
    vector<2,double> const& text_offset,
    bool draw_mouse,
    vector<2,double> const& mouse_position,
    vector<2,double> const& mouse_offset,
    vector<2,double> const& mouse_lateral_offset,
    rgba8 const& color)
{
    vector<2,double> location = initial_location;
    double value = initial_value;

    double minor_value_inc = value_increment * .1;
    vector<2,double> minor_location_inc = location_increment * .1;

    alia_for (int i = 0; i < iterations; ++i)
    {
        draw_line(ctx, color, line_style(1, solid_line),
            location, location + full_tick_offset);
        value = std::floor(value / minor_value_inc + 0.5) * minor_value_inc;
        if (value == 0) value = 0; // eliminate -0's
        draw_rotated_number(ctx, location + text_offset, text_rotation_angle,
            color, value);
        location += minor_location_inc;
        value += minor_value_inc;

        if (draw_tenth_ticks)
        {
            for (int i = 0; i < 4; ++i)
            {
                draw_line(ctx, color, line_style(1, solid_line),
                    location, location + tenth_tick_offset);
                location += minor_location_inc;
                value += minor_value_inc;
            }
        }
        else
        {
            location += minor_location_inc * 4;
            value += minor_value_inc * 4;
        }

        draw_line(ctx, color, line_style(1, solid_line),
            location, location + half_tick_offset);
        if (label_half_ticks)
        {
            value = std::floor(value / minor_value_inc + 0.5) *
                minor_value_inc;
            draw_rotated_number(
                ctx, location + text_offset, text_rotation_angle,
                color, value);
        }
        location += minor_location_inc;
        value += minor_value_inc;

        if (draw_tenth_ticks)
        {
            for (int i = 0; i < 4; ++i)
            {
                draw_line(ctx, color, line_style(1, solid_line),
                    location, location + tenth_tick_offset);
                location += minor_location_inc;
                value += minor_value_inc;
            }
        }
        else
        {
            location += minor_location_inc * 4;
            value += minor_value_inc * 4;
        }
    }
    alia_end

    if (draw_mouse && is_render_pass(ctx))
    {
        glColor4ub(color.r, color.g, color.b, color.a);
        glBegin(GL_POLYGON);
        vector2d p = mouse_position;
        glVertex2d(p[0], p[1]);
        p = mouse_position + mouse_offset + mouse_lateral_offset;
        glVertex2d(p[0], p[1]);
        p = mouse_position + mouse_offset - mouse_lateral_offset;
        glVertex2d(p[0], p[1]);
        glEnd();
    }
}

// Calculate the location and values of the ruler marks and store the
// information in the provided variables.
void static
calculate_ruler_values(
    embedded_canvas& canvas,
    layout_box const& region,
    unsigned principal_axis,
    double& initial_value, double& value_inc,
    double& initial_location, double& location_inc,
    int& n_major_ticks,
    double scale)
{
    // NOTE: "ruler space" refers to the current coordinate frame of the
    // canvas.  The coordinates labeled on the rulers refer to this space.

    // the location of some key points on the canvas, transformed into
    // ruler space
    vector<2,double> canvas_origin =
        canvas_to_scene(canvas,
            vector2d(canvas.region().corner) + make_vector(0., 0.));
    vector<2,double> canvas_axis =
        canvas_to_scene(canvas, vector2d(canvas.region().corner) +
            (principal_axis != 0 ? make_vector(0., 1.) : make_vector(1., 0.)));

    // get unit vectors describing the canvas axes in ruler space
    vector<2,double> axis_dir = unit(canvas_axis - canvas_origin);

    // take the absolute values of the vector components
    axis_dir[0] = std::fabs(axis_dir[0]);
    axis_dir[1] = std::fabs(axis_dir[1]);

    // NOTE: The word "value" below refers to the numerical values on the
    // rulers (i.e, the location in ruler space).  "location" refers to the
    // position on the canvas.

    double value_at_origin =
        dot(canvas_origin, axis_dir) * scale;

    double value_inc_per_pixel =
        dot(canvas_axis, axis_dir) * scale - value_at_origin;

    double values_per_pixel = std::fabs(value_inc_per_pixel);

    // determine the major tick spacing (in values)
    // TODO: This should use physicals units, or units relative to the font
    // size, not pixels.
    double major_tick_spacing = 1000000;
    while (major_tick_spacing / values_per_pixel > 600)
        major_tick_spacing /= 10;

    // determine the value increment per major tick
    value_inc = value_inc_per_pixel > 0 ?
        major_tick_spacing : -major_tick_spacing;

    // determine the value of the first tick (which will be safely off
    // screen)
    initial_value = std::floor(value_at_origin / value_inc) * value_inc;

    // determine the canvas location of the first tick
    initial_location = double(region.corner[principal_axis]) +
        (initial_value - value_at_origin) / value_inc_per_pixel;

    location_inc = value_inc / value_inc_per_pixel;

    // determine how many major ticks are needed along each axis
    n_major_ticks = static_cast<int>(region.size[principal_axis] /
        std::fabs(location_inc)) + 3;
}

struct sider_ruler_style_info
{
    layout_scalar width;
    double transition_size;
};

void static
read_style_info(dataless_ui_context& ctx, sider_ruler_style_info* info,
    style_search_path const* path)
{
    info->transition_size =
        resolve_absolute_length(get_layout_traversal(ctx), 0,
            get_property(path, "transition-size", UNINHERITED_PROPERTY,
                absolute_length(120, PIXELS)));
    info->width =
        as_layout_size(
            resolve_absolute_length(get_layout_traversal(ctx), 0,
                get_property(path, "ruler-width", UNINHERITED_PROPERTY,
                    absolute_length(2, EM))));
}

void static
draw_side_ruler(
    gui_context& ctx, embedded_canvas& canvas, layout_box const& region,
    rgba8 const& bg_color, rgba8 const& fg_color,
    double scale, ruler_flag_set flags,
    sider_ruler_style_info const& style)
{
    assert(is_render_pass(ctx));

    bool mouse_inside = canvas.mouse_position() ? true : false;

    scoped_clip_region scr;
    scr.begin(get_geometry_context(ctx));

    double text_height =
        get_layout_traversal(ctx).style_info->character_size[1];

    double initial_value, initial_location;
    double value_inc, location_inc;
    int n_major_ticks;

    unsigned principal_axis =
        (flags & BOTTOM_RULER) || (flags & TOP_RULER) ? 0 : 1;

    calculate_ruler_values(canvas, region, principal_axis,
        initial_value, value_inc, initial_location, location_inc,
        n_major_ticks, scale);

    bool const label_half_ticks =
        std::fabs(location_inc) > style.transition_size;
    bool const draw_tenth_ticks = label_half_ticks;

    scr.set(box<2,double>(region));

    draw_filled_box(ctx, bg_color, box<2,double>(region));

    double full_tick = text_height * .75,
        half_tick = text_height * .5,
        minor_tick = text_height * .25,
        mouse_spacing = half_tick + 2,
        mouse_arrow = 6;

    layout_box r = region;

    switch (flags.code & RULER_SIDE_MASK_CODE)
    {
     case BOTTOM_RULER_CODE:
      {
        label_ruler(
            ctx, n_major_ticks, initial_value, value_inc,
            make_vector<double>(initial_location, r.corner[1]),
            make_vector<double>(location_inc, 0),
            draw_tenth_ticks, label_half_ticks,
            make_vector<double>(0, full_tick),
            make_vector<double>(0, half_tick),
            make_vector<double>(0, minor_tick),
            0, make_vector<double>(minor_tick - 1, minor_tick - 1),
            mouse_inside,
            make_vector(get_mouse_position(ctx)[0],
                r.corner[1] + mouse_spacing),
            make_vector<double>(0, mouse_arrow),
            make_vector<double>(mouse_arrow, 0),
            fg_color);
        break;
      }
     case TOP_RULER_CODE:
      {
        label_ruler(
            ctx, n_major_ticks, initial_value, value_inc,
            make_vector<double>(initial_location, (r.corner + r.size)[1]),
            make_vector<double>(location_inc, 0),
            draw_tenth_ticks, label_half_ticks,
            make_vector<double>(0, -full_tick),
            make_vector<double>(0, -half_tick),
            make_vector<double>(0, -minor_tick),
            0,
            make_vector<double>(minor_tick - 1,
                -text_height - (minor_tick - 1)),
            mouse_inside,
            make_vector<double>(get_mouse_position(ctx)[0],
                (r.corner + r.size)[1] - mouse_spacing),
            make_vector<double>(0, -mouse_arrow),
            make_vector<double>(mouse_arrow, 0),
            fg_color);
        break;
      }
     case RIGHT_RULER_CODE:
      {
        label_ruler(
            ctx, n_major_ticks, initial_value, value_inc,
            make_vector<double>(r.corner[0], initial_location),
            make_vector<double>(0, location_inc),
            draw_tenth_ticks, label_half_ticks,
            make_vector<double>(full_tick, 0),
            make_vector<double>(half_tick, 0),
            make_vector<double>(minor_tick, 0),
            pi / 2,
            make_vector<double>(text_height + (minor_tick - 1),
            minor_tick - 1),
            mouse_inside,
            make_vector<double>(r.corner[0] + mouse_spacing,
                get_mouse_position(ctx)[1]),
            make_vector<double>(mouse_arrow, 0),
            make_vector<double>(0, mouse_arrow),
            fg_color);
        break;
      }
     case LEFT_RULER_CODE:
      {
        label_ruler(
            ctx, n_major_ticks, initial_value, value_inc,
            make_vector<double>((r.corner + r.size)[0], initial_location),
            make_vector<double>(0, location_inc),
            draw_tenth_ticks, label_half_ticks,
            make_vector<double>(-full_tick, 0),
            make_vector<double>(-half_tick, 0),
            make_vector<double>(-minor_tick, 0),
            -pi / 2,
            make_vector<double>(-text_height - (minor_tick - 1),
                -(minor_tick - 1)),
            mouse_inside,
            make_vector<double>((r.corner + r.size)[0] - mouse_spacing,
                get_mouse_position(ctx)[1]),
            make_vector<double>(-mouse_arrow, 0),
            make_vector<double>(0, mouse_arrow),
            fg_color);
        break;
      }
    }
}

void embedded_side_rulers::begin(
    gui_context& ctx, embedded_canvas& canvas, ruler_flag_set flags,
    layout const& layout_spec, accessor<string> const& units,
    vector<2,double> const& scales)
{
    ctx_ = &ctx;
    active_ = true;
    canvas_ = &canvas;
    flags_ = flags;
    units_ = make_persistent_copy(ctx, units);
    scales_ = scales;

    get_cached_style_info(ctx, &style_, text("rulers"));
    style_data_ = get_substyle_data(ctx, text("rulers"));

    grid_.begin(ctx, add_default_padding(layout_spec, PADDED));

    {
        scoped_style style(ctx, get(*style_data_).state,
            &get(*style_data_).style_info);

        do_ruler_row(TOP_RULER);

        row_.begin(grid_, GROW);
        alia_if (flags & LEFT_RULER)
        {
            do_ruler(LEFT_RULER, 0);
        }
        alia_end
    }
}
void embedded_side_rulers::end()
{
    gui_context& ctx = *ctx_;
    if (ctx.pass_aborted)
        return;
    alia_if (active_)
    {
        {
            scoped_style style(ctx, get(*style_data_).state,
                &get(*style_data_).style_info);
            alia_if (flags_ & RIGHT_RULER)
            {
                do_ruler(RIGHT_RULER, 0);
            }
            alia_end
            row_.end();
            do_ruler_row(BOTTOM_RULER);
            grid_.end();
        }
        active_ = false;
    }
    alia_end
}
void embedded_side_rulers::do_ruler(ruler_flag_set side, unsigned index)
{
    gui_context& ctx = *ctx_;

    ruler_flag_set flags =
        side | (flags_ & ruler_flag_set(~RULER_SIDE_MASK_CODE));

    layout_box box;
    do_spacer(ctx, &box, index != 0 ?
        layout(height(float(style_->width), PIXELS), GROW | UNPADDED) :
        layout(width(float(style_->width), PIXELS), FILL | UNPADDED));

    alia_pass_dependent_if(is_render_pass(ctx))
    {
        draw_side_ruler(ctx, *canvas_, box,
            ctx.style.properties->background_color,
            ctx.style.properties->text_color,
            scales_[index], flags, *style_);
    }
    alia_end
}
void embedded_side_rulers::do_ruler_row(ruler_flag_set side)
{
    gui_context& ctx = *ctx_;
    alia_if (flags_ & side)
    {
        grid_row r(grid_);
        alia_if (flags_ & LEFT_RULER)
        {
            do_corner();
        }
        alia_end
        do_ruler(side, 1);
        alia_if (flags_ & RIGHT_RULER)
        {
            do_corner();
        }
        alia_end
    }
    alia_end
}
void embedded_side_rulers::do_corner()
{
    gui_context& ctx = *ctx_;
    panel p(ctx, text("junction"), UNPADDED);
    do_text(ctx, make_accessor(units_), layout(CENTER | UNPADDED));
}

//void zoom_to_box(embedded_canvas& canvas, box<2,double> const& box)
//{
//    camera new_camera;
//    new_camera.position = get_center(box);
//    layout_vector const& canvas_size = canvas.region().size;
//    if (box.size[0] != 0 && box.size[1] != 0)
//    {
//        new_camera.zoom = make_concrete_zoom((std::min)(
//            canvas_size[0] / box.size[0], canvas_size[1] / box.size[1]));
//    }
//    else
//        new_camera.zoom = canvas.camera().zoom;
//    set_camera(canvas, new_camera);
//}

//void apply_zoom_box_tool(
//    gui_context& ctx, embedded_canvas& canvas, mouse_button button,
//    rgba8 const& color, line_style const& style, zoom_box_tool_data* data)
//{
//    if (!data)
//        get_data(ctx, &data);
//    if (detect_mouse_press(ctx, canvas.id(), button))
//        *data = get_mouse_position(ctx);
//    if (is_drag_in_progress(ctx, canvas.id(), button))
//    {
//        override_mouse_cursor(ctx, canvas.id(), CROSS_CURSOR);
//        if (is_render_pass(ctx))
//        {
//            vector<2,double> mp = get_mouse_position(ctx);
//            box<2,double> box;
//            for (unsigned i = 0; i < 2; ++i)
//            {
//                box.corner[i] = (std::min)((*data)[i], mp[i]);
//                double high = (std::max)((*data)[i], mp[i]);
//                box.size[i] = high - box.corner[i];
//            }
//            draw_box_outline(ctx, in(color), in(style), in(box));
//        }
//    }
//    if (detect_drag_release(ctx, canvas.id(), button))
//    {
//        vector<2,double> mp = get_mouse_position(ctx);
//        box<2,double> box;
//        for (unsigned i = 0; i < 2; ++i)
//        {
//            box.corner[i] = (std::min)((*data)[i], mp[i]);
//            double high = (std::max)((*data)[i], mp[i]);
//            box.size[i] = high - box.corner[i];
//        }
//        if (box.size[0] > 2 || box.size[1] > 2)
//            zoom_to_box(canvas, box);
//    }
//}

// Create a new camera that has the given zoom level and positioned so that
// the given scene point appears on the same point in the canvas as it would
// have with the old camera.
camera static
zoom_camera_about_point(embedded_canvas& canvas, camera const& old_camera,
    double new_zoom, vector<2,double> const& scene_point)
{
    auto scene_point_on_canvas =
        scene_to_canvas(canvas, scene_point,
            evaluate_zoom_level_for_canvas(canvas, old_camera.zoom),
            old_camera.position);

    return cradle::camera(new_zoom,
        scene_point -
        canvas_to_scene(canvas, scene_point_on_canvas,
            evaluate_zoom_level_for_canvas(canvas, new_zoom),
            make_vector(0., 0.)));
}

void apply_zoom_wheel_tool(embedded_canvas& canvas, double factor)
{
    dataless_ui_context& ctx = canvas.context();
    canvas.set_canvas_coordinates();
    hit_test_box_region(ctx, canvas.id(), canvas.region(), HIT_TEST_WHEEL);
    {
        float movement;
        if (detect_wheel_movement(ctx, &movement, canvas.id()))
        {
            set_camera(canvas,
                zoom_camera_about_point(
                    canvas, canvas.camera(),
                    canvas.camera().zoom * pow(1.1f, movement),
                    canvas_to_scene(canvas, get_mouse_position(ctx))));
        }
    }
}

struct zoom_drag_tool_data
{
    vector<2,double> start_point_in_scene, starting_camera_position,
        start_point_on_canvas;
    double starting_zoom;
    vector<2,double> zoom_out_translation;
        double over_zoom_position;
};

/// \fn void apply_zoom_drag_tool(gui_context& ctx, embedded_canvas& canvas, mouse_button button)
/// \ingroup gui_displays
/// Sets up a canvas to allow click and drag zoom functionality with button
void apply_zoom_drag_tool(
    gui_context& ctx, embedded_canvas& canvas, mouse_button button)
{
    zoom_drag_tool_data* data;
    get_cached_data(ctx, &data);

        // Sets the max_zoom level relative to the canvas size, larger canvas can zoom farther in. Larger view ports (single views/large monitors) can zoom farther in.
        // This max_zoom value must match in the clamp_zoom_level function.
        auto surface_region = canvas.region();
        double max_zoom = 20.;//(surface_region.size[0] > 700) ? surface_region.size[0] / 100 : surface_region.size[0] / 75;

        if (is_drag_in_progress(ctx, canvas.id(), button))
        {
                override_mouse_cursor(ctx, canvas.id(), UP_DOWN_ARROW_CURSOR);
        }
    if (detect_mouse_press(ctx, canvas.id(), button))
    {
        // Record the state when the drag started.
        canvas.set_canvas_coordinates();
        data->start_point_on_canvas = get_mouse_position(ctx);
        data->start_point_in_scene =
            canvas_to_scene(canvas, data->start_point_on_canvas);
        data->starting_zoom = canvas.camera().zoom;
                data->over_zoom_position = 0.;

        data->starting_camera_position = canvas.camera().position;

        // Calculate the scale factor that will fit the scene in the canvas.
        vector<2,double> normal_scale =
            evaluate_zoom_level_for_canvas(canvas, 1.);

        // At that zoom level, calculate where the starting scene point will
        // fall on the canvas, assuming that the scene is centered.
        vector<2,double> end_point_on_canvas =
            scene_to_canvas(canvas, data->start_point_in_scene,
                normal_scale, get_center(canvas.scene_box()));

        // When zooming out, there will be a translation applied to move the
        // starting scene point smoothly from the start point to the end.
        data->zoom_out_translation = end_point_on_canvas -
            vector<2,double>(data->start_point_on_canvas);
    }

    if (detect_drag(ctx, canvas.id(), button))
    {
        canvas.set_canvas_coordinates();
        double motion =
            data->start_point_on_canvas[1] - get_mouse_position(ctx)[1] -
            data->over_zoom_position;

        // Since we limit the zoom levels we must fix the behavior when returning
        // from a zoom limited position
        if (data->starting_zoom * std::pow(1.02, motion) < 0.8)
        {
            data->over_zoom_position =
                data->start_point_on_canvas[1] - get_mouse_position(ctx)[1] -
                std::log(0.8 / data->starting_zoom) / std::log(1.02);
        }
        else if (data->starting_zoom * std::pow(1.02, motion) > max_zoom)
        {
            data->over_zoom_position =
                data->start_point_on_canvas[1] - get_mouse_position(ctx)[1] -
                std::log(max_zoom / data->starting_zoom) / std::log(1.02);
        }

        // Compute actual zoom level
        double new_zoom = clamp_zoom_level(canvas,
            data->starting_zoom * std::pow(1.02, motion));

        vector<2,double> new_scale =
            evaluate_zoom_level_for_canvas(canvas, new_zoom);

        // Calculate the point on the canvas where the starting scene point
        // should currently fall.
        vector<2,double> current_canvas_point =
            vector<2,double>(data->start_point_on_canvas);
        if (motion < 0)
        {
            if (data->starting_zoom > 1.)
            {
                double interpolation_factor =
                    (1 / new_zoom - 1 / data->starting_zoom) /
                    (1 / 1 - 1 / data->starting_zoom);
                current_canvas_point +=
                    data->zoom_out_translation *
                    clamp(interpolation_factor, 0., 1.);
            }
        }
        set_camera(canvas, cradle::camera(
            new_zoom,
            data->starting_camera_position +
              ( data->start_point_in_scene -
                canvas_to_scene(
                    canvas,
                    vector<2,double>(current_canvas_point),
                    evaluate_zoom_level_for_canvas(canvas, new_zoom),
                    data->starting_camera_position))));
    }
}

/// \fn void apply_double_click_reset_tool(embedded_canvas& canvas, mouse_button button)
/// \ingroup gui_displays
/// Sets up a canvas to reset to the default zoom on a double click of button
void apply_double_click_reset_tool(
    embedded_canvas& canvas, mouse_button button)
{
    if (detect_double_click(canvas.context(), canvas.id(), button))
        set_camera(canvas, make_default_camera(canvas.scene_box()));
}

void clear_canvas(embedded_canvas& canvas, rgba8 const& color)
{
    if (is_render_pass(canvas.context()))
    {
        box<2,double> region;
        vector<2,double> const corner0 = canvas_to_scene(canvas,
            vector<2,double>(canvas.region().corner));
        vector<2,double> const corner1 = canvas_to_scene(canvas,
            vector<2,double>(get_high_corner(canvas.region())));
        for (int i = 0; i < 2; ++i)
        {
            double lower = (std::floor)((std::min)(corner0[i], corner1[i]));
            region.corner[i] = lower;
            region.size[i] =
                (std::ceil)((std::abs)((corner1 - corner0)[i]) + 1);
        }
        canvas.context().surface->draw_filled_box(color, region);
    }
}

void draw_scene_line(embedded_canvas& canvas,
    rgba8 const& color, line_style const& style,
    unsigned axis, double position)
{
    dataless_ui_context& ctx = canvas.context();
    if (!is_render_pass(ctx))
        return;
    box2d const& scene_box = canvas.scene_box();
    switch (axis)
    {
     case 0:
        draw_line(ctx, color, style,
            make_vector(position, scene_box.corner[1]),
            make_vector(position, get_high_corner(scene_box)[1]));
        break;
     case 1:
        draw_line(ctx, color, style,
            make_vector(scene_box.corner[0], position),
            make_vector(get_high_corner(scene_box)[0], position));
        break;
    }
}

double apply_line_tool(
    embedded_canvas& canvas,
    rgba8 const& color, line_style const& style,
    unsigned axis, double position,
    widget_id line_id, mouse_button button)
{
    auto& ctx = canvas.context();

    draw_scene_line(canvas, color, style, axis, position);

    scoped_transformation st;
    st.begin(get_geometry_context(ctx));

    canvas.set_canvas_coordinates();

    box2d const& scene_box = canvas.scene_box();

    vector2d sc = scene_to_canvas(canvas, scene_box.corner);
    vector2d shc = scene_to_canvas(canvas, get_high_corner(scene_box));

    int p =
        int(scene_to_canvas(canvas, make_vector(position, position))[axis]);

    int const margin = 4;

    box2i region;
    region.corner[axis] = p - margin;
    region.corner[1 - axis] = int(sc[1 - axis]);
    region.size[axis] = margin * 2;
    region.size[1 - axis] = int(shc[1 - axis] - sc[1 - axis]);
    if (region.size[1 - axis] < 0)
    {
        region.corner[1 - axis] += region.size[1 - axis];
        region.size[1 - axis] = -region.size[1 - axis];
    }
    do_box_region(ctx, line_id, region);

    if (is_click_possible(ctx, line_id) || is_region_active(ctx, line_id))
    {
        override_mouse_cursor(ctx, line_id,
            (axis == 0) ? LEFT_RIGHT_ARROW_CURSOR : UP_DOWN_ARROW_CURSOR);
    }
    if (detect_drag(ctx, line_id, button))
    {
        vector2d q = canvas_to_scene(canvas, get_mouse_position(ctx));
        return q[axis] - position;
    }

    return 0;
}

// make the view zoom as needed to match the 2d canvas
multiple_source_view scale_view_to_canvas(embedded_canvas const& ec, multiple_source_view const& view)
{
    vector2d region_size = make_vector(double(ec.region().size[0]), double(ec.region().size[1]));
    vector2d size = ec.scene_box().size;
    multiple_source_view return_view(view);

    double aspect = region_size[0] / region_size[1];
    double current_aspect = view.display_surface.size[0] / view.display_surface.size[1];

    switch (ec.base_zoom())
    {
        case base_zoom_type::STRETCH_TO_FIT:
            {
                double temp = size[0] - return_view.display_surface.size[0];
                return_view.display_surface.corner[0] -= 0.5 * temp;
                return_view.display_surface.size[0] += temp;

                temp = size[1] - return_view.display_surface.size[1];
                return_view.display_surface.corner[1] -= 0.5 * temp;
                return_view.display_surface.size[1] += temp;
            }
            break;
        case base_zoom_type::FIT_SCENE:
        default:
            if (aspect > current_aspect)
            {
                double temp = (return_view.display_surface.size[1] * aspect) - return_view.display_surface.size[0];
                return_view.display_surface.corner[0] -= 0.5 * temp;
                return_view.display_surface.size[0] += temp;
            }
            else
            {
                double temp = (return_view.display_surface.size[0] / aspect) - return_view.display_surface.size[1];
                return_view.display_surface.corner[1] -= 0.5 * temp;
                return_view.display_surface.size[1] += temp;
            }
            break;
        case base_zoom_type::FIT_SCENE_HEIGHT:
            {
                double temp = (return_view.display_surface.size[1] * aspect) - return_view.display_surface.size[0];
                return_view.display_surface.corner[0] -= 0.5 * temp;
                return_view.display_surface.size[0] += temp;
            }
            break;
        case base_zoom_type::FIT_SCENE_WIDTH:
            {
                double temp = (return_view.display_surface.size[0] / aspect) - return_view.display_surface.size[1];
                return_view.display_surface.corner[1] -= 0.5 * temp;
                return_view.display_surface.size[1] += temp;
            }
            break;
        case base_zoom_type::FILL_CANVAS:
            if (aspect > current_aspect)
            {
                double temp = (return_view.display_surface.size[0] / aspect) - return_view.display_surface.size[1];
                return_view.display_surface.corner[1] -= 0.5 * temp;
                return_view.display_surface.size[1] += temp;
            }
            else
            {
                double temp = (return_view.display_surface.size[1] * aspect) - return_view.display_surface.size[0];
                return_view.display_surface.corner[0] -= 0.5 * temp;
                return_view.display_surface.size[0] += temp;
            }
            break;
    }

    return return_view;
}

vector<3u,canvas_flag_set> get_view_flags(accessor<patient_position_type> const& position)
{
    if (position.is_gettable())
    {
        switch(get(position))
        {
            case patient_position_type::HFS:
                return
                    make_vector<canvas_flag_set>(
                        CANVAS_FLIP_Y,
                        CANVAS_FLIP_Y,
                        NO_FLAGS);
            case patient_position_type::HFP:
                return
                    make_vector<canvas_flag_set>(
                        CANVAS_FLIP_X | CANVAS_FLIP_Y,
                        CANVAS_FLIP_X | CANVAS_FLIP_Y,
                        CANVAS_FLIP_X | CANVAS_FLIP_Y);
            case patient_position_type::FFS:
                return
                    make_vector<canvas_flag_set>(
                        NO_FLAGS,
                        CANVAS_FLIP_X,
                        CANVAS_FLIP_X);
            case patient_position_type::FFP:
                return
                    make_vector<canvas_flag_set>(
                        CANVAS_FLIP_X,
                        NO_FLAGS,
                        CANVAS_FLIP_Y);
            case patient_position_type::HFDR:
            case patient_position_type::HFDL:
            case patient_position_type::FFDR:
            case patient_position_type::FFDL:
            default:
                return
                    make_vector<canvas_flag_set>(
                        NO_FLAGS,
                        NO_FLAGS,
                        NO_FLAGS);
        }
    }
    return
        make_vector<canvas_flag_set>(NO_FLAGS, NO_FLAGS, NO_FLAGS);
}

}
