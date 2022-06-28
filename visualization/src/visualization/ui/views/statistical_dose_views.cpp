#include <visualization/ui/views/statistical_dose_views.hpp>

#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

#include <cradle/geometry/grid_points.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>
#include <cradle/gui/displays/graphing.hpp>
#include <cradle/gui/displays/inspection.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/imaging/api.hpp>
#include <cradle/imaging/inclusion_image.hpp>
#include <cradle/imaging/integral.hpp>

#include <dosimetry/dvh.hpp>

#include <visualization/data/utilities.hpp>

namespace visualization {

// DVHS

struct dvh_graph_highlight_node
{
    owned_id structure_id;
    styled_text label;
    rgba8 color;
    double volume;
    dvh_graph_highlight_node* next;
};

struct dvh_graph_highlight_list_data
{
    bool locked;
    dvh_graph_highlight_node* nodes;

    dvh_graph_highlight_list_data() : locked(false), nodes(0) {}
};

bool static
is_empty(dvh_graph_highlight_list_data& data)
{
    return data.nodes == 0;
}

// dvh_graph_highlight_list records the list of structures that are under the
// mouse in the DVH canvas.
struct dvh_graph_highlight_list
{
    dvh_graph_highlight_list() : active_(false) {}
    dvh_graph_highlight_list(
        gui_context& ctx, min_max<double> const& dose_range,
        min_max<double> const& volume_range)
    { begin(ctx, dose_range, volume_range); }
    ~dvh_graph_highlight_list() { end(); }
    void begin(
        gui_context& ctx, min_max<double> const& dose_range,
        min_max<double> const& volume_range);
    void end();
    bool is_active() const { return active_; }
    dvh_graph_highlight_list_data& data() const { return *data_; }
    double dose() const { return dose_; }
    min_max<double> const& dose_range() const { return dose_range_; }
    min_max<double> const& volume_range() const { return volume_range_; }
    void process_structure(
        gui_context& ctx, id_interface const& structure_id,
        styled_text const& label, rgba8 const& color,
        optional<double> const& volume,
        optional<double> const& low_volume,
        optional<double> const& high_volume);
 private:
    bool active_;
    dataless_ui_context* ctx_;
    double dose_;
    // the ranges in which structures should be highlighted
    min_max<double> dose_range_, volume_range_;
    dvh_graph_highlight_list_data* data_;
    // If not locked, this is for recording new nodes.
    dvh_graph_highlight_node** next_ptr_;
    // If locked, this is for traversing the list of nodes and updating it.
    dvh_graph_highlight_node* node_;
};

void dvh_graph_highlight_list::begin(
    gui_context& ctx, min_max<double> const& dose_range,
    min_max<double> const& volume_range)
{
    ctx_ = &ctx;
    dose_ = center_of_range(dose_range);
    dose_range_ = dose_range;
    volume_range_ = volume_range;
    get_cached_data(ctx, &data_);
    if (is_refresh_pass(ctx))
    {
        if (data_->locked)
        {
            node_ = data_->nodes;
        }
        else
        {
            data_->nodes = 0;
            next_ptr_ = &data_->nodes;
        }
    }
    active_ = true;
}
void dvh_graph_highlight_list::end()
{
    if (active_)
    {
        if (is_refresh_pass(*ctx_))
        {
            if (data_->locked)
            {
                // We should have seen all the locked nodes.
                assert(node_ == 0);
            }
            else
            {
                // Terminate the list.
                *next_ptr_ = 0;
            }
        }
        active_ = false;
    }
}

void dvh_graph_highlight_list::process_structure(
    gui_context& ctx, id_interface const& structure_id,
    styled_text const& label, rgba8 const& color,
    optional<double> const& volume,
    optional<double> const& low_volume,
    optional<double> const& high_volume)
{
    dvh_graph_highlight_node* node_data;
    get_cached_data(ctx, &node_data);
    if (is_refresh_pass(ctx))
    {
        if (data_->locked)
        {
            if (node_ && node_->structure_id.matches(structure_id))
            {
                node_->volume = volume ? get(volume) : 0;
                node_ = node_->next;
            }
        }
        else
        {
            if (volume &&
                high_volume && get(high_volume) >= volume_range_.min &&
                low_volume && get(low_volume) < volume_range_.max)
            {
                node_data->structure_id.store(structure_id);
                node_data->label = label;
                node_data->color = color;
                node_data->volume = get(volume);
                *next_ptr_ = node_data;
                next_ptr_ = &node_data->next;
            }
        }
    }
}

struct dvh_highlight_overlay_data
{
    popup_positioning positioning;
    value_smoother<float> popup_intensity;
};

void do_dvh_graph_highlight_overlay(
    gui_context& ctx, embedded_canvas& canvas,
    dvh_graph_highlight_list& highlight_list, accessor<bool> const& absolute)
{
    dvh_highlight_overlay_data* overlay;
    if (get_data(ctx, &overlay))
        reset_smoothing(overlay->popup_intensity, 0.f);

    auto& highlight_data = highlight_list.data();

    alia_untracked_if (!is_empty(highlight_data))
    {
        alia_untracked_if (is_refresh_pass(ctx))
        {
            set_active_overlay(ctx, overlay);
        }
        alia_end

        alia_untracked_if (detect_click(ctx, canvas.id(), LEFT_BUTTON))
        {
            highlight_data.locked = !highlight_data.locked;
            end_pass(ctx);
        }
        alia_end

        alia_untracked_if (is_render_pass(ctx))
        {
            alia_untracked_if (highlight_data.locked)
            {
                scoped_transformation st(ctx);
                canvas.set_scene_coordinates();
                double max_volume = 0.;
                for (auto* node = highlight_data.nodes; node;
                    node = node->next)
                {
                    draw_line(ctx, node->color, line_style(1, solid_line),
                        make_vector(0., node->volume),
                        make_vector(highlight_list.dose(), node->volume));
                    if (node->volume > max_volume)
                        max_volume = node->volume;
                }
                draw_line(ctx, rgb8(0x80, 0x80, 0x90),
                    line_style(1, solid_line),
                    make_vector(highlight_list.dose(), 0.),
                    make_vector(highlight_list.dose(), max_volume));
            }
            alia_end

            {
                scoped_transformation st(ctx);
                canvas.set_canvas_coordinates();

                layout_vector center = layout_vector(
                    scene_to_canvas(canvas,
                        make_vector(highlight_list.dose(),
                            center_of_range(highlight_list.volume_range()))));
                layout_box box;
                layout_scalar x = as_layout_size(3.);
                box.corner = center - make_layout_vector(x, x);
                box.size = 2 * make_layout_vector(x, x);

                position_overlay(ctx, overlay->positioning, box);
            }
        }
        alia_end
    }
    alia_end

    float popup_intensity =
        smooth_raw_value(ctx, overlay->popup_intensity,
            is_empty(highlight_data) ? 0.f : 1.f,
            animated_transition(default_curve, 250));

    alia_if (popup_intensity > 0 && !is_empty(highlight_data))
    {
        {
            scoped_transformation st(ctx);
            canvas.set_canvas_coordinates();
            {
                nonmodal_popup popup(ctx, overlay, overlay->positioning,
                    NONMODAL_POPUP_DESCENDING_GRAPH_PLACEMENT);
                scoped_surface_opacity scoped_opacity(ctx, popup_intensity);
                panel panel(ctx, text("transparent-overlay"));
                do_styled_text(ctx, text("heading"), text("DVH"));
                grid_layout grid(ctx);
                {
                    grid_row row(grid);
                    do_spacer(ctx);
                    do_styled_text(ctx, text("label"), text("dose"), LEFT);
                    do_styled_text(ctx, text("value"),
                        printf(ctx, "%.2f", in(highlight_list.dose())), RIGHT);
                    do_styled_text(ctx, text("units"), text("Gy(RBE)"), LEFT);
                }
                alia_for (auto* node = highlight_data.nodes; node;
                    node = node->next)
                {
                    grid_row row(grid);
                    do_color(ctx, in(node->color));
                    {
                        scoped_substyle ss(ctx, text("label"));
                        do_flow_text(ctx, in(node->label),
                            layout(width(10, CHARS), LEFT));
                    }
                    do_styled_text(ctx, text("value"),
                        printf(ctx, "%.1f", in_ptr(&node->volume)), RIGHT);
                    alia_if (is_true(absolute))
                    {
                        do_styled_text(ctx, text("units"), text("cc"), LEFT);
                    }
                    alia_else
                    {
                        do_styled_text(ctx, text("units"), text("%"), LEFT);
                    }
                    alia_end
                }
                alia_end
            }
        }
    }
    alia_end
}

struct dvh_graph : noncopyable
{
    dvh_graph(gui_context& ctx)
      : ctx_(&ctx), active_(false)
    {
        labels_.initialize(ctx);
        rulers_.initialize(ctx);
    }
    dvh_graph(
        gui_context& ctx, double max_dose, double max_volume,
        bool absolute_volume, layout const& layout_spec = default_layout)
      : ctx_(&ctx), active_(false)
    {
        labels_.initialize(ctx);
        rulers_.initialize(ctx);
        begin(ctx, max_dose, max_volume, absolute_volume, layout_spec);
    }
    ~dvh_graph() { end(); }

