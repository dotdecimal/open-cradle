#include <analysis/ui/displays/pbs_machine_display.hpp>

#include <alia/layout/utilities.hpp>
#include <alia/ui/utilities/rendering.hpp>

#include <cradle/external/opengl.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/graphing.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/canvas.hpp>
#include <cradle/gui/displays/display.hpp>
#include <cradle/gui/requests.hpp>

namespace analysis {

struct pbs_machine_display_context
{
    indirect_accessor<pbs_machine_spec> machine;
};

static void
do_label_cell_with_units(ui_context& ctx, table_row& row,
    accessor<string> const& label, accessor<string> const& units)
{
    table_cell cell(row);
    row_layout layout(ctx);
    do_text(ctx, label);
    do_styled_text(ctx, text("units"), units);
}

struct scrollable_table
{
    scrollable_table() : ctx_(0) {}
    scrollable_table(ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout)
    { begin(ctx, style, layout_spec); }
    ~scrollable_table() { end(); }
    void begin(ui_context& ctx, accessor<string> const& style,
        layout const& layout_spec = default_layout);
    void begin_content();
    void end();
    table& table() { return table_; }
 private:
    ui_context* ctx_;
    alia::table table_;
    scoped_substyle header_style_;
    panel header_panel_;
    scrollable_panel content_panel_;
};

void scrollable_table::begin(ui_context& ctx, accessor<string> const& style,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    table_.begin(ctx, style, GROW);
    header_style_.begin(ctx, style);
    header_panel_.begin(ctx, text("first-row"), UNPADDED,
        PANEL_NO_INTERNAL_PADDING);
}
void scrollable_table::begin_content()
{
    ui_context& ctx = *ctx_;
    header_panel_.end();
    header_style_.end();
    content_panel_.begin(ctx, text(""), GROW | UNPADDED,
        PANEL_NO_INTERNAL_PADDING);
}
void scrollable_table::end()
{
    if (ctx_)
    {
        content_panel_.end();
        table_.end();
        ctx_ = 0;
    }
}

static void
do_deliverable_energy_table_row(gui_context& ctx, table& table,
    pbs_deliverable_energy const& energy)
{
    table_row row(table);
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%6.2f", in(energy.energy)));
    }
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%6.2f", in(energy.r90)));
    }
}

static void
do_deliverable_energy_table(gui_context& ctx,
    accessor<std::vector<pbs_deliverable_energy> > const& energies)
{
    scrollable_table st(ctx, text("energy-table"), GROW);
    {
        table_row row(st.table());
        do_label_cell_with_units(ctx, row, text("Energy"), text("(MeV)"));
        do_label_cell_with_units(ctx, row, text("R90"), text("(mm)"));
    }
    st.begin_content();
    {
        alia_cached_ui_block(energies.id(), default_layout)
        {
            alia_if (is_gettable(energies))
            {
                alia_for (auto const& energy : get(energies))
                {
                    do_deliverable_energy_table_row(ctx, st.table(), energy);
                }
                alia_end
            }
            alia_end
        }
        alia_end
    }
}

CRADLE_DEFINE_SIMPLE_VIEW(
    deliverable_energies_view, pbs_machine_display_context,
    "deliverable_energies", "Deliverable Energies",
    {
        do_deliverable_energy_table(ctx,
            _field(display_ctx.machine, deliverable_energies));
    })

static void
do_modeled_energy_table_row(gui_context& ctx, table& table,
    pbs_modeled_energy const& energy)
{
    table_row row(table);
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%6.2f", in(energy.energy)));
    }
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%6.2f", in(energy.r90)));
    }
    {
        table_cell cell(row);
        do_text(ctx, printf(ctx, "%5.2f", in(energy.w80)));
    }
    {
        table_cell cell(row);
        auto sigma =
            gui_apply(ctx,
                [] (pbs_optical_sigma const& sigma) {
                    return evaluate_sigma_at_z(sigma, 0); },
                in(energy.sigma));
        do_text(ctx, printf(ctx, "%5.2f x %5.2f",
            select_index_by_value(sigma, 0),
            select_index_by_value(sigma, 1)));
    }
}

static void
do_modeled_energy_table(gui_context& ctx,
    accessor<std::vector<pbs_modeled_energy> > const& energies)
{
    scrollable_table st(ctx, text("energy-table"), GROW);
    {
        table_row row(st.table());
        do_label_cell_with_units(ctx, row, text("Energy"), text("(MeV)"));
        do_label_cell_with_units(ctx, row, text("R90"), text("(mm)"));
        do_label_cell_with_units(ctx, row, text("W80"), text("(mm)"));
        do_label_cell_with_units(ctx, row, text("Sigma at Iso"),
            text("(X x Y, mm)"));
    }
    st.begin_content();
    {
        alia_cached_ui_block(energies.id(), default_layout)
        {
            alia_if (is_gettable(energies))
            {
                alia_for (auto const& energy : get(energies))
                {
                    do_modeled_energy_table_row(ctx, st.table(), energy);
                }
                alia_end
            }
            alia_end
        }
        alia_end
    }
}

