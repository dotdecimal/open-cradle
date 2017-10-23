#ifndef CRADLE_GUI_DISPLAYS_STRUCTURE_DISPLAYS_HPP
#define CRADLE_GUI_DISPLAYS_STRUCTURE_DISPLAYS_HPP

//#include <cradle/gui/displays/image_interface.hpp>
//#include <cradle/gui/displays/drawing.hpp>
//#include <cradle/gui/displays/geometry_utilities.hpp>
//#include <cradle/gui/displays/sliced_3d_canvas.hpp>

namespace cradle {

// SINGLE STRUCTURE DISPLAY

// -----------------------------------------
// Commented out unused code. Not sure if this needs to be kept so it was left here, but none
// of it is currently being used - Daniel
// -----------------------------------------

//api(struct)
//struct sliced_3d_structure_view_state
//{
//    unsigned view_axis;
//};
//
//void do_structure_display(
//    gui_context& ctx, image_interface_3d const& image,
//    accessor<gray_image_display_options> const& image_options,
//    accessor<gui_structure> const& structure,
//    accessor<spatial_region_display_options> const& spatial_region_options,
//    accessor<sliced_3d_view_state> const& camera,
//    layout const& layout_spec = default_layout);
//
//// STRUCTURE SET DISPLAY
//
//api(struct)
//struct sliced_3d_structure_set_view_state
//{
//    unsigned view_axis;
//};
//
//void do_structure_set_display(
//    gui_context& ctx, image_interface_3d const& image,
//    accessor<gray_image_display_options> const& image_options,
//    accessor<std::vector<gui_structure> > const& structures,
//    accessor<spatial_region_display_options> const& spatial_region_options,
//    accessor<std::map<string,bool> > const& structure_visibility,
//    accessor<sliced_3d_view_state> const& camera,
//    layout const& layout_spec = default_layout);
//
}

#endif
