#ifndef CRADLE_GUI_DISPLAYS_TYPES_HPP
#define CRADLE_GUI_DISPLAYS_TYPES_HPP

#include <cradle/gui/types.hpp>
#include <cradle/imaging/color.hpp>
#include <cradle/geometry/polygonal.hpp>

#include <cradle/geometry/common.hpp>
#include <cradle/geometry/slicing.hpp>
#include <cradle/imaging/variant.hpp>
#include <cradle/geometry/regular_grid.hpp>

// This file defines some data types that are display-related but are still
// useful outside GUI code (e.g., as the result of functions).
// This file is included even for command-line only builds.

namespace cradle {

api(struct)
struct display_state
{
    optional<string> selected_composition;
    optional<string> focused_view;
    bool controls_expanded;
};

display_state static inline
make_default_display_state()
{
    display_state state;
    state.controls_expanded = false;
    return state;
}

// SPATIAL REGION DISPLAY OPTIONS - This defines more specific options for
// how to render polysets and other delineations of spatial regions.

api(enum)
enum class line_stipple_type
{
    NONE,
    SOLID,
    DASHED,
    DOTTED
};

void static inline ensure_default_initialization(line_stipple_type &type) {
    type = line_stipple_type::NONE;
}

api(struct)
struct spatial_region_fill_options
{
    bool enabled;
    float opacity;
};

api(struct)
struct spatial_region_outline_options
{
    line_stipple_type type;
    float width;
    float opacity;
};

api(enum)
enum class structure_render_mode
{
    SOLID,
    CONTOURS
};

api(struct)
struct spatial_region_display_options
{
    spatial_region_fill_options fill;
    spatial_region_outline_options outline;
    structure_render_mode render_mode;
    bool show_slice_highlight;
};

static inline
spatial_region_display_options
make_default_spatial_region_display_options()
{
    spatial_region_display_options options;
    options.fill.enabled = false;
    options.fill.opacity = 0.6f;
    options.outline.type = line_stipple_type::SOLID;
    options.outline.width = 2;
    options.outline.opacity = 1;
    options.render_mode = structure_render_mode::SOLID;
    options.show_slice_highlight = true;
    return options;
}


api(struct)
struct point_rendering_options
{
    double size;
    line_stipple_type line_type;
    double line_thickness;
};

// A lot of this should probably be moved elsewhere, since it's being used for
// purposes outside the GUI.

struct gui_point
{
    styled_text label;
    rgb8 color;
    request<vector3d> position;
};

// biological parameters for calculating EUD, Veffective, NTCP, etc.
api(struct)
struct biological_structure_parameters
{
    optional<double> a, alphabeta, gamma50, d50, cutoff;
};

struct gui_structure
{
    styled_text label;
    rgb8 color;
    request<structure_geometry> geometry;
    biological_structure_parameters biological;
};

api(struct)
struct notable_data_point
{
    string label;
    rgb8 color;
    vector2d position;
};

// gray image display options
api(struct)
struct gray_image_display_options
{
    double level, window;
};

// parameters for drr generation
api(struct)
struct drr_parameters
{
    double min_z;
    double max_z;
    double min_value;
    double max_value;
    double weight;
};

// options for drr generation
api(struct)
struct drr_options
{
    gray_image_display_options image_display_options;
    std::vector<drr_parameters> parameters;
    double image_z;
    regular_grid<2, double> sizing;
};

/// This should probably not live in cradle, but rather in visualization. But planning app
/// in thinknode needs access to it, so it can't live in visualization.
api(struct)
struct dvh_view_state
{
    bool absolute;
};

void static inline
ensure_default_initialization(dvh_view_state& x)
{
    x.absolute = false;
}

// sliced_3d_view_state stores the persistent state associated with a view
// of 3D scene.
api(struct)
struct sliced_3d_view_state
{
    vector3d slice_positions;
    bool show_hu_overlays;
};

void static inline ensure_default_initialization(sliced_3d_view_state &state)
{
    state.slice_positions = make_vector(0., 0., 0.);
}

// Get the default view state for a scene.
sliced_3d_view_state
make_default_view_state(sliced_scene_geometry<3> const& scene);

api(struct)
struct out_of_plane_information
{
    unsigned axis;
    double thickness, position;
};

// image_geometry<N> describes the geometry of an N-dimensional image.
api(struct with(N:1,2,3,4))
template<unsigned N>
struct image_geometry
{
    // the (sorted) list of slices along each axis
    c_array<N,slice_description_list> slicing;

    // out of plane information
    // An image can include information about how it fits into the next higher
    // dimensional space.
    optional<out_of_plane_information> out_of_plane_info;

    // the regular grid that corresponds to this image
    // (Even an irregularly sliced image must also provide a regularly spaced
    // version.)
    cradle::regular_grid<N, double> grid;
};


}

#endif
