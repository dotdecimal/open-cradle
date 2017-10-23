#include "utilities.hpp"

static void
do_demo_block(ui_context& ctx, int i)
{
    scoped_substyle style(ctx, text("layout-demo-label"));
    layered_layout layer(ctx, layout(size(3, 3, EM), PADDED | TOP | LEFT));
    do_color(ctx, in(rgb8(i * 8, 0x40, 0x80)), UNPADDED | FILL);
    do_text(ctx, printf(ctx, "%d", in(i)), CENTER);
}

static void
do_demo_block(ui_context& ctx, int i, layout const& layout_spec)
{
    scoped_substyle style(ctx, text("layout-demo-label"));
    layered_layout layer(ctx, 
        add_default_alignment(add_default_padding(layout_spec, PADDED),
            LEFT, TOP));
    do_color(ctx, in(rgb8(i * 8, 0x40, 0x80)), UNPADDED | FILL);
    do_text(ctx, printf(ctx, "%d", in(i)), CENTER);
}

ALIA_DEFINE_DEMO(
    row_demo, "Row Layout",
    "row_layout arranges its children in a horizontal row.",
    void do_ui(ui_context& ctx)
    {
        row_layout row(ctx);
        do_demo_block(ctx, 0);
        do_demo_block(ctx, 1);
        do_demo_block(ctx, 2);
    })

ALIA_DEFINE_DEMO(
    column_demo, "Column Layout",
    "column_layout arranges its children in a vertical column.",
    void do_ui(ui_context& ctx)
    {
        column_layout column(ctx);
        do_demo_block(ctx, 0);
        do_demo_block(ctx, 1);
        do_demo_block(ctx, 2);
    })

ALIA_DEFINE_DEMO(
    linear_layout_demo, "Linear Layout",
    "linear_layout allows you to dynamically switch between horizontal and vertical layout.",
    void do_ui(ui_context& ctx)
    {
        state_accessor<bool> vertical = get_state(ctx, false);
        if (do_link(ctx, text("switch")))
        {
            set(vertical, !get(vertical));
        }
        {
            linear_layout line(ctx,
                get(vertical) ? VERTICAL_LAYOUT : HORIZONTAL_LAYOUT);
            do_demo_block(ctx, 0);
            do_demo_block(ctx, 1);
            do_demo_block(ctx, 2);
        }
    })

static demo_interface* linear_demos[] =
    { &row_demo, &column_demo, &linear_layout_demo, 0 };

static demo_section linear_section =
    { "Rows and Columns", "", linear_demos };

ALIA_DEFINE_DEMO(
    grid_layout_demo, "Grid Layout",
    "A grid_layout is used to arrange widgets in a grid. To use it, create grid_row containers that reference the grid_layout.\n\n"
    "Note that the grid_layout container by itself is just a normal column, so you can intersperse other widgets amongst the grid rows.",
    void do_ui(ui_context& ctx)
    {
        grid_layout grid(ctx);
        {
            grid_row row(grid);
            for (int i = 0; i != 8; ++i)
            {
                do_demo_block(ctx, i, size(2 + i / 4.f, 3, EM));
            }
        }
        {
            grid_row row(grid);
            for (int i = 0; i != 8; ++i)
            {
                do_demo_block(ctx, 8 + i, size(4 - i / 4.f, 3, EM));
            }
        }
    })

ALIA_DEFINE_DEMO(
    uniform_grid_layout_demo, "Uniform Grid Layout",
    "A uniform_grid_layout is the similar to a grid_layout, but it forces all grid cells to be the same size.",
    void do_ui(ui_context& ctx)
    {
        uniform_grid_layout grid(ctx);
        {
            uniform_grid_row row(grid);
            for (int i = 0; i != 8; ++i)
            {
                do_demo_block(ctx, i, size(2 + i / 4.f, 3, EM));
            }
        }
        {
            uniform_grid_row row(grid);
            for (int i = 0; i != 8; ++i)
            {
                do_demo_block(ctx, 8 + i, size(4 - i / 4.f, 3, EM));
            }
        }
    })

static demo_interface* grid_demos[] =
    { &grid_layout_demo, &uniform_grid_layout_demo, 0 };

static demo_section grid_section =
    { "Grids", "", grid_demos };

ALIA_DEFINE_DEMO(
    flow_layout_demo, "Flow Layout",
    "flow_layout lets its children wrap from one line to the next.",
    void do_ui(ui_context& ctx)
    {
        flow_layout flow(ctx);
        for (int i = 0; i != 32; ++i)
        {
            do_demo_block(ctx, i);
        }
    })

ALIA_DEFINE_DEMO(
    vertical_flow_layout_demo, "Vertical Flow",
    "A vertical_flow layout_arranges its children in columns. Widgets flow down the columns, starting with the left column.",
    void do_ui(ui_context& ctx)
    {
        vertical_flow_layout flow(ctx);
        for (int i = 0; i != 32; ++i)
        {
            do_demo_block(ctx, i);
        }
    })

static demo_interface* flow_demos[] =
    { &flow_layout_demo, &vertical_flow_layout_demo, 0 };

static demo_section flow_section =
    { "Flows", "", flow_demos };

ALIA_DEFINE_DEMO(
    layered_layout_demo, "Layered Layout",
    "layered_layout layers its children on top of one another.",
    void do_ui(ui_context& ctx)
    {
        layered_layout rotated(ctx);
        do_color(ctx, in(silver), size(6, 6, EM));
        do_demo_block(ctx, 0);
    })

ALIA_DEFINE_DEMO(
    rotated_layout_demo, "Rotated Layout",
    "rotated_layout rotates its child 90 degress counterclockwise.",
    void do_ui(ui_context& ctx)
    {
        rotated_layout rotated(ctx);
        {
            row_layout row(ctx);
            do_demo_block(ctx, 0);
            do_demo_block(ctx, 1);
            do_demo_block(ctx, 2);
        }
    })

static demo_interface* special_demos[] =
    { &layered_layout_demo, &rotated_layout_demo, 0 };

static demo_section special_section =
    { "Special Layouts", "", special_demos };

static demo_section* section_list[] =
    { &linear_section, &grid_section, &flow_section, &special_section, 0 };

demo_page layout_page =
    { "Layout", section_list};
