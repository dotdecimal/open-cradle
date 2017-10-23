//#include <analysis/ui/displays/spatial_display_2d.hpp>
//#include <cradle/gui/displays/image_utilities.hpp>
//#include <cradle/gui/widgets.hpp>
//#include <cradle/gui/displays/canvas.hpp>
//#include <cradle/gui/displays/display.hpp>
//#include <cradle/gui/displays/views/simple_2d_view.hpp>
//#include <cradle/gui/displays/views/sliced_3d_view.hpp>
//#include <cradle/gui/collections.hpp>
//#include <cradle/gui/requests.hpp>
//#include <alia/ui/utilities.hpp>
//#include <cradle/gui/displays/inspection.hpp>
//#include <cradle/imaging/inclusion_image.hpp>
//#include <dosimetry/dvh.hpp>
//#include <cradle/gui/displays/regular_image.hpp>
//
//namespace analysis {
//
//// 2D DISPLAY
//
//struct dose2_display_context
//{
//    image_interface_2d const* image;
//    indirect_accessor<gray_image_display_options> image_options;
//
//    image_interface_2d const* dose;
//    indirect_accessor<dose_display_options> dose_options;
//
//    indirect_accessor<simple_2d_view_measurement_state> measurement;
//};
//
//struct dose2_view_controller : simple_2d_view_controller
//{
//    dose2_display_context const* display_ctx;
//    void do_content(gui_context& ctx, embedded_canvas& canvas) const
//    {
//        alia_if (display_ctx->image)
//        {
//            draw_gray_image(ctx, *display_ctx->image,
//                display_ctx->image_options);
//        }
//        alia_end
//
//        draw_dose(ctx, in(dose_color_config()),
//            display_ctx->dose_options, *display_ctx->dose);
//    }
//    void do_overlays(gui_context& ctx) const
//    {
//    }
//    indirect_accessor<data_reporting_parameters>
//    get_spatial_parameters(gui_context& ctx) const
//    {
//        return alia::ref(erase_type(ctx,
//            in(data_reporting_parameters("position", "mm", 1))));
//    }
//    indirect_accessor<optional<min_max<double> > >
//    get_profile_value_range(gui_context& ctx) const
//    {
//        return alia::ref(erase_type(ctx,
//            display_ctx->dose->get_value_range(ctx)));
//    }
//    void do_profile_content(gui_context& ctx, line_graph& graph,
//        accessor<line_profile> const& profile) const
//    {
//    }
//};
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    simple_2d_dose_view, dose2_display_context, "simple", "2D View",
//    {
//        dose2_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_simple_2d_view(ctx, controller,
//            gui_request(ctx, get_sliced_scene_for_image(ctx, *display_ctx.dose)),
//            display_ctx.measurement,
//            GROW | UNPADDED);
//    })
//
//void static
//do_dose2_display_controls(
//    gui_context& ctx, dose2_display_context const& display_ctx,
//    accordion& accordion)
//{
//    alia_if (display_ctx.image)
//    {
//        do_separator(ctx);
//
//        do_gray_image_display_options(ctx, *display_ctx.image,
//            display_ctx.image_options);
//    }
//    alia_end
//
//    do_separator(ctx);
//
//    //do_dose_level_list_ui(
//    //    ctx, text("Dose Levels"),
//    //    _field(display_ctx.dose_options, levels));
//
//    //do_separator(ctx);
//
//    //do_dose_display_style_options(
//    //    ctx, text("Dose Style"),
//    //    _field(display_ctx.dose_options, style));
//}
//
//display_view_composition_list
//make_default_dose2_display_composition_list()
//{
//    display_view_composition_list compositions;
//    display_view_instance_list views;
//    views.push_back(display_view_instance("simple", "simple"));
//    compositions.push_back(
//        display_view_composition(
//            "default",
//            "Default",
//            views,
//            display_layout_type::MAIN_PLUS_COLUMN));
//    return compositions;
//}
//
//void do_dose2_display(
//    gui_context& ctx,
//    image_interface_2d const* image,
//    accessor<gray_image_display_options> const& image_options,
//    image_interface_2d const& dose,
//    accessor<dose_display_options> const& dose_options,
//    layout const& layout_spec)
//{
//    dose2_display_context display_ctx;
//    display_ctx.image = image;
//    display_ctx.image_options = alia::ref(&image_options);
//    display_ctx.dose = &dose;
//    display_ctx.dose_options = alia::ref(&dose_options);
//
//    display_view_provider<dose2_display_context> provider(&display_ctx);
//    simple_2d_dose_view simple;
//    provider.add_view(&simple);
//
//    auto display_state = get_state(ctx, make_default_display_state());
//
//    do_display(ctx, provider,
//        in(make_default_dose2_display_composition_list()), // TODO
//        display_state,
//        [&](gui_context& ctx, accessor<cradle::display_state> const& state,
//            accordion& accordion)
//        {
//            do_dose2_display_controls(ctx, display_ctx, accordion);
//        });
//}
//
//}
