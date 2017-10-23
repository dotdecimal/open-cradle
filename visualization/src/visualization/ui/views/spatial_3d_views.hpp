#ifndef VISUALIZATION_UI_VIEWS_SPATIAL_3D_VIEWS_HPP
#define VISUALIZATION_UI_VIEWS_SPATIAL_3D_VIEWS_HPP

#include <visualization/ui/common.hpp>

#include <cradle/gui/displays/display.hpp>
#include <cradle/gui/displays/projected_canvas.hpp>
#include <cradle/gui/displays/sliced_3d_canvas.hpp>

// forward declarations
namespace cradle {
template<unsigned N>
struct sliced_scene_geometry;
struct sliced_3d_canvas;
struct embedded_canvas;
struct image_interface_3d;
struct gray_image_display_options;
struct gui_point;
struct gui_structure;
struct spatial_region_display_options;
struct point_rendering_options;
struct dose_rendering_options;
struct line_style;
struct sliced_3d_view_state;
enum class patient_position_type;
}

namespace visualization {

// The following interface is used by application code to specify the contents
// of a 3D scene. spatial_3d_scene_graph is used to accumulate the contents,
// but it's only defined internally.

struct spatial_3d_scene_graph;

// Set the geometry of the 3D scene. - This determines the extents of the
// scene and also the positions that will be chosen for sliced views of the
// scene.
void
set_scene_geometry(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<sliced_scene_geometry<3> > const& scene_geometry,
    // TODO: This should be optional and the views should fall back to "X",
    // "Y, and "Z" labels for the axes.
    accessor<patient_position_type> const& patient_position);

// This defines the canvas_layers in which 2D content can be drawn. Objects
// are sorted according by layer before rendering, so objects in higher layers
// will appear above those in lower layers. (Objects in the same layer will be
// rendered according to the order that they were added to the scene.)
//
// Note that there are gaps between the layers defined here. This is to allow
// for the definition of custom layers in between.
typedef int32_t canvas_layer;
// This layer is used for background content (e.g., the patient image).
canvas_layer static const BACKGROUND_CANVAS_LAYER       = 0x100;
// This layer is used for the primary foreground content.
canvas_layer static const FOREGROUND_CANVAS_LAYER       = 0x200;
// This layer is used for overlaying filled polygons and other large objects
// over the primary content.
canvas_layer static const FILLED_OVERLAY_CANVAS_LAYER   = 0x300;
// This layer is used for overlaying lines.
canvas_layer static const LINE_OVERLAY_CANVAS_LAYER     = 0x400;
// This layer is used for overlaying points.
canvas_layer static const POINT_OVERLAY_CANVAS_LAYER    = 0x500;

// This defines the interface that must be implemented by objects for them
// to be rendered as 3D objects in projected views of a 3D scene.
struct spatial_3d_scene_graph_projected_3d_object
{
    // Render the object on the given canvas.
    virtual void
    render(
        gui_context& ctx,
        projected_canvas& canvas) const = 0;

    // Get the depth of the object along the Z axis.
    virtual indirect_accessor<double>
    get_z_depth(
        gui_context& ctx,
        projected_canvas& canvas) const = 0;

    // Get the opacity (from 0 to 1) of the object.
    virtual indirect_accessor<double>
    get_opacity(gui_context& ctx) const = 0;

    // The following are managed internally.
    // (Maybe these should be hidden at some point.)

    // used to uniquely identify this scene object
    local_identity id_;

    // the next object in the graph's linked list
    spatial_3d_scene_graph_projected_3d_object* next_;
};

// Add a projected object to a 3D scene (for the current frame).
// Note that this should only be called on refresh passes.
void
add_projected_3d_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_projected_3d_object* object);

// This defines the interface that must be implemented by objects for them
// to be rendered in sliced views of a 3D scene.
struct spatial_3d_scene_graph_sliced_object
{
    // Render the object on the given sliced canvas.
    virtual void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const = 0;

    // The following are managed internally.
    // (Maybe these should be hidden at some point.)

    // the layer in which this object should appear in sliced views
    canvas_layer layer_;

    // used to uniquely identify this scene object
    local_identity id_;

    // the next object in the graph's linked list
    spatial_3d_scene_graph_sliced_object* next_;
};

// Add a sliced object to a 3D scene (for the current frame).
// Note that this should only be called on refresh passes.
void
add_sliced_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_sliced_object* object,
    canvas_layer layer);

