#include <visualization/ui/inspection.hpp>

#include <boost/format.hpp>

namespace visualization {

struct inspectable_image_3d_scene_object : spatial_3d_scene_graph_inspectable_object
{
    image_interface_3d const* image;
    keyed_data<styled_text> label;
    keyed_data<string> format;
    keyed_data<string> units;

    indirect_accessor<optional<spatial_3d_inspection_report> >
    inspect(
        gui_context& ctx,
        accessor<vector3d> const& inspection_position) const
    {
        return
            make_indirect(ctx,
                gui_apply(ctx,
                    [ ](styled_text const& label,
                        optional<double> const& value,
                        string const& format,
                        string const& units)
                    {
                        return
                            value
                          ? some(
                                spatial_3d_inspection_report(
                                    label,
                                    str(boost::format(format) % value),
                                    units))
                          : none;
                    },
                    make_accessor(&this->label),
                    image->get_point(ctx,
                        gui_apply(ctx,
                            rq_value<vector3d>,
                            inspection_position)),
                    make_accessor(&this->format),
                    make_accessor(&this->units)));
    }
};

void
add_inspectable_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<styled_text> const& label,
    accessor<string> const& format,
    accessor<string> const& units)
{
    inspectable_image_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        refresh_accessor_clone(object->label, label);
        refresh_accessor_clone(object->format, format);
        refresh_accessor_clone(object->units, units);
        add_inspectable_scene_object(scene_graph, object);
    }
}

}
