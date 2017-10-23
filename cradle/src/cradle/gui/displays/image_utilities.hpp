#ifndef CRADLE_GUI_DISPLAYS_IMAGE_UTILITIES_HPP
#define CRADLE_GUI_DISPLAYS_IMAGE_UTILITIES_HPP

#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/image_interface.hpp>
#include <cradle/geometry/scenes.hpp>

#include <cradle/gui/displays/types.hpp>

namespace cradle {

template<unsigned N>
request<sliced_scene_geometry<N> >
compose_sliced_scene_geometry_request(
    request<image_geometry<N> > const& image_geometry)
{
    return
        rq_foreground(
            rq_make_sliced_scene_geometry(
                rq_property(image_geometry, slicing)));
}

// Get the scene geometry for an image.
template<unsigned N>
indirect_accessor<request<sliced_scene_geometry<N> > >
get_sliced_scene_for_image(
    gui_context& ctx, image_interface<N> const& image)
{
    return make_indirect(ctx,
        gui_apply(ctx,
            compose_sliced_scene_geometry_request<N>,
            image.get_geometry_request(ctx)));
}

void do_gray_image_display_options(
    gui_context& ctx, accessor<min_max<double> > const& value_range,
    accessor<gray_image_display_options> const& options);

void do_rsp_image_display_options(
    gui_context& ctx, accessor<min_max<double> > const& value_range,
    accessor<gray_image_display_options> const& options);

void do_drr_options(
    gui_context& ctx,
    accessor<bool> const& show_drrs);


template<unsigned N>
struct inspection_data
{
    optional<vector<N,double> > position;
};

struct embedded_canvas;

void update_inspection_data(
    inspection_data<2>& inspection,
    embedded_canvas& canvas);

struct sliced_3d_canvas;

void update_inspection_data(
    inspection_data<3>& inspection,
    sliced_3d_canvas& canvas);

struct image_profiling_state
{
    bool active;
    double position;

    image_profiling_state() : active(false), position(0) {}
};

void do_image_profile_overlay(gui_context& ctx, embedded_canvas& c,
    accessor<image_profiling_state> const& state);

void do_image_profile_panel(gui_context& ctx, embedded_canvas& c,
    image_interface_2d const& image,
    accessor<image_profiling_state> const& state);

}

#endif
