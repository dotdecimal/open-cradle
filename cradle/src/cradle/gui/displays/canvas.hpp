#ifndef CRADLE_GUI_DISPLAYS_CANVAS_HPP
#define CRADLE_GUI_DISPLAYS_CANVAS_HPP

#include <cradle/rt/types.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <alia/ui/utilities/styling.hpp> // TODO: Get rid of this.

namespace cradle {

struct multiple_source_view;

api(enum internal)
enum class base_zoom_type
{
    // Allow the two scene axes to be zoomed at different levels so that the
    // scene perfectly fits the canvas.
    STRETCH_TO_FIT,
    // Maintaining the scene's aspect ratio, zoom so that the entire scene
    // fits in the canvas.
    FIT_SCENE,
    // Maintaining the scene's aspect ratio, zoom so that the entire scene
    // fits horizontally in the canvas.
    FIT_SCENE_WIDTH,
    // Maintaining the scene's aspect ratio, zoom so that the entire scene
    // fits vertically in the canvas.
    FIT_SCENE_HEIGHT,
    // Maintaining the scene's aspect ratio, zoom so that the scene just
    // barely fills the canvas.
    FILL_CANVAS,
};

// A camera includes a zoom level and a position.
// In order to facilitate display sharing, the zoom is specified relative to
// a base zoom level which is calculated according to the scene size and the
// available screen space.
// The position is the point in the scene where the canvas will be centered.
api(struct internal)
struct camera
{
    double zoom;
    vector<2,double> position;
};

ALIA_DEFINE_FLAG_TYPE(canvas)
ALIA_DEFINE_FLAG(canvas, 0x1, CANVAS_FLIP_X)
ALIA_DEFINE_FLAG(canvas, 0x2, CANVAS_FLIP_Y)
// Using this ensures that the camera stays far enough within the scene that
// the canvas is not showing anything outside the scene.
// (The only exception is if the camera is zoomed out so far that the entire
// scene fits within the canvas. In this case, the camera is simply placed
// at the center of the scene.)
ALIA_DEFINE_FLAG(canvas, 0x4, CANVAS_STRICT_CAMERA_CLAMPING)

camera make_default_camera(box<2,double> const& scene_box);

struct embedded_canvas : noncopyable
{
    embedded_canvas() : active_(false) {}

    // two-phase begin
    // initialize() initializes the canvas such that the queries below will
    // function properly.
    // begin() actually inserts the canvas into the widget tree and begins its
    // scope.
    void initialize(
        gui_context& ctx, box<2,double> const& scene_box,
        base_zoom_type base_zoom = base_zoom_type::FIT_SCENE,
        optional_storage<cradle::camera> const& camera =
            optional_storage<cradle::camera>(none),
        canvas_flag_set flags = NO_FLAGS);
    void begin(
        layout const& layout_spec = default_layout);

    void begin(
        gui_context& ctx, box<2,double> const& scene_box,
        layout const& layout_spec = default_layout,
        base_zoom_type base_zoom = base_zoom_type::FIT_SCENE,
        optional_storage<cradle::camera> const& camera =
            optional_storage<cradle::camera>(none),
        canvas_flag_set flags = NO_FLAGS);

    void end();

    // If you want to force a specific scale factor on the canvas (say,
    // to keep it in sync with another canvas), you can do so with these.
    void force_scale_factor(unsigned axis, double scale)
    { forced_scale_[axis] = scale; }

    vector<2,optional<double> > const& forced_scale() const
    { return forced_scale_; }

    gui_context& context() const { return *ctx_; }

    widget_id id() const { return id_; }
    layout_box const& region() const;

    box<2,double> const& scene_box() const { return scene_box_; }

    base_zoom_type base_zoom() const { return base_zoom_; }

    cradle::camera const& camera() const { return camera_; }

    canvas_flag_set flags() const { return flags_; }

    bool flip_x() const { return (flags_ & CANVAS_FLIP_X) != 0; }
    bool flip_y() const { return (flags_ & CANVAS_FLIP_Y) != 0; }

    // Evaluate that canvas's current zoom factor to yield a scale factor in
    // canvas pixels per scene unit.
    // (This is 2D as it can be different in X and Y.)
    vector<2,double> get_scale_factor() const;

    void set_scene_coordinates();
    void set_canvas_coordinates();

    // Get the mouse position within the canvas's frame of reference, in
    // canvas pixels.
    //
    // Unfortunately, the position is lagged one frame with respect to changes
    // in the layout of the canvas. This is difficult to solve without making
    // changes to alia's dataflow, and it's not really a problem in practice.
    //
    // The position is optional. It's none if the mouse is outside the canvas.
    //
    optional<vector<2,double> > const& mouse_position() const;

