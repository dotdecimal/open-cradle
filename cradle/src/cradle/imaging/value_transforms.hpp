#ifndef CRADLE_IMAGING_VALUE_TRANSFORMS_HPP
#define CRADLE_IMAGING_VALUE_TRANSFORMS_HPP

#include <cradle/imaging/variant.hpp>

namespace cradle {

api(fun)
image3
transform_image_values_3d(
    image3 const& image, interpolated_function const& transform,
    units const& transformed_units);

}

#endif
