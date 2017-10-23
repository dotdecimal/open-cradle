#ifndef ANALYSIS_UI_TASKS_SPATIAL_ANALYSIS_3D_TASK_HPP
#define ANALYSIS_UI_TASKS_SPATIAL_ANALYSIS_3D_TASK_HPP

#include <cradle/gui/displays/display.hpp>
#include <cradle/gui/displays/sliced_3d_canvas.hpp>

#include <analysis/ui/common.hpp>
#include <analysis/ui/state/common_state.hpp>

namespace analysis {

void register_spatial_analysis_3d_task();

api(struct)
struct gray_image_rendering_parameters
{
    double level, window;
};

api(struct)
struct image_analysis
{
    string label;
    string thinknode_id;
    gray_image_rendering_parameters rendering;
};

api(struct)
struct image_level
{
    double value;
    // the primary color associated with the level - This is used to draw
    // isolines at this level and to shade the higher side of this level in
    // color washes and isobands.
    rgb8 color;
    // If present, dose on the lower side of this level will end at this color.
    omissible<rgb8> lower_color;
};

typedef std::vector<image_level> image_level_list;

api(struct)
struct color_wash_rendering_parameters
{
    double opacity;
};

api(struct)
struct isoline_rendering_parameters
{
    line_stipple_type type;
    float width;
    double opacity;
};

api(struct)
struct isoband_rendering_parameters
{
    double opacity;
};

api(struct)
struct dose_rendering_parameters
{
    image_level_list levels;
    optional<color_wash_rendering_parameters> color_wash;
    optional<isoband_rendering_parameters> isobands;
    optional<isoline_rendering_parameters> isolines;
};

api(struct)
struct dose_analysis
{
    string label;
    string thinknode_id;
    dose_rendering_parameters rendering;
};

api(struct)
struct spatial_region_fill_parameters
{
    double opacity;
};

api(struct)
struct spatial_region_outline_parameters
{
    line_stipple_type type;
    float width;
    double opacity;
};

api(struct)
struct spatial_region_rendering_parameters
{
    optional<spatial_region_fill_parameters> fill;
    optional<spatial_region_outline_parameters> outline;
};

api(struct)
struct structure_analysis
{
    string label;
    string thinknode_id;
    rgb8 color;
    spatial_region_rendering_parameters rendering;
};

api(struct record("analysis"))
struct spatial_analysis_3d
{
    optional<image_analysis> image;
    std::vector<structure_analysis> structures;
    std::vector<dose_analysis> doses;
};

//api(struct)
//struct spatial_analysis_3d_task_state
//{
//    size_t analysis_index;
//};
//
//void push_spatial_analysis_3d_task(
//    app_context& app_ctx, string const& parent_id,
//    size_t analysis_index);

//void do_spatial_analysis_3d_summary(
//    gui_context& ctx, app_context& app_ctx,
//    accessor<spatial_analysis_3d> const& analysis);

void do_spatial_analysis_3d_controls(
    gui_context& ctx, app_context& app_ctx,
    accessor<spatial_analysis_3d> const& analysis);

void do_spatial_analysis_3d_display(
    gui_context& ctx, app_context& app_ctx,
    accessor<spatial_analysis_3d> const& analysis);

}

#endif