 private:
    gui_context* ctx_;
    struct data;
    data* data_;
    base_zoom_type base_zoom_;
    cradle::camera camera_;
    widget_id id_;
    box<2,double> scene_box_;
    canvas_flag_set flags_;
    bool active_;
    scoped_transformation st_;
    scoped_clip_region scr_;
    vector<2,optional<double> > forced_scale_;
};
struct canvas : raii_adaptor<embedded_canvas>
{
    canvas() {}
    canvas(
        gui_context& ctx, box<2,double> const& scene_box,
        layout const& layout_spec = default_layout,
        base_zoom_type base_zoom = base_zoom_type::FIT_SCENE,
        optional_storage<cradle::camera> const& camera =
            optional_storage<cradle::camera>(none),
        canvas_flag_set flags = NO_FLAGS)
    { begin(ctx, scene_box, layout_spec, base_zoom, camera, flags); }
};

void set_camera(embedded_canvas& canvas, cradle::camera const& new_camera);

vector<2,double> canvas_to_scene(
    embedded_canvas& c, vector<2,double> const& p);
vector<2,double> scene_to_canvas(
    embedded_canvas& c, vector<2,double> const& p);

void zoom_to_box(embedded_canvas& canvas, box<2,double> const& box);

// panning tool - This allows the user to drag the scene around using the
// specified mouse button.
void apply_panning_tool(embedded_canvas& canvas, mouse_button button);

void apply_zoom_wheel_tool(embedded_canvas& canvas, double factor = 1.1);

void draw_checker_background(embedded_canvas& canvas, rgba8 const& color1,
    rgba8 const& color2, double spacing);

// zoom box tool - This tool allows the user to zoom in to a rectangular area
// of the scene by dragging the mouse and drawing a box.
//typedef vector<2,double> zoom_box_tool_data;
//void apply_zoom_box_tool(
//    gui_context& ctx, embedded_canvas& canvas, mouse_button button,
//    rgba8 const& color, line_style const& style = line_style(1, solid_line),
//    zoom_box_tool_data* data = 0);

// Sets a mouse drag action on button to cause zooming of the canvas (must drag vertically)
void apply_zoom_drag_tool(
    gui_context& ctx, embedded_canvas& canvas, mouse_button button);

void draw_grid_lines_for_axis(
    embedded_canvas& canvas, box<2,double> const& box,
    rgba8 const& color, line_style const& style, unsigned axis, double spacing,
    unsigned skip = 0);

void draw_grid_lines(embedded_canvas& canvas, box<2,double> const& box,
    rgba8 const& color, line_style const& style,
    double spacing, unsigned skip = 0);

ALIA_DEFINE_FLAG_TYPE(ruler)
// which sides(s) the ruler should be on
ALIA_DEFINE_FLAG(ruler, 0x10, TOP_RULER)
ALIA_DEFINE_FLAG(ruler, 0x20, BOTTOM_RULER)
ALIA_DEFINE_FLAG(ruler, 0x40, LEFT_RULER)
ALIA_DEFINE_FLAG(ruler, 0x80, RIGHT_RULER)
ALIA_DEFINE_FLAG(ruler, 0xf0, RULER_SIDE_MASK)

// Sets a double click action on button to reset the canvas to zoom extents
void apply_double_click_reset_tool(
    embedded_canvas& canvas, mouse_button button);

struct sider_ruler_style_info;

struct embedded_side_rulers : noncopyable
{
 public:
    void initialize(gui_context& ctx) { ctx_ = &ctx; active_ = false; }
    void begin(
        gui_context& ctx, embedded_canvas& canvas, ruler_flag_set flags,
        layout const& layout_spec = default_layout,
        accessor<string> const& units = text(""),
        vector<2,double> const& scales = make_vector(1., 1.));
    void end();
 private:
    void do_ruler(ruler_flag_set flags, unsigned index);
    void do_ruler_row(ruler_flag_set side);
    void do_corner();
    gui_context* ctx_;
    bool active_;
    embedded_canvas* canvas_;
    keyed_data<substyle_data>* style_data_;
    grid_layout grid_;
    grid_row row_;
    ruler_flag_set flags_;
    sider_ruler_style_info const* style_;
    vector<2,double> scales_;
    keyed_data<string>* units_;
};

struct side_rulers : raii_adaptor<embedded_side_rulers>
{
    side_rulers(gui_context& ctx) { initialize(ctx); }
    side_rulers(
        gui_context& ctx, embedded_canvas& canvas, ruler_flag_set flags,
        layout const& layout_spec = default_layout,
        accessor<string> const& units = text(""),
        vector<2,double> const& scales = make_vector(1., 1.))
    { begin(ctx, canvas, flags, layout_spec, units, scales); }
};

void clear_canvas(embedded_canvas& canvas, rgba8 const& color);

void draw_scene_line(embedded_canvas& canvas,
    rgba8 const& color, line_style const& style,
    unsigned axis, double position);

// Draws a line across the scene and allows it to be dragged.
// The return value is the number of scene units the line was dragged this
// pass.
double apply_line_tool(
    embedded_canvas& canvas,
    rgba8 const& color, line_style const& style,
    unsigned axis, double position,
    widget_id line_id, mouse_button button);

// make the view zoom out as needed to match an aspect ratio
multiple_source_view scale_view_to_canvas(embedded_canvas const& ec, multiple_source_view const& view);

// Gets the view flags for setting a CT image slice view to the proper orientation based on the
// patient position and the view at hand. Note: The return flags will be applied in addition to
// the standard flags per view.
// Returns flags for each axis of slice viewing (0: x/sagittal, 1: y/coronal, 2: z/transverse)
vector<3u,canvas_flag_set> get_view_flags(
    // Patient position type (e.g. HFS, HFP, etc)
    accessor<patient_position_type> const& position);

}

#endif