api(struct internal)
struct spatial_3d_inspection_report
{
    styled_text label;
    string value;
    string units;
};

// This defines the interface that must be implemented by objects for them
// to be inspected by hovering the mouse over them.
struct spatial_3d_scene_graph_inspectable_object
{
    // Render the object on the given sliced canvas.
    virtual indirect_accessor<optional<spatial_3d_inspection_report> >
    inspect(
        gui_context& ctx,
        accessor<vector3d> const& inspection_position) const = 0;

    // The following are managed internally.
    // (Maybe these should be hidden at some point.)

    // used to uniquely identify this scene object
    local_identity id_;

    // the next object in the graph's linked list
    spatial_3d_scene_graph_inspectable_object* next_;
};

// Add an inspectable object to a 3D scene (for the current frame).
// Note that this should only be called on refresh passes.
void
add_inspectable_scene_object(
    spatial_3d_scene_graph& scene_graph,
    spatial_3d_scene_graph_inspectable_object* object);

// A spatial_3d_view_controller is provided by the application code that wants
// to instantiate a spatial_3d_view. It's used to control content and
// implement custom behaviors.
struct spatial_3d_view_controller
{
    // Generate the scene graph. The application should use the above
    // interface to add any content that should be part of the scene for the
    // current frame.
    virtual void
    generate_scene(gui_context& ctx,
        spatial_3d_scene_graph& scene_graph) = 0;

    // TODO: The following are all specific to a particular view, so they
    // should have access to the view's instance ID to allow for view-specific
    // behavior.
    // TODO: Some of the following should probably be unified.

    // Do any custom tool interactions for a sliced 3D view.
    virtual void
    do_sliced_tools(gui_context& ctx,
        sliced_3d_canvas& c3d, embedded_canvas& c2d)
    {
    }

    // Do any other UI elements layered over a sliced 3D view.
    virtual void
    do_sliced_layered_ui(gui_context& ctx)
    {
    }

    // Do any custom tool interactions for a projected view that operate on
    // the projected canvas.
    virtual void
    do_projected_tools(gui_context& ctx, projected_canvas& c3d)
    {
    }

    // Do any custom tool interactions for a projected view that operate on
    // the 2D canvas.
    virtual void
    do_2d_tools(gui_context& ctx, embedded_canvas& c2d)
    {
    }

    // Do any other UI elements layered over a sliced 3D view.
    virtual void
    do_projected_layered_ui(gui_context& ctx)
    {
    }
};

// The state that is manipulated directly by the spatial 3D views.
// TODO: This should use a single structure (like the following) that can
// capture both the sliced view state and the true 3D view state, but to
// facilitate integrating intermediate forms of this into the planning, I've
// disabled this in favor of using separate states.
// Once the planning app is more fully integrated with this, we can hopefully
// change this.
//api(struct internal)
//struct spatial_3d_view_state
//{
//    sliced_3d_view_state sliced;
//};

api(struct internal)
struct projected_3d_view_state
{
    vector3d direction, up, center;
    box<2,double> display_surface;
};

// The actual view definitions that follow are essentially internal details,
// but they have to be exposed here so that callers can create them on the
// stack.

struct spatial_3d_projected_view : display_view_interface<null_display_context>
{
    spatial_3d_projected_view() {}

    // Initialize the view. This is done internally when it's added.
    void initialize(
        spatial_3d_view_controller* controller,
        spatial_3d_scene_graph const* scene_graph);

    // Implementation of the display_view_interface interface...

    string const& get_type_id() const;

    string const&
    get_type_label(null_display_context const& display_ctx);

    indirect_accessor<string>
    get_view_label(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id);

    void
    do_view_content(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id,
        bool is_preview);

 private:
    spatial_3d_view_controller* controller_;
    spatial_3d_scene_graph const* scene_graph_;
    //indirect_accessor<projected_3d_view_state> state_;
};

struct spatial_3d_sliced_view : display_view_interface<null_display_context>
{
    spatial_3d_sliced_view() {}

    // Initialize the view. This is done internally when it's added.
    void initialize(
        spatial_3d_view_controller* controller,
        spatial_3d_scene_graph const* scene_graph,
        indirect_accessor<sliced_3d_view_state> state,
        unsigned view_axis);

    // Implementation of the display_view_interface interface...

    string const& get_type_id() const;

