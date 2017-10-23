#include <gui_demo/utilities.hpp>

namespace gui_demo {

DEFINE_DEMO(
    square_wave_demo, "Square Wave",
    "One of the signals you can generate is a square wave.\n\n"
    "It alternates between true and false.\n\n"
    "Note that all durations in alia are specified in milliseconds.",
    void do_ui(gui_context& ctx)
    {
        do_color(ctx, in(square_wave(ctx, 1000) ? purple : silver));
    })

DEFINE_DEMO(
    asymmetric_square_wave_demo, "Asymmetric Square Wave",
    "A square wave can spend different amounts of time in the two states.",
    void do_ui(gui_context& ctx)
    {
        do_color(ctx, in(square_wave(ctx, 350, 1000) ? fuchsia : silver));
    })

DEFINE_DEMO(
    tick_demo, "Tick Counter",
    "Another signal that's available is a simple millisecond tick counter.\n\n"
    "It's a nice building block for doing any sort of animation.\n\n"
    "Be aware that your UI will be forced to update frequently as long as you're calling this.",
    void do_ui(gui_context& ctx)
    {
        double x = (sin(get_animation_tick_count(ctx) / 500.) + 1) / 2;
        do_color(ctx, in(interpolate(silver, navy, x)));
    })

static demo_interface* signals_demos[] =
  { &square_wave_demo, &asymmetric_square_wave_demo, &tick_demo, 0 };

static demo_section signals_section =
    { "Signals", "Since alia is reactive, any variable (or expression) in your UI can be thought of as a signal that changes over time. Normally, these changes occur as a result of interaction with the user, but alia also provides functions for generating signals that change automatically over time.", signals_demos };

DEFINE_DEMO(
    simple_smoothing_demo, "Smoothed Square Wave",
    "Here, the square wave signal from above is smoothed out.",
    void do_ui(gui_context& ctx)
    {
        rgb8 signal = square_wave(ctx, 1000) ? purple : silver;
        do_color(ctx, smooth_value(ctx, in(signal)));
    })

DEFINE_DEMO(
    input_smoothing_demo, "Input Smoothing",
    "Of course, since user inputs are signals, they can also be smoothed.\n\n"
    "Try entering numbers into the text control and watch the smoothed view of its value change.",
    void do_ui(gui_context& ctx)
    {
        state_accessor<int> n = get_state(ctx, 0);
        do_text_control(ctx, n);
        do_text(ctx, smooth_value(ctx, n));
    })

DEFINE_DEMO(
    smoothing_curves_demo, "Smoothing Parameters",
    "You have control over the duration of the smoothed transition and the curve that it follows.",
    void do_ui(gui_context& ctx)
    {
        rgb8 signal = square_wave(ctx, 1000) ? teal : silver;
        {
            row_layout row(ctx);
            do_color(ctx, smooth_value(ctx, in(signal),
                animated_transition(default_curve, 700)));
            do_color(ctx, smooth_value(ctx, in(signal),
                animated_transition(linear_curve, 700)));
            do_color(ctx, smooth_value(ctx, in(signal),
                animated_transition(ease_in_out_curve, 700)));
        }
    })

static demo_interface* smoothing_demos[] =
  { &simple_smoothing_demo, &input_smoothing_demo, &smoothing_curves_demo, 0 };

static demo_section smoothing_section =
    { "Smoothing", "Signals that change abruptly can be transformed into smoothly changing signals using the smooth_value function.", smoothing_demos };

DEFINE_DEMO(
    simple_delay_demo, "Simple Delay",
    "Enter text into the text control below. The line below it will show a delayed view of the control's value.",
    void do_ui(gui_context& ctx)
    {
        state_accessor<string> x = get_state<string>(ctx, "");
        do_text_control(ctx, x, default_layout, TEXT_CONTROL_IMMEDIATE);
        do_paragraph(ctx, delay_value(ctx, x, 500));
    })

DEFINE_DEMO(
    cascading_demo, "Cascading, Composition",
    "You can easily create cascading effects by using many delayed views with increasing lag times.\n\n"
    "Of course, you can also compose smoothing and delaying.",
    void do_ui(gui_context& ctx)
    {
        state_accessor<bool> on = get_state(ctx, false);
        rgb8 signal = get(on) ? purple : silver;
        {
            row_layout row(ctx);
            for (int i = 0; i != 20; ++i)
            {
                do_color(ctx, smooth_value(ctx,
                    delay_value(ctx, in(signal), i * 100)));
            }
        }
        if (do_link(ctx, text("toggle signal")))
        {
            set(on, !get(on));
        }
    })

static demo_interface* delay_demos[] =
  { &simple_delay_demo, &cascading_demo, 0 };

static demo_section delay_section =
    { "Delays", "delay_value produces a delayed view of its input signal.", delay_demos };

DEFINE_DEMO(
    one_shot_demo, "One-shot Timer",
    "This demonstrates a simple one-shot timer.",
    void do_ui(gui_context& ctx)
    {
        state_accessor<int> tick_count = get_state(ctx, 0);
        do_text(ctx, printf(ctx, "ticks: %d", tick_count));
        timer t(ctx);
        if (t.triggered())
        {
            set(tick_count, get(tick_count) + 1);
        }
        alia_if (t.is_active())
        {
            row_layout row(ctx);
            do_text(ctx, text("active"));
            if (do_link(ctx, text("stop")))
            {
                t.stop();
            }
        }
        alia_else
        {
            if (do_link(ctx, text("start")))
            {
                t.start(1000);
            }
        }
        alia_end
    })

DEFINE_DEMO(
    periodic_timer_demo, "Periodic Timer",
    "A periodic timer is implemented by simply restarting it each time it triggers.",
    void do_ui(gui_context& ctx)
    {
        state_accessor<int> tick_count = get_state(ctx, 0);
        do_text(ctx, printf(ctx, "ticks: %d", tick_count));
        timer t(ctx);
        if (t.triggered())
        {
            set(tick_count, get(tick_count) + 1);
            t.start(1000);
        }
        alia_if (t.is_active())
        {
            row_layout row(ctx);
            do_text(ctx, text("active"));
            if (do_link(ctx, text("stop")))
            {
                t.stop();
            }
        }
        alia_else
        {
            if (do_link(ctx, text("start")))
            {
                t.start(1000);
            }
        }
        alia_end
    })

static demo_interface* timer_demos[] =
  { &one_shot_demo, &periodic_timer_demo, 0 };

static demo_section timers_section =
    { "Timers", "Timers offer a more event-based approach to handling time.", timer_demos };

static demo_section* section_list[] =
    { &signals_section, &smoothing_section, &delay_section, &timers_section, 0 };

demo_page timing_page =
    { "Timing", section_list };

}
