#include <cradle/gui/displays/image_utilities.hpp>
#include <cradle/gui/displays/regular_image.hpp>
#include <cradle/gui/displays/canvas.hpp>
#include <cradle/external/opengl.hpp>
#include <cradle/geometry/grid_points.hpp>
#include <cradle/imaging/level_window.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/gui/displays/graphing.hpp>
#include <cradle/gui/widgets.hpp>
#include <cradle/gui/requests.hpp>

namespace cradle {

static void set_color(rgba8 const& color)
{
    glColor4ub(color.r, color.g, color.b, color.a);
}

struct plot_lw_image_fn
{
    gray_image_display_options options;
    template<class T>
    void operator()(image<1,T,shared> const& img)
    {
        auto image_grid = get_grid(img);
        auto grid_points = make_grid_point_list(image_grid);
        {
        auto pixel = get_begin(img);
        glBegin(GL_QUAD_STRIP);
        for (auto const& p : grid_points)
        {
            cradle::uint8_t gray =
                apply_level_window(options.level, options.window, p[0]);
            set_color(rgba8(gray, gray, gray, 0xff));
            glVertex2d(p[0], 0);
            glVertex2d(p[0], apply(img.value_mapping, double(*pixel)));
            ++pixel;
        }
        glEnd();
        }
    }
};

static void set_line_style(line_style const& style)
{
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(style.stipple.factor, style.stipple.pattern);
    glLineWidth(style.width);
}

void plot_lw_image(gui_context& ctx, image_interface_1d const& img,
    accessor<gray_image_display_options> const& options)
{
    alia_if (is_gettable(options))
    {
        auto regular = img.get_regularly_spaced_image(ctx);
        alia_if (is_gettable(regular))
        {
            plot_lw_image_fn fn;
            fn.options = get(options);
            apply_fn_to_gray_variant(fn, get(regular));
        }
        alia_end
    }
    alia_end
}

image1 static
sanitize_histogram(image1 const& histogram)
{
    image<1,uint32_t,const_view> histogram_view =
        as_const_view(cast_variant<uint32_t>(histogram));

    // Establish a cutoff level halfway between air and tissue.
    unsigned cutoff_level =
        (std::min)(
            unsigned(transform_point(inverse(get_spatial_mapping(histogram)),
                make_vector(-500.0))[0]),
            histogram.size[0]);

    // Determine the largest bin value above the cutoff level.
    uint32_t max_bin_value = 0;
    for (unsigned i = cutoff_level; i != histogram.size[0]; ++i)
    {
        uint32_t this_bin_value = histogram_view.pixels[i];
        if (this_bin_value > max_bin_value)
            max_bin_value = this_bin_value;
    }

    image<1,uint32_t,unique> copy;
    create_image(copy, histogram_view.size);//make_vector(300u));
    //set_spatial_mapping(copy, make_vector(-1000.), make_vector(10.));
    copy_spatial_mapping(copy, histogram_view);
    copy_value_mapping(copy, histogram_view);

    // Copy the histogram, clamping all the bins.
    for (unsigned i = 0; i < histogram.size[0]; ++i)
    {
        copy.pixels.ptr[i] =
            (std::min)(histogram_view.pixels[i], max_bin_value);
    }

    return as_variant(share(copy));
}

double static
determine_bin_size(min_max<double> const& value_range)
{
    return (value_range.max - value_range.min) / 40.0;
}

double static
select_min(optional<min_max<double> > const& min_max)
{
    return get(min_max).min;
}

double static
select_max(optional<min_max<double> > const& min_max)
{
    return get(min_max).max;
}

template<class Value>
struct deflickered_data
{
    alia::owned_id input_id, output_id;
    bool is_current;
    alia__shared_ptr<Value> value;
};

// Calculate a reasonable step size for the given range.
double static
calculate_step_size(min_max<double> const& range)
{
    double step = 1;
    // Ensure there are at least 100 steps.
    while ((range.max - range.min) / step < 100)
        step /= 10;
    return step;
}

void do_rsp_image_display_options(
    gui_context& ctx, accessor<min_max<double> > const& value_range,
    accessor<gray_image_display_options> const& options)
{
    auto step_size = gui_apply(ctx, calculate_step_size, value_range);

    alia_if (is_gettable(value_range) && is_gettable(step_size))
    {
        grid_layout grid(ctx);
        auto level =
            select_field(ref(&options), &gray_image_display_options::level);
        {
            grid_row row(grid);
            do_text(ctx, text("Level:"));
            do_text_control(ctx,
                enforce_max(enforce_min(
                    select_field(ref(&options), &gray_image_display_options::level),
                    _field(value_range, min)),
                    _field(value_range, max)));
        }
        do_slider(ctx, level, get(value_range).min, get(value_range).max,
            get(step_size));
        auto window =
            select_field(ref(&options), &gray_image_display_options::window);
        {
            grid_row row(grid);
            do_text(ctx, text("Window:"));
            do_text_control(ctx,
                enforce_max(enforce_min(
                    select_field(ref(&options), &gray_image_display_options::window),
                    in(0.)),
                    in(get(value_range).max - get(value_range).min)));
        }
        do_slider(ctx, window, 0, get(value_range).max - get(value_range).min,
            get(step_size));

        auto ressel = get_state<size_t>(ctx, size_t());
        string options[] = {"Default", "Wide", "Custom"};

        {
            grid_row row(grid);
            {
                do_text(ctx, text("Presets:"));
                drop_down_list<size_t> ddl(ctx, ressel, layout(size(12,2,EM)));

                alia_if(is_gettable(ressel))
                {
                    do_text(ctx, in(options[get(ressel)]));
                }
                alia_end

                alia_if (ddl.do_list())
                {
                    for(size_t i=0; i<2; i++)
                    {
                        size_t temp = i;
                        ddl_item<size_t> item(ddl, temp);
                        do_text(ctx, in(options[i]));
                    }
                }
                alia_end

                // Set selected index based on current window/level
                alia_if(is_refresh_pass(ctx))
                {
                    alia_if (get(window) == 1. && get(level) == 1.)
                    {
                        set(ressel, size_t(0));
                    }
                    alia_else_if (get(window) == 3. && get(level) == 1.)
                    {
                        set(ressel, size_t(1));
                    }
                    alia_else
                    {
                        set(ressel, size_t(2));
                    }
                    alia_end
                }
                alia_end

                alia_if(ddl.changed())
                {
                    switch(get(ressel))
                    {
                        case 0:
                        {
                            set(window, 1.);
                            set(level, 1.);
                            break;
                        }
                        case 1:
                        {
                            set(window, 3.);
                            set(level, 1.);
                            break;
                        }
                        default:
                            break;
                    }
                }
                alia_end
            }
        }
    }
    alia_end
}

void do_gray_image_display_options(
    gui_context& ctx, accessor<min_max<double> > const& value_range,
    accessor<gray_image_display_options> const& options)
{
    //do_heading(ctx, text("section-heading"), text("Image Controls"));

    auto step_size = gui_apply(ctx, calculate_step_size, value_range);

    alia_if (is_gettable(value_range) && is_gettable(step_size))
    {
        grid_layout grid(ctx);
        auto level =
            select_field(ref(&options), &gray_image_display_options::level);
        {
            grid_row row(grid);
            do_text(ctx, text("Level"));
            do_text_control(ctx,
                enforce_max(enforce_min(
                    select_field(ref(&options), &gray_image_display_options::level),
                    _field(value_range, min)),
                    _field(value_range, max)));
        }
        do_slider(ctx, level, get(value_range).min, get(value_range).max,
            get(step_size));
        auto window =
            select_field(ref(&options), &gray_image_display_options::window);
        {
            grid_row row(grid);
            do_text(ctx, text("Window"));
            do_text_control(ctx,
                enforce_max(enforce_min(
                    select_field(ref(&options), &gray_image_display_options::window),
                    in(0.)),
                    in(get(value_range).max - get(value_range).min)));
        }
        do_slider(ctx, window, 0, get(value_range).max - get(value_range).min,
            get(step_size));

        auto ressel = get_state<size_t>(ctx, size_t());
        string options[] = {"Default", "Head", "Breast", "Thorax", "Lungs", "Pelvis", "Bone", "Custom"};

        {
            grid_row row(grid);
            {
                do_text(ctx, text("Presets"));
                drop_down_list<size_t> ddl(ctx, ressel, layout(size(12,2,EM)));

                alia_if(is_gettable(ressel))
                {
                    do_text(ctx, in(options[get(ressel)]));
                }
                alia_end

                alia_if (ddl.do_list())
                {
                    for(size_t i=0; i<7; i++)
                    {
                        size_t temp = i;
                        ddl_item<size_t> item(ddl, temp);
                        do_text(ctx, in(options[i]));
                    }
                }
                alia_end

                // Set selected index based on current window/level
                alia_if(is_refresh_pass(ctx))
                {
                    alia_if (get(window) == 400. && get(level) == 75.)
                    {
                        set(ressel, size_t(0));
                    }
                    alia_else_if (get(window) == 900. && get(level) == 180.)
                    {
                        set(ressel, size_t(1));
                    }
                    alia_else_if (get(window) == 500. && get(level) == 0.)
                    {
                        set(ressel, size_t(2));
                    }
                    alia_else_if (get(window) == 1000. && get(level) == 100.)
                    {
                        set(ressel, size_t(3));
                    }
                    alia_else_if (get(window) == 1300 && get(level) == -650.)
                    {
                        set(ressel, size_t(4));
                    }
                    alia_else_if (get(window) == 800. && get(level) == 30.)
                    {
                        set(ressel, size_t(5));
                    }
                    alia_else_if (get(window) == 475. && get(level) == 250.)
                    {
                        set(ressel, size_t(6));
                    }
                    alia_else
                    {
                        set(ressel, size_t(7));
                    }
                    alia_end
                }
                alia_end

                alia_if(ddl.changed())
                {
                    switch(get(ressel))
                    {
                        case 0:
                        {
                            set(window, 400.);
                            set(level, 75.);
                            break;
                        }
                        case 1:
                        {
                            set(window, 900.);
                            set(level, 180.);
                            break;
                        }
                        case 2:
                        {
                            set(window, 500.);
                            set(level, 0.);
                            break;
                        }
                        case 3:
                        {
                            set(window, 1000.);
                            set(level, 100.);
                            break;
                        }
                        case 4:
                        {
                            set(window, 1300.);
                            set(level, -650.);
                            break;
                        }
                        case 5:
                        {
                            set(window, 800.);
                            set(level, 30.);
                            break;
                        }
                        case 6:
                        {
                            set(window, 475.);
                            set(level, 250.);
                            break;
                        }
                        default:
                            break;
                    }
                }
                alia_end
            }
        }
    }
    alia_end
}

void do_drr_options(
    gui_context& ctx,
    accessor<bool> const& show_drrs)
{
    row_layout row(ctx);
    do_check_box(ctx, show_drrs, text("Show DRRs"), GROW_X);
}

void update_inspection_data(
    inspection_data<2>& inspection,
    embedded_canvas& canvas)
{
    dataless_ui_context& ctx = canvas.context();
    if (is_refresh_pass(ctx))
    {
        canvas.set_canvas_coordinates();
        bool hot = is_mouse_inside_box(ctx, box<2,double>(canvas.region()));
        canvas.set_scene_coordinates();
        if (hot)
            inspection.position = get_mouse_position(ctx);
    }
}

void do_image_profile_overlay(gui_context& ctx, embedded_canvas& canvas,
    accessor<image_profiling_state> const& state)
{
    uint8_t slice_opacity =
        smooth_raw_value(ctx, get(state).active ? 0xff : 0x00);
    auto tool_id = get_widget_id(ctx);
    alia_untracked_if (slice_opacity)
    {
        double delta =
            apply_line_tool(
                canvas, apply_alpha(rgb8(0x88, 0x88, 0xff), slice_opacity),
                line_style(2, solid_line), 1, get(state).position,
                tool_id, LEFT_BUTTON);
        if (delta != 0)
        {
            set(_field(ref(&state), position),
                get(state).position + delta);
        }
    }
    alia_end
}

}