CRADLE_DEFINE_SIMPLE_VIEW(
    modeled_energies_view, pbs_machine_display_context,
    "modeled_energies", "Modeled Energies",
    {
        do_modeled_energy_table(ctx,
            _field(display_ctx.machine, modeled_energies));
    })

optional<double> static
get_max_peak_dose(std::vector<pbs_modeled_energy> const& energies)
{
    optional<double> max;
    for (auto const& energy : energies)
    {
        auto range = irregularly_sampled_function_range(energy.pristine_peak);
        if (range && (!max || get(range).max > get(max)))
            max = get(range).max;
    }
    return max;
}

template<class Energy>
double get_max_range(std::vector<Energy> const& energies)
{
    return energies.empty() ? 0. : energies.back().r90;
}

template<class Energy>
double get_min_range(std::vector<Energy> const& energies)
{
    return energies.empty() ? 0. : energies.front().r90;
}

void static
do_modeled_energy_peaks(gui_context& ctx,
    accessor<std::vector<pbs_modeled_energy> > const& energies/*,
    accessor<size_t> const& selected*/)
{
    auto max_peak_dose =
        unwrap_optional(
            gui_apply(ctx,
                get_max_peak_dose,
                energies));

    auto max_range =
        gui_apply(ctx,
            get_max_range<pbs_modeled_energy>,
            energies);

    alia_if (is_gettable(max_range) && is_gettable(max_peak_dose))
    {
        line_graph graph(ctx,
            make_box(
                make_vector(0., 0.),
                make_vector(get(max_range) + 20, get(max_peak_dose) * 1.1)),
            in(data_reporting_parameters("depth", "mm water", 1)),
            in(data_reporting_parameters("dose", "Gy (RBE) mm^2 / Gp", 1)),
            text("pristine-peak-graph"),
            layout(size(800, 600, PIXELS), GROW));

        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<pbs_modeled_energy> const& energy)
            {
                auto pp = _field(energy, pristine_peak);
                graph.do_line(ctx,
                    printf(ctx, "%.1f MeV", _field(energy, energy)),
                    text("peak"),
                    in(data_reporting_parameters(
                        "dose", "Gy (RBE) mm^2 / Gp", 1)),
                    _field(pp, samples));
            },
            energies);

        graph.do_highlight(ctx);

        apply_panning_tool(graph.canvas(), LEFT_BUTTON);
        apply_zoom_drag_tool(ctx, graph.canvas(), RIGHT_BUTTON);
    }
    alia_else
    {
        do_empty_display_panel(ctx);
    }
    alia_end
}

CRADLE_DEFINE_SIMPLE_VIEW(
    pristine_peaks_view, pbs_machine_display_context,
    "pristine_peaks", "Pristine Peaks",
    {
        do_modeled_energy_peaks(ctx,
            _field(display_ctx.machine, modeled_energies));
    })

double static
get_max_sigma_at_iso(std::vector<pbs_modeled_energy> const& energies)
{
    double max = 0;
    for (auto const& energy : energies)
    {
        auto sigma_at_iso = evaluate_sigma_at_z(energy.sigma, 0);
        max = (std::max)(max, (std::max)(sigma_at_iso[0], sigma_at_iso[1]));
    }
    return max;
}

std::vector<vector2d> static
generate_sigma_points(
    std::vector<pbs_modeled_energy> const& energies, unsigned axis)
{
    return map(
        [axis](pbs_modeled_energy const& energy)
        {
            return make_vector(
                energy.energy, evaluate_sigma_at_z(energy.sigma, 0)[axis]);
        },
        energies);
}

void static
do_line_sample(gui_context& ctx,
    rgba8 const& color, line_style const& style,
    layout const& layout_spec = default_layout)
{
    layout_box assigned_region;
    do_spacer(ctx, &assigned_region,
        add_default_size(layout_spec, size(1.2f, 1.2f, EM)));
    alia_untracked_if (is_render_pass(ctx))
    {
        double y = assigned_region.corner[1] + assigned_region.size[1] / 2;
        draw_line(ctx, color, style,
            make_vector<double>(assigned_region.corner[0], y),
            make_vector<double>(get_high_corner(assigned_region)[0], y));
    }
    alia_end
}

void static
do_sigma_graphs(gui_context& ctx,
    accessor<std::vector<pbs_modeled_energy> > const& energies)
{
    alia_if (is_gettable(energies))
    {
        layered_layout ll(ctx, GROW);

        graph_line_style_info const* x_style_info;
        graph_line_style_info const* y_style_info;

        {
            auto max_y =
                gui_apply(ctx, get_max_sigma_at_iso, energies);

            box<2,double> scene_box =
                make_box(
                    make_vector(0., 0.),
                    make_vector(
                        get(energies).back().energy + 20,
                        get(max_y) * 1.1));

            static data_reporting_parameters x_parameters("energy", "MeV", 1);
            static data_reporting_parameters y_parameters(
                "sigma at isocenter", "mm", 1);

            line_graph graph(ctx, scene_box,
                in_ptr(&x_parameters), in_ptr(&y_parameters),
                text("sigma-graph"), GROW | UNPADDED);

            get_cached_style_info(ctx, &x_style_info, text("x"));
            get_cached_style_info(ctx, &y_style_info, text("y"));

            embedded_canvas& c = graph.canvas();

            if (is_render_pass(ctx))
            {
                rgba8 major_grid_line_color(0x40, 0x40, 0x40, 0xff);
                rgba8 minor_grid_line_color(0x30, 0x30, 0x30, 0xff);
                draw_grid_lines_for_axis(c, scene_box, minor_grid_line_color,
                    line_style(1, solid_line), 0, 50.);
                draw_grid_lines_for_axis(c, scene_box, major_grid_line_color,
                    line_style(1, solid_line), 0, 100.);
                draw_grid_lines_for_axis(c, scene_box, minor_grid_line_color,
                    line_style(1, solid_line), 1, 5.);
                draw_grid_lines_for_axis(c, scene_box, major_grid_line_color,
                    line_style(1, solid_line), 1, 10.);
            }

            auto x_points =
                gui_apply(ctx,
                    generate_sigma_points,
                    energies,
                    in(0));
            auto y_points =
                gui_apply(ctx,
                    generate_sigma_points,
                    energies,
                    in(1));

            graph.do_line(text("X"), *x_style_info, in_ptr(&y_parameters),
                x_points);
            graph.do_line(text("Y"), *y_style_info, in_ptr(&y_parameters),
                y_points);

            graph.do_highlight(ctx);

            apply_panning_tool(c, LEFT_BUTTON);
            apply_zoom_drag_tool(ctx, c, RIGHT_BUTTON);
        }

        {
            panel p(ctx, text("overlay"), TOP | RIGHT);

            {
                row_layout r(ctx);
                do_line_sample(ctx, x_style_info->color,
                    line_style(2, solid_line));
                do_styled_text(ctx, text("label"), text("X"));
            }

            {
                row_layout r(ctx);
                do_line_sample(ctx, y_style_info->color,
                    line_style(2, solid_line));
                do_styled_text(ctx, text("label"), text("Y"));
            }
        }
    }
    alia_end
}

CRADLE_DEFINE_SIMPLE_VIEW(
    sigma_graphs_view, pbs_machine_display_context,
    "sigma_graphs", "Sigma Graphs",
    {
        do_sigma_graphs(ctx,
            _field(display_ctx.machine, modeled_energies));
    })