    void begin(
        gui_context& ctx, double max_dose, double max_volume,
        bool absolute_volume, layout const& layout_spec = default_layout);
    void end();

    embedded_canvas& canvas() { return canvas_; }

    void do_structure(id_interface const& id,
        accessor<styled_text> const& label, accessor<rgb8> const& color,
        image_interface_1d const& dvh);

    void do_highlight(gui_context& ctx, accessor<bool> const& absolute);

 private:
    gui_context* ctx_;
    bool active_;
    scoped_substyle substyle_;
    embedded_graph_labels labels_;
    embedded_side_rulers rulers_;
    embedded_canvas canvas_;
    dvh_graph_highlight_list highlight_;
};

double static
choose_major_grid_line_spacing(double scene_size)
{
    double x = 0.001;
    while (x * 2 < scene_size)
        x *= 10;
    return x;
}

double static
choose_minor_grid_line_spacing(double scene_size)
{
    double x = 0.0001;
    while (x * 20 < scene_size)
        x *= 10;
    return x;
}

void dvh_graph::begin(
    gui_context& ctx, double max_dose, double max_volume,
    bool absolute_volume, layout const& layout_spec)
{
    substyle_.begin(ctx, text("dvh"));

    labels_.begin(ctx,
        text("dose (Gy(RBE))"),
        absolute_volume ? text("volume (cc)") : text("volume (%)"),
        layout_spec);

    auto scene_box =
        make_box(
            make_vector(0., 0.),
            make_vector(max_dose * 1.05,
                absolute_volume ? max_volume * 1.05 : 105.0));

    canvas_.initialize(ctx, scene_box, base_zoom_type::STRETCH_TO_FIT, none,
        CANVAS_FLIP_Y | CANVAS_STRICT_CAMERA_CLAMPING);

    // Track the state of the absolute_volume flag, and when it changes, reset
    // the camera.
    bool* cached_absolute_volume;
    if (get_cached_data(ctx, &cached_absolute_volume))
        *cached_absolute_volume = absolute_volume;
    if (is_refresh_pass(ctx) && *cached_absolute_volume != absolute_volume)
    {
        set_camera(canvas_, make_default_camera(scene_box));
        *cached_absolute_volume = absolute_volume;
    }

    rulers_.begin(ctx, canvas_, LEFT_RULER | BOTTOM_RULER, GROW | UNPADDED);

    canvas_.begin(GROW | UNPADDED);

    clear_canvas(canvas_, rgb8(0x10, 0x10, 0x14));

    if (is_render_pass(ctx))
    {
        rgba8 major_grid_line_color(0x48, 0x48, 0x4c, 0xff);
        rgba8 minor_grid_line_color(0x20, 0x20, 0x24, 0xff);
        draw_grid_lines_for_axis(canvas_, scene_box, minor_grid_line_color,
            line_style(1, solid_line), 0,
            choose_minor_grid_line_spacing(scene_box.size[0]));
        draw_grid_lines_for_axis(canvas_, scene_box, major_grid_line_color,
            line_style(1, solid_line), 0,
            choose_major_grid_line_spacing(scene_box.size[0]));
        draw_grid_lines_for_axis(canvas_, scene_box, minor_grid_line_color,
            line_style(1, solid_line), 1,
            choose_minor_grid_line_spacing(scene_box.size[1]));
        draw_grid_lines_for_axis(canvas_, scene_box, major_grid_line_color,
            line_style(1, solid_line), 1,
            choose_major_grid_line_spacing(scene_box.size[1]));
    }

    alia_if (canvas_.mouse_position())
    {
        auto const mouse = get(canvas_.mouse_position());
        auto dose = canvas_to_scene(canvas_, mouse)[0];
        auto volume_range = min_max<double>(
            canvas_to_scene(canvas_, mouse + make_vector(0., 5.))[1],
            canvas_to_scene(canvas_, mouse - make_vector(0., 5.))[1]);
        assert(volume_range.max >= volume_range.min);
        auto dose_range = min_max<double>(
            canvas_to_scene(canvas_, mouse - make_vector(5., 0.))[0],
            canvas_to_scene(canvas_, mouse + make_vector(5., 0.))[0]);
        assert(dose_range.max >= dose_range.min);
        highlight_.begin(ctx, dose_range, volume_range);
    }
    alia_end

    active_ = true;
}
void dvh_graph::end()
{
    if (active_)
    {
        highlight_.end();
        canvas_.end();
        rulers_.end();
        labels_.end();
        substyle_.end();
        active_ = false;
    }
}

void dvh_graph::do_structure(id_interface const& id,
    accessor<styled_text> const& label, accessor<rgb8> const& color,
    image_interface_1d const& dvh)
{
    gui_context& ctx = *ctx_;
    embedded_canvas& c = canvas_;

    alia_if (is_gettable(color))
    {
        draw_line_graph(ctx, in(apply_alpha(get(color), 0xff)),
            in(line_style(2, solid_line)),
            make_image_plottable(ctx, dvh));

        alia_if (highlight_.is_active())
        {
            auto volume =
                dvh.get_point(ctx, rq_in(make_vector(highlight_.dose())));
            auto low_volume = dvh.get_point(ctx,
                rq_in(make_vector(highlight_.dose_range().max)));
            auto high_volume = dvh.get_point(ctx,
                rq_in(make_vector(highlight_.dose_range().min)));
            alia_if (is_gettable(label) && is_gettable(volume) &&
                is_gettable(low_volume) && is_gettable(high_volume))
            {
                highlight_.process_structure(ctx, id, get(label), get(color),
                    get(volume), get(low_volume), get(high_volume));
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

void dvh_graph::do_highlight(gui_context& ctx, accessor<bool> const& absolute)
{
    // Reset overlay if mouse entering new canvas
    if(detect_mouse_motion(canvas_.context(), canvas_.id()))
    {
        clear_active_overlay(ctx);
    }
    alia_if (canvas_.mouse_position())
    {
        highlight_.end();
        do_dvh_graph_highlight_overlay(ctx, canvas_, highlight_, absolute);
    }
    alia_end
}

request<double> static
compose_structure_dvh_volume_request(
    request<double> const& voxel_volume_scale_factor,
    request<std::vector<weighted_grid_index> > const& structure_voxels)
{
    return
        rq_multiplication(
            rq_multiplication(
                voxel_volume_scale_factor,
                rq_sum_grid_index_weights(structure_voxels)),
            rq_value(0.001)); // for conversion from mm^3 to cc
}

request<image1> static
compose_dvh_request(
    request<double> const& voxel_volume_scale_factor,
    request<image1> const& histogram,
    request<std::vector<weighted_grid_index> > const& structure_voxels,
    bool absolute)
{
    auto cumulative_dvh =
        rq_accumulate_dvh(
            rq_normalize_differential_dvh(histogram,
                rq_sum_grid_index_weights(structure_voxels)));
    if (absolute)
    {
        return
            rq_scale_image_values(
                cumulative_dvh,
                compose_structure_dvh_volume_request(
                    voxel_volume_scale_factor,
                    structure_voxels),
                rq_value(no_units),
                rq_value(units("cc")));
    }
    else
    {
        return
            rq_scale_image_values(
                cumulative_dvh,
                rq_value(100.),
                rq_value(no_units),
                rq_value(units("percent")));
    }
}

request<optional<min_max<double> > > static
compose_dvh_range_request(
    request<image1> const& dvh,
    bool absolute)
{
    return absolute ?
        rq_value(some(min_max<double>(0., 100.))) :
        rq_image_min_max(dvh);
}

request<std::vector<weighted_grid_index> > static
compose_gui_structure_voxels_request(
    request<image_geometry<3> > const& geometry,
    gui_structure const& structure)
{
    auto grid_cells =
        rq_compute_grid_cells_in_structure(
            rq_property(geometry, grid),
            structure.geometry);
    return rq_property(grid_cells, cells_inside);
}

// Get the maximum volume of any of the structures in the list.
// This is necessary to properly scale the DVH.
auto static
determine_max_dvh_volume(
    gui_context& ctx,
    image_interface_3d const& dose,
    accessor<std::vector<gui_structure> > const& structures)
{
    // This is done in a roundabout way because in cases when the requests for
    // some structure volumes haven't completed yet, we still want an answer.
    // (We want to use the maximum of any results that ARE ready.)
    // Thus, we issue the request for each structure separately, treat the
    // result as optional, add a fallback value of none, and then eliminate
    // the nones when we determine the maximum.
    auto voxel_volume_scale_factor = dose.get_voxel_volume_scale(ctx);
    auto volumes =
        gui_map<optional<double> >(ctx,
            [&](gui_context& ctx, accessor<gui_structure> const& structure)
            {
                auto volume =
                    gui_request(ctx,
                        gui_apply(ctx,
                            [ ](request<double> const& voxel_scale_factor,
                                request<std::vector<weighted_grid_index> > const& voxels)
                            {
                                return
                                    compose_structure_dvh_volume_request(
                                        voxel_scale_factor,
                                        voxels);
                            },
                            voxel_volume_scale_factor,
                            dose.get_voxels_in_structure_request(ctx,
                                _field(structure, geometry))));
                return
                    add_fallback_value(
                        gui_apply(ctx,
                            [ ](double x)
                            {
                                return some(x);
                            },
                            volume),
                        in(optional<double>()));
            },
            structures);
    return
        unwrap_optional(
            gui_apply(ctx,
                [ ](std::vector<optional<double> > const& volumes)
                {
                    return array_max(filter_optionals(volumes));
                },
                volumes));
}

void
do_dvh_view(
    gui_context& ctx,
    image_interface_3d const& dose,
    accessor<std::vector<gui_structure> > const& structures,
    accessor<dvh_view_state> const& state)
{
    auto voxel_volume_scale_factor = dose.get_voxel_volume_scale(ctx);
    auto value_range = unwrap_optional(dose.get_value_range(ctx));
    auto max_volume = determine_max_dvh_volume(ctx, dose, structures);

    alia_if (is_gettable(value_range) && is_gettable(max_volume))
    {
        dvh_graph graph(ctx, get(value_range).max, get(max_volume),
            is_true(_field(state, absolute)), GROW | UNPADDED);

        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<gui_structure> const& structure) -> void
            {
                auto structure_voxels =
                    dose.get_voxels_in_structure_request(ctx,
                        _field(structure, geometry));
                auto histogram =
                    dose.get_partial_histogram_request(ctx,
                        structure_voxels,
                        _field(value_range, min),
                        _field(value_range, max),
                        in(0.01));
                auto dvh =
                    gui_apply(ctx,
                        compose_dvh_request,
                        voxel_volume_scale_factor,
                        histogram,
                        structure_voxels,
                        _field(state, absolute));
                auto& gui_dvh =
                    make_image_interface(ctx,
                        dvh,
                        rq_in(optional<out_of_plane_information>()),
                        gui_apply(ctx,
                            compose_dvh_range_request,
                            dvh,
                            _field(state, absolute)));
                graph.do_structure(
                    structure.id(),
                    _field(structure, label),
                    _field(structure, color),
                    gui_dvh);
            },
            structures);

        graph.do_highlight(ctx, _field(state, absolute));

        auto& c = graph.canvas();

        apply_panning_tool(c, LEFT_BUTTON);
        apply_double_click_reset_tool(c, LEFT_BUTTON);
        apply_zoom_drag_tool(ctx, c, RIGHT_BUTTON);
    }
    alia_else
    {
        do_empty_display_panel(ctx, GROW);
    }
    alia_end
}

struct statistical_dose_view_contents
{
    image_interface_3d const* dose;
    keyed_data<std::vector<gui_structure> > structures;
};

statistical_dose_view_contents*
generate_statistical_dose_view_contents(
    gui_context& ctx,
    image_interface_3d const* dose,
    accessor<std::vector<gui_structure> > const& structures)
{
    statistical_dose_view_contents* contents;
    get_cached_data(ctx, &contents);

    // Reset the contents on refresh passes.
    alia_untracked_if (is_refresh_pass(ctx))
    {
        contents->dose = dose;
        refresh_accessor_clone(contents->structures, structures);
    }
    alia_end

    return contents;
}

// dvh_view

void
dvh_view::initialize(
    statistical_dose_view_contents* contents,
    keyed_data<dvh_view_state>* state)
{
    contents_ = contents;
    state_ = state;
}

string const&
dvh_view::get_type_id() const
{
    static string const type_id = "dvh";
    return type_id;
}

string const&
dvh_view::get_type_label(null_display_context const& display_ctx)
{
    static string const label = "DVH";
    return label;
}

indirect_accessor<string>
dvh_view::get_view_label(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id)
{
    return make_indirect(ctx, text("DVH"));
}

void
dvh_view::do_view_content(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id,
    bool is_preview)
{
    do_dvh_view(ctx,
        *contents_->dose,
        make_accessor(&contents_->structures),
        make_accessor(state_));
}

void
add_dvh_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    dvh_view* view,
    statistical_dose_view_contents* contents,
    accessor<dvh_view_state> const& state)
{
    // Get a cached version of the state that can be passed into the view.
    keyed_data<dvh_view_state>* cached_state;
    get_cached_data(ctx, &cached_state);
    alia_untracked_if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(*cached_state, state);
    }
    alia_end

    // Initialize the view and add it to the provider.
    view->initialize(contents, cached_state);
    provider.add_view(view);
}

// STATS

double
compute_dvh_statistic(
    image1 const& normalized_differential_dvh,
    double volume_fraction)
{
    return
        compute_inverse_image_integral_over_ray(
            normalized_differential_dvh,
            ray<1,double>(make_vector(0.), make_vector(1.)),
            // The integral will be skewed by the size of the bins, so we have
            // to adjust the expected value based on that.
            volume_fraction * get_spacing(normalized_differential_dvh)[0]);
}

template<class Pixel>
double compute_eud(
    image<1,Pixel,const_view> const& differential_dvh,
    double a_value)
{
    double sum = 0;
    double total_volume = 0;
    auto dose_values = get_points_on_grid(get_grid(differential_dvh));
    auto bin = get_begin(differential_dvh);
    for (auto const& dose : dose_values)
    {
        sum += *bin * std::pow(dose[0], a_value);
        total_volume += *bin;
        ++bin;
    }
    return std::pow(sum / total_volume, 1. / a_value);
}

struct eud_computer
{
    double a_value;
    double result;
    template<class Pixel, class SrcSP>
    void operator()(image<1,Pixel,SrcSP> const& src)
    {
        result = compute_eud(as_const_view(src), a_value);
    }
};

double compute_eud(
    image1 const& differential_dvh,
    double a_value)
{
    eud_computer computer;
    computer.a_value = a_value;
    apply_fn_to_gray_variant(computer, differential_dvh);
    return computer.result;
}

request<image1>
compose_normalized_dvh_request(
    request<image1> const& histogram,
    request<std::vector<weighted_grid_index> > const& structure_voxels)
{
    return
        rq_foreground(rq_normalize_differential_dvh(histogram,
            rq_sum_grid_index_weights(structure_voxels)));
}

void
do_dose_stats_view(
    gui_context& ctx,
    image_interface_3d const& dose,
    accessor<std::vector<gui_structure> > const& structures)
{
    scrollable_panel panel(ctx,
        text("background"),
        layout(width(300, PIXELS), GROW | UNPADDED));

    grid_layout grid(ctx);
    scoped_substyle style(ctx, text("table"));

    {
        scoped_substyle style(ctx, text("header"));
        grid_row row(grid);
        do_text(ctx, text(""), LEFT);
        do_text(ctx, text("structure"), LEFT);
        do_text(ctx, text("   min"), RIGHT);
        do_text(ctx, text("   max"), RIGHT);
        do_text(ctx, text("  mean"), RIGHT);
        do_text(ctx, text("   D99"), RIGHT);
        do_text(ctx, text("    D1"), RIGHT);
        do_text(ctx, text("   EUD"), RIGHT);
    }

    auto dose_geometry = dose.get_geometry_request(ctx);
    auto value_range = unwrap_optional(dose.get_value_range(ctx));

    for_each(ctx,
        [&](gui_context& ctx, size_t index,
            accessor<gui_structure> const& structure) -> void
        {
            scoped_substyle
                style(ctx, ( index % 2 == 0) ? text("even_row") : text("odd_row"));

            grid_row row(grid);
            auto structure_voxels =
                dose.get_voxels_in_structure_request(ctx,
                    _field(structure, geometry));
            auto differential_dvh =
                dose.get_partial_histogram_request(ctx,
                    structure_voxels,
                    _field(value_range, min),
                    _field(value_range, max),
                    in(0.01));
            auto normalized_dvh =
                gui_request(ctx,
                    gui_apply(ctx,
                        compose_normalized_dvh_request,
                        differential_dvh,
                        structure_voxels));
            auto stats =
                dose.get_partial_statistics(ctx, structure_voxels);
            do_color(ctx, _field(structure, color));
            do_flow_text(ctx, _field(structure, label),
                layout(width(20, CHARS), LEFT));
            do_text(ctx,
                printf(ctx, "%.2f", unwrap_optional(_field(stats, min))),
                RIGHT);
            do_text(ctx,
                printf(ctx, "%.2f", unwrap_optional(_field(stats, max))),
                RIGHT);
            do_text(ctx,
                printf(ctx, "%.2f", unwrap_optional(_field(stats, mean))),
                RIGHT);
            do_text(ctx,
                printf(ctx, "%.2f",
                    gui_apply(ctx,
                        compute_dvh_statistic,
                        normalized_dvh,
                        in(0.99))),
                RIGHT);
            do_text(ctx,
                printf(ctx, "%.2f",
                    gui_apply(ctx,
                        compute_dvh_statistic,
                        normalized_dvh,
                        in(0.01))),
                RIGHT);
            do_text(ctx,
                printf(ctx, "%.2f",
                    gui_apply(ctx,
                        compute_eud,
                        normalized_dvh,
                        unwrap_optional(
                            _field(_field(structure, biological), a)))),
                RIGHT);
        },
        structures);
}

// dose_stats_view

void
dose_stats_view::initialize(
    statistical_dose_view_contents* contents)
{
    contents_ = contents;
}

string const&
dose_stats_view::get_type_id() const
{
    static string const type_id = "statistics";
    return type_id;
}

string const&
dose_stats_view::get_type_label(null_display_context const& display_ctx)
{
    static string const label = "Statistics";
    return label;
}

indirect_accessor<string>
dose_stats_view::get_view_label(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id)
{
    return make_indirect(ctx, text("Statistics"));
}

void
dose_stats_view::do_view_content(
    gui_context& ctx,
    null_display_context const& display_ctx,
    string const& instance_id,
    bool is_preview)
{
    do_dose_stats_view(ctx,
        *contents_->dose,
        make_accessor(&contents_->structures));
}

void
add_dose_stats_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    dose_stats_view* view,
    statistical_dose_view_contents* contents)
{
    view->initialize(contents);
    provider.add_view(view);
}

}
