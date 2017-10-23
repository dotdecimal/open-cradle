#include <alia/ui/api.hpp>
#include <alia/ui/utilities/rendering.hpp>

#include <cradle/gui/displays/compositions/point_display.hpp>
#include <cradle/gui/widgets.hpp>

namespace cradle {

void do_point_rendering_options(gui_context& ctx, accessor<point_rendering_options> const& options)
{
    alia::form form(ctx);
    {
        form_field field(form, text("Size"));
        do_slider(ctx, _field(options, size), 0.0, 20.0, 0.5);
    }
    {
        form_field field(form, text("Line Type"));
        do_enum_drop_down(
            ctx,
            _field(ref(&options), line_type),
            width(12, CHARS));
    }
    {
        form_field field(form, text("Thickness"));
        do_slider(ctx, _field(options, line_thickness), 0.0, 20.0, 0.5);
    }
}

}