void static
do_overview(gui_context& ctx, accessor<pbs_machine_spec> const& machine)
{
    panel p(ctx, text("content"), GROW);

    do_heading(ctx, text("subsection-heading"), text("Energies"));

    {
        flow_layout r(ctx);
        do_styled_text(ctx, text("value"),
            as_text(ctx, get_collection_size(ctx,
                _field(machine, deliverable_energies))));
        do_text(ctx, text("deliverable energies"));
    }
    {
        row_layout r(ctx);
        do_styled_text(ctx, text("value"),
            as_text(ctx, get_collection_size(ctx,
                _field(machine, modeled_energies))));
        do_text(ctx, text("modeled energies"));
    }

    {
        grid_layout g(ctx);
        {
            grid_row r(g);
            do_text(ctx, text("minimum range: "));
            do_styled_text(ctx, text("value"),
                as_text(ctx,
                    gui_apply(ctx,
                        get_min_range<pbs_deliverable_energy>,
                        _field(machine, deliverable_energies))));
            do_text(ctx, text("mm"));
        }
        {
            grid_row r(g);
            do_text(ctx, text("maximum range: "));
            do_styled_text(ctx, text("value"),
                as_text(ctx,
                    gui_apply(ctx,
                        get_max_range<pbs_deliverable_energy>,
                        _field(machine, deliverable_energies))));
            do_text(ctx, text("mm"));
        }
    }

    do_heading(ctx, text("subsection-heading"), text("Magnet SAD"));

    {
        grid_layout g(ctx);
        {
            grid_row r(g);
            do_text(ctx, text("X: "));
            do_styled_text(ctx, text("value"), as_text(ctx,
                select_index_by_value(_field(machine, sad), 0)));
            do_text(ctx, text("mm"));
        }
        {
            grid_row r(g);
            do_text(ctx, text("Y: "));
            do_styled_text(ctx, text("value"), as_text(ctx,
                select_index_by_value(_field(machine, sad), 1)));
            do_text(ctx, text("mm"));
        }
    }

    do_heading(ctx, text("subsection-heading"), text("Apparent Aperture SAD"));

    {
        grid_layout g(ctx);
        {
            grid_row r(g);
            do_text(ctx, text("X: "));
            do_styled_text(ctx, text("value"), as_text(ctx,
                select_index_by_value(_field(machine, aperture_sad), 0)));
            do_text(ctx, text("mm"));
        }
        {
            grid_row r(g);
            do_text(ctx, text("Y: "));
            do_styled_text(ctx, text("value"), as_text(ctx,
                select_index_by_value(_field(machine, aperture_sad), 1)));
            do_text(ctx, text("mm"));
        }
    }

    //do_spacer(ctx, height(1, CHARS));

    //{
    //    scoped_substyle ss(ctx, subsection_heading_style);
    //    do_text(ctx, "Validation");
    //}
    //{
    //    row_layout r(ctx);
    //    do_text(ctx, "maximum R90 discrepancy: ");
    //    {
    //        scoped_substyle ss(ctx, value_style);
    //        do_text(ctx, data->largest_r90_error);
    //    }
    //    do_text(ctx, "mm");
    //}
    //do_text(ctx, data->deliverable_sorted_properly ?
    //    "Deliverable energies are properly sorted." :
    //    "Deliverable energies are NOT properly sorted!");
    //do_text(ctx, data->modeled_sorted_properly ?
    //    "Modeled energies are properly sorted." :
    //    "Modeled energies are NOT properly sorted!");
}

CRADLE_DEFINE_SIMPLE_VIEW(
    overview_view, pbs_machine_display_context,
    "overview", "Overview",
    {
        do_overview(ctx, display_ctx.machine);
    })

display_view_composition_list static
make_default_display_composition_list()
{
    display_view_composition_list compositions;
    {
        display_view_instance_list views;
        views.push_back(display_view_instance("overview", "overview"));
        compositions.push_back(
            display_view_composition(
                "overview",
                "Overview",
                views,
                display_layout_type::MAIN_PLUS_COLUMN));
    }
    {
        display_view_instance_list views;
        views.push_back(display_view_instance("deliverable_energies",
            "deliverable_energies"));
        compositions.push_back(
            display_view_composition(
                "deliverable_energies",
                "Deliverable Energies",
                views,
                display_layout_type::MAIN_PLUS_COLUMN));
    }
    {
        display_view_instance_list views;
        views.push_back(display_view_instance("modeled_energies",
            "modeled_energies"));
        compositions.push_back(
            display_view_composition(
                "modeled_energies",
                "Modeled Energies",
                views,
                display_layout_type::MAIN_PLUS_COLUMN));
    }
    {
        display_view_instance_list views;
        views.push_back(display_view_instance("pristine_peaks",
            "pristine_peaks"));
        compositions.push_back(
            display_view_composition(
                "pristine_peaks",
                "Pristine Peaks",
                views,
                display_layout_type::MAIN_PLUS_COLUMN));
    }
    {
        display_view_instance_list views;
        views.push_back(display_view_instance("sigma_graphs", "sigma_graphs"));
        compositions.push_back(
            display_view_composition(
                "sigma_graphs",
                "Sigma Graphs",
                views,
                display_layout_type::MAIN_PLUS_COLUMN));
    }
    return compositions;
}

void do_pbs_machine_display(
    gui_context& ctx, accessor<pbs_machine_spec> const& machine,
    accessor<pbs_machine_display_state> const& state)
{
    pbs_machine_display_context display_ctx;
    display_ctx.machine = alia::ref(&machine);

    display_view_provider<pbs_machine_display_context> provider(&display_ctx);
    modeled_energies_view modeled_energies;
    provider.add_view(&modeled_energies);
    deliverable_energies_view deliverable_energies;
    provider.add_view(&deliverable_energies);
    sigma_graphs_view sigma_graphs;
    provider.add_view(&sigma_graphs);
    pristine_peaks_view pristine_peaks;
    provider.add_view(&pristine_peaks);
    overview_view overview;
    provider.add_view(&overview);

    auto display_state = get_state(ctx, make_default_display_state());
    do_display(ctx, provider,
        in(display_view_composition_list()),
        display_state,
        // This can't get the app config value it doesn't have access to the app context
        // currently, but it also isn't really used and doesn't have controls, so it's not
        // a big deal at the moment.
        get_state(ctx, 350.f),
        [&](gui_context& ctx, accessor<cradle::display_state> const& state,
            accordion& accordion)
        {
        });
}

}
