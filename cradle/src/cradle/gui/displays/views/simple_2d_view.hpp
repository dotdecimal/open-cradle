#ifndef CRADLE_GUI_DISPLAYS_VIEWS_SIMPLE_2D_VIEW_HPP
#define CRADLE_GUI_DISPLAYS_VIEWS_SIMPLE_2D_VIEW_HPP

#include <cradle/gui/displays/canvas.hpp>
#include <cradle/gui/displays/graphing.hpp>
#include <cradle/geometry/scenes.hpp>

namespace cradle {

api(struct internal)
struct point_sample_2d
{
    vector<2,double> position;
    rgb8 color;
};

api(struct internal)
struct line_profile
{
    unsigned axis;
    double position;
    rgb8 color;
};

api(struct internal)
struct simple_2d_view_measurement_state
{
    std::vector<line_profile> profiles;
    std::vector<point_sample_2d> point_samples;
};

struct simple_2d_view_controller
{
    virtual void
    do_content(gui_context& ctx, embedded_canvas& canvas) const = 0;

    virtual void do_overlays(gui_context& ctx) const = 0;

    virtual indirect_accessor<data_reporting_parameters>
    get_spatial_parameters(gui_context& ctx) const = 0;

    virtual indirect_accessor<optional<min_max<double> > >
    get_profile_value_range(gui_context& ctx) const = 0;

    virtual void
    do_profile_content(gui_context& ctx, line_graph& graph,
        accessor<line_profile> const& profile) const = 0;
};

void
do_simple_2d_view(
    gui_context& ctx,
    simple_2d_view_controller const& controller,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<simple_2d_view_measurement_state> const& state,
    layout const& layout_spec);

void
do_simple_2d_view_controls(
    gui_context& ctx,
    simple_2d_view_controller const& controller,
    accessor<sliced_scene_geometry<2> > const& scene_geometry,
    accessor<simple_2d_view_measurement_state> const& state);

}

#endif