    string const&
    get_type_label(null_display_context const& display_ctx);

    indirect_accessor<string>
    get_view_label(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id);

    void
    do_view_content(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id,
        bool is_preview);

 private:
    spatial_3d_view_controller* controller_;
    spatial_3d_scene_graph const* scene_graph_;
    indirect_accessor<sliced_3d_view_state> state_;
    unsigned view_axis_;
};

struct spatial_3d_views
{
    spatial_3d_projected_view projected;
    spatial_3d_sliced_view sliced[3];
};

ALIA_DEFINE_FLAG_TYPE(spatial_3d)
ALIA_DEFINE_FLAG(spatial_3d, 0x1, SPATIAL_3D_NO_PROJECTED_VIEW)

// Add the spatial 3D views to the given display provider.
// :views must be declared in the stack frame of the provider.
void
add_spatial_3d_views(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    spatial_3d_views& views,
    spatial_3d_view_controller* controller,
    indirect_accessor<sliced_3d_view_state> state,
    spatial_3d_flag_set flags = NO_FLAGS);

// Add the sliced 3D views to a list of views for a display composition.
// This will add them in the correct order if you're using a 2x2 composition
// and these are the first three views.
void
add_sliced_3d_views(display_view_instance_list& views);

// Create the default display composition for the 3D spatial views.
display_view_composition
make_default_spatial_3d_view_composition(string const& label = "Ortho + 3D");

// Create the default display composition for the 3D spatial views.
display_view_composition
make_default_ortho_bev_view_composition(string const& label = "Ortho + BEV");

// Create the display composition for the 3D spatial views.
display_view_composition
make_spatial_3d_view_composition(string const& label = "Ortho + 3D");

// Create the display composition for the 3D spatial views.
display_view_composition
make_ortho_bev_view_composition(string const& label = "Ortho + BEV");

// Create the display composition for the Transverse, 3D, and BEV view
display_view_composition
make_3d_bev_view_composition(string const& label = "3D + BEV");

// FIXED PROJECTED VIEW

// TODO: This should be unified with the other spatial 3D views as a normal
// projected view with a special type of camera.

struct fixed_projected_view_controller
{
    // Generate the scene graph. The application should use the above
    // interface to add any content that should be part of the scene for the
    // current frame.
    virtual void
    generate_scene(gui_context& ctx,
        spatial_3d_scene_graph& scene_graph) = 0;

    // TODO: The following are all specific to a particular view, so they
    // should have access to the view's instance ID to allow for view-specific
    // behavior.

    // Get the view geometry associated with the view.
    virtual indirect_accessor<multiple_source_view>
    get_view_geometry(gui_context& ctx) = 0;

    // Do any custom tool interactions for the view that operate on the
    // projected canvas.
    virtual void
    do_projected_tools(gui_context& ctx, projected_canvas& pc)
    {
    }

    // Do any custom tool interactions for the view that operate on the 2D
    // canvas.
    virtual void
    do_2d_tools(gui_context& ctx, embedded_canvas& c2d)
    {
    }

    // Do any other UI elements layered over the view.
    virtual void
    do_layered_ui(gui_context& ctx)
    {
    }
};

struct spatial_3d_fixed_projected_view
  : display_view_interface<null_display_context>
{
    spatial_3d_fixed_projected_view(
        string const& id = "fixed_projected_view",
        string const& label = "BEV")
    {
        type_id_ = id;
        type_label_ = label;
    }

    // Initialize the view. This is done internally when it's added.
    void initialize(
        fixed_projected_view_controller* controller,
        spatial_3d_scene_graph const* scene_graph);

    // Implementation of the display_view_interface interface...

    string const& get_type_id() const;

    string const&
    get_type_label(null_display_context const& display_ctx);

    indirect_accessor<string>
    get_view_label(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id);

    void
    do_view_content(
        gui_context& ctx,
        null_display_context const& display_ctx,
        string const& instance_id,
        bool is_preview);

 private:
    fixed_projected_view_controller* controller_;
    spatial_3d_scene_graph const* scene_graph_;
    string type_id_;
    string type_label_;
    //indirect_accessor<projected_3d_view_state> state_;
};

// Add a projected 3D view where the camera is fixed in 3D.
// :views must be declared in the stack frame of the provider.
void
add_fixed_projected_3d_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    spatial_3d_fixed_projected_view& view,
    fixed_projected_view_controller* controller);

}

#endif
