#ifndef CRADLE_GUI_DISPLAYS_POINT_DISPLAY_HPP
#define CRADLE_GUI_DISPLAYS_POINT_DISPLAY_HPP

#include <cradle/gui/common.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>
#include <cradle/gui/displays/sliced_3d_canvas.hpp>

namespace cradle {

void do_point_rendering_options(gui_context& ctx, accessor<point_rendering_options> const& options);

}

#endif // CRADLE_GUI_DISPLAYS_POINT_DISPLAY_HPP