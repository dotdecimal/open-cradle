#ifndef VISUALIZATION_DATA_TYPES_GEOMETRY_TYPES_HPP
#define VISUALIZATION_DATA_TYPES_GEOMETRY_TYPES_HPP

#include <cradle/gui/types.hpp>

// TODO: Get rid of this file and move its contents to the visualization app.
#include <cradle/gui/displays/types.hpp>

#include <visualization/common.hpp>

namespace visualization {

typedef spatial_region_display_options spatial_region_rendering_parameters;

typedef spatial_region_outline_options spatial_region_outline_parameters;

typedef point_rendering_options point_rendering_parameters;

api(struct internal)
struct spatial_region_projected_rendering_parameters
{
    structure_render_mode render_mode;
    double opacity;
    bool highlighted;
};

}

#endif
