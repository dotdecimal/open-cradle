#include <cradle/gui/displays/compositions/structure_displays.hpp>
//#include <cradle/gui/displays/sliced_3d_canvas.hpp>
//#include <cradle/gui/displays/canvas.hpp>
//#include <cradle/gui/displays/display.hpp>
//#include <cradle/gui/displays/image_utilities.hpp>
//#include <cradle/gui/displays/geometry_utilities.hpp>
//#include <cradle/gui/displays/views/sliced_3d_view.hpp>
//#include <cradle/gui/widgets.hpp>
//#include <cradle/gui/collections.hpp>
//#include <cradle/gui/requests.hpp>

namespace cradle {

// SINGLE STRUCTURE DISPLAY

// -----------------------------------------
// Commented out unused code. Not sure if this needs to be kept so it was left here, but none
// of it is currently being used - Daniel
// -----------------------------------------

//struct structure_display_context
//{
//    image_interface_3d const* image;
//    indirect_accessor<gray_image_display_options> image_options;
//
//    indirect_accessor<gui_structure> structure;
//    indirect_accessor<spatial_region_display_options> spatial_region_options;
//
//    indirect_accessor<sliced_3d_view_state> camera;
//};
//
//struct structure_view_controller : sliced_3d_view_controller
//{
//    structure_display_context const* display_ctx;
//
//    void do_content(gui_context& ctx,
//        sliced_3d_canvas& c3d, embedded_canvas & c2d) const
//    {
//        draw_gray_image(ctx, get_image_slice(ctx, c3d, *display_ctx->image),
//            display_ctx->image_options);
//
//        draw_structure_slice(ctx, c3d, display_ctx->structure,
//            display_ctx->spatial_region_options);
//    }
//    void do_overlays(gui_context& ctx,
//        sliced_3d_canvas& c3d, embedded_canvas & c2d) const
//    {}
//};
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    sagittal_structure_view, structure_display_context,
//    "sagittal", "Sagittal",
//    {
//        structure_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(0u), GROW | UNPADDED);
//    })
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    coronal_structure_view, structure_display_context,
//    "coronal", "Coronal",
//    {
//        structure_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(1u), GROW | UNPADDED);
//    })
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    transverse_structure_view, structure_display_context,
//    "transverse", "Transverse",
//    {
//        structure_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(2u), GROW | UNPADDED);
//    })
//
//void static
//do_structure_display_controls(
//    gui_context& ctx, structure_display_context const& display_ctx,
//    accordion& accordion)
//{
//    grid_layout grid(ctx);
//
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
//    do_heading(ctx, text("section-heading"),
//        text("Structure Options"));
//    do_spatial_region_display_controls(ctx, grid,
//        display_ctx.spatial_region_options);
//}
//
//display_view_composition_list static
//make_default_structure_display_composition_list()
//{
//    display_view_composition_list compositions;
//    display_view_instance_list views;
//    views.push_back(display_view_instance("transverse", "transverse"));
//    views.push_back(display_view_instance("sagittal", "sagittal"));
//    views.push_back(display_view_instance("coronal", "coronal"));
//    compositions.push_back(
//        display_view_composition(
//            "default", "Default", views,
//            display_layout_type::TWO_COLUMNS));
//    return compositions;
//}
//
//void do_structure_display(
//    gui_context& ctx, image_interface_3d const& image,
//    accessor<gray_image_display_options> const& image_options,
//    accessor<gui_structure> const& structure,
//    accessor<spatial_region_display_options> const& spatial_region_options,
//    accessor<sliced_3d_view_state> const& camera,
//    layout const& layout_spec)
//{
//    structure_display_context display_ctx;
//    display_ctx.image = &image;
//    display_ctx.image_options = alia::ref(&image_options);
//    display_ctx.structure = alia::ref(&structure);
//    display_ctx.spatial_region_options = alia::ref(&spatial_region_options);
//    display_ctx.camera = alia::ref(&camera);
//
//    display_view_provider<structure_display_context> provider(&display_ctx);
//    sagittal_structure_view sagittal;
//    provider.add_view(&sagittal);
//    coronal_structure_view coronal;
//    provider.add_view(&coronal);
//    transverse_structure_view transverse;
//    provider.add_view(&transverse);
//
//    auto display_state = get_state(ctx, make_default_display_state());
//    do_display(ctx, provider,
//        in(make_default_structure_display_composition_list()),
//        display_state,
//        [&](gui_context& ctx, accessor<cradle::display_state> const& state,
//            accordion& accordion)
//        {
//            do_structure_display_controls(ctx, display_ctx, accordion);
//        });
//}
//
//// STRUCTURE SET DISPLAY
//
//struct structure_set_display_context
//{
//    image_interface_3d const* image;
//    indirect_accessor<gray_image_display_options> image_options;
//
//    indirect_accessor<std::map<string,gui_structure> > structures;
//    indirect_accessor<spatial_region_display_options> spatial_region_options;
//    indirect_accessor<std::map<string,bool> > structure_visibility;
//
//    indirect_accessor<sliced_3d_view_state> camera;
//};
//
//bool static
//    is_structure_visible(
//    std::map<string,bool> const& visibility, string const& id)
//{
//    auto iter = visibility.find(id);
//    return iter != visibility.end() && iter->second;
//}
//
//struct structure_set_view_controller : sliced_3d_view_controller
//{
//    structure_set_display_context const* display_ctx;
//
//    void do_content(gui_context& ctx,
//        sliced_3d_canvas& c3d, embedded_canvas & c2d) const
//    {
//        draw_gray_image(ctx, get_image_slice(ctx, c3d, *display_ctx->image),
//            display_ctx->image_options);
//
//        for_each(ctx,
//            [&](gui_context& ctx, accessor<string> const& id,
//                accessor<gui_structure> const& structure) -> void
//            {
//                auto visible =
//                    gui_apply(ctx,
//                        is_structure_visible,
//                        display_ctx->structure_visibility,
//                        id);
//                alia_if (is_gettable(visible) && get(visible))
//                {
//                    draw_structure_slice(ctx, c3d, structure,
//                        display_ctx->spatial_region_options);
//                }
//                alia_end
//            },
//            display_ctx->structures);
//    }
//    void do_overlays(gui_context& ctx,
//        sliced_3d_canvas& c3d, embedded_canvas & c2d) const
//    {}
//};
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    sagittal_structure_set_view, structure_set_display_context,
//    "sagittal", "Sagittal",
//    {
//        structure_set_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(0u), GROW | UNPADDED);
//    })
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    coronal_structure_set_view, structure_set_display_context,
//    "coronal", "Coronal",
//    {
//        structure_set_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(1u), GROW | UNPADDED);
//    })
//
//CRADLE_DEFINE_SIMPLE_VIEW(
//    transverse_structure_set_view, structure_set_display_context,
//    "transverse", "Transverse",
//    {
//        structure_set_view_controller controller;
//        controller.display_ctx = &display_ctx;
//        do_sliced_3d_view(ctx, controller,
//            gui_request(ctx,
//                get_sliced_scene_for_image(ctx, *display_ctx.image)),
//            display_ctx.camera, in(2u), GROW | UNPADDED);
//    })
//
//void static
//do_structure_set_display_controls(
//    gui_context& ctx, structure_set_display_context const& display_ctx,
//    accordion& accordion)
//{
//    grid_layout grid(ctx);
//
//    do_separator(ctx);
//
//    do_gray_image_display_options(ctx, *display_ctx.image,
//        display_ctx.image_options);
//
//    do_separator(ctx);
//
//    do_heading(ctx, text("section-heading"), text("Structures"));
//    do_structure_selection_controls(ctx, display_ctx.structures,
//        display_ctx.structure_visibility);
//
//    do_separator(ctx);
//
//    do_heading(ctx, text("section-heading"),
//        text("Structure Options"));
//    do_spatial_region_display_controls(ctx, grid,
//        display_ctx.spatial_region_options);
//}
//
//display_view_composition_list static
//make_default_structure_set_display_composition_list()
//{
//    display_view_composition_list compositions;
//    display_view_instance_list views;
//    views.push_back(display_view_instance("transverse", "transverse"));
//    views.push_back(display_view_instance("sagittal", "sagittal"));
//    views.push_back(display_view_instance("coronal", "coronal"));
//    compositions.push_back(
//        display_view_composition(
//            "default", "Default", views,
//            display_layout_type::TWO_COLUMNS));
//    return compositions;
//}
//
//void do_structure_set_display(
//    gui_context& ctx, image_interface_3d const& image,
//    accessor<gray_image_display_options> const& image_options,
//    accessor<std::map<string,gui_structure> > const& structures,
//    accessor<spatial_region_display_options> const& spatial_region_options,
//    accessor<sliced_3d_view_state> const& camera,
//    layout const& layout_spec)
//{
//    structure_set_display_context display_ctx;
//    display_ctx.image = &image;
//    display_ctx.image_options = alia::ref(&image_options);
//    display_ctx.structures = alia::ref(&structures);
//    display_ctx.spatial_region_options = alia::ref(&spatial_region_options);
//    display_ctx.camera = alia::ref(&camera);
//
//    display_view_provider<structure_set_display_context>
//        provider(&display_ctx);
//    sagittal_structure_set_view sagittal;
//    provider.add_view(&sagittal);
//    coronal_structure_set_view coronal;
//    provider.add_view(&coronal);
//    transverse_structure_set_view transverse;
//    provider.add_view(&transverse);
//
//    auto display_state = get_state(ctx, make_default_display_state());
//    do_display(ctx, provider,
//        in(make_default_structure_set_display_composition_list()),
//        display_state,
//        [&](gui_context& ctx, accessor<cradle::display_state> const& state,
//            accordion& accordion)
//        {
//            do_structure_set_display_controls(ctx, display_ctx, accordion);
//        });
//}

}
