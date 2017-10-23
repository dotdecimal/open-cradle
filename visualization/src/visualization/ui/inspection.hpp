#ifndef VISUALIZATION_UI_INSPECTION_HPP
#define VISUALIZATION_UI_INSPECTION_HPP

#include <visualization/ui/views/spatial_3d_views.hpp>

namespace visualization {

// Add a 3D image to the inspection UI for a scene graph.
// Note that the units argument should be unnecessary because that's available
// directly from the image, but we don't seem to be doing a good job of
// filling that in, so for now it's just an explicit parameter.
// :format is the printf-style format string for the image value.
void
add_inspectable_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<styled_text> const& label,
    accessor<string> const& format,
    accessor<string> const& units);

}

#endif
