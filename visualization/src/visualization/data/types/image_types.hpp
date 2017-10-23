#ifndef VISUALIZATION_DATA_TYPES_IMAGE_TYPES_HPP
#define VISUALIZATION_DATA_TYPES_IMAGE_TYPES_HPP

#include <visualization/data/types/geometry_types.hpp>

// This file defines types that are relevant for rendering images.

namespace visualization {

api(struct internal)
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

api(struct internal)
struct color_wash_rendering_parameters
{
    double opacity;
};

api(struct internal)
struct isoline_rendering_parameters
{
    line_stipple_type type;
    float width;
    double opacity;
};

api(struct internal)
struct isoband_rendering_parameters
{
    double opacity;
};

typedef gray_image_display_options gray_image_rendering_parameters;

}

#endif
