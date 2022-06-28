#include <visualization/ui/rendering/grid_rendering.hpp>

#include <alia/ui/utilities/rendering.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>

namespace visualization {

std::vector<vector2f> static
vertices_for_box_projections(
    std::vector<box3d> const& boxes, unsigned axis, double position,
    double min_spacing)
{
    std::vector<vector2f> vertices;
    // There's 8 vertices per box, but most boxes won't be in this plane, so
    // this is just a rough estimate.
    vertices.reserve(boxes.size() / 2);
    for (auto const& box3 : boxes)
    {
        if (box3.corner[axis] < position &&
            position <= (box3.corner[axis] + box3.size[axis]))
        {
            auto box2 =
                box2f(add_margin_to_box(slice(box3, axis),
                    min_spacing * make_vector(-0.0, -0.0)));

            vertices.push_back(box2.corner);
            vertices.push_back(
                make_vector(box2.corner[0] + box2.size[0], box2.corner[1]));

            vertices.push_back(
                make_vector(box2.corner[0] + box2.size[0], box2.corner[1]));
            vertices.push_back(box2.corner + box2.size);

            vertices.push_back(box2.corner + box2.size);
            vertices.push_back(
                make_vector(box2.corner[0], box2.corner[1] + box2.size[1]));

            vertices.push_back(
                make_vector(box2.corner[0], box2.corner[1] + box2.size[1]));
            vertices.push_back(box2.corner);
        }
    }
    return vertices;
}

std::vector<rgba8> static
colors_for_box_projections(
    std::vector<box3d> const& boxes, unsigned axis, double position,
    double min_spacing)
{
    std::vector<rgba8> colors;
    // There's 8 vertices per box, but most boxes won't be in this plane, so
    // this is just a rough estimate.
    colors.reserve(boxes.size() / 2);
    for (auto const& box3 : boxes)
    {
        if (box3.corner[axis] < position &&
            position <= (box3.corner[axis] + box3.size[axis]))
        {
            auto relative_size = box3.size[0] / min_spacing;
            auto color =
                apply_alpha(rgb8(204, 102, 0), uint8_t(215));
            for (int i = 0; i != 8; ++i)
            {
                colors.push_back(color);
            }
        }
    }
    return colors;
}

void static
draw_grid_boxes(
    gui_context& ctx,
    sliced_3d_canvas& c3d,
    accessor<std::vector<box3d> > const& grid_boxes,
    accessor<double> const& min_spacing)
{
    auto box_vertices =
        gui_apply(ctx,
            vertices_for_box_projections,
            grid_boxes,
            in(c3d.slice_axis()),
            in(get_slice_position(c3d)),
            min_spacing);

    auto box_colors =
        gui_apply(ctx,
            colors_for_box_projections,
            grid_boxes,
            in(c3d.slice_axis()),
            in(get_slice_position(c3d)),
            min_spacing);

    alia_untracked_if (is_render_pass(ctx) && is_gettable(box_vertices) &&
        is_gettable(box_colors))
    {
        auto const& vertices = get(box_vertices);
        auto const& colors = get(box_colors);
        assert(vertices.size() == colors.size());
        if (!vertices.empty())
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, &vertices[0]);
            glColorPointer(4, GL_UNSIGNED_BYTE, 0, &colors[0]);
            glLineWidth(1.0f);
            glDrawArrays(GL_LINES, 0, GLsizei(vertices.size()));
            glDisableClientState(GL_COLOR_ARRAY);
            glDisableClientState(GL_VERTEX_ARRAY);
        }
    }
    alia_end
}

struct grid_boxes_scene_object : spatial_3d_scene_graph_sliced_object
{
    keyed_data<request<std::vector<box3d> > > grid_boxes;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto grid_boxes =
            gui_request(ctx, make_accessor(&this->grid_boxes));
        auto min_spacing =
            gui_apply(ctx,
                [ ](std::vector<box3d> const& boxes)
                {
                    double min_spacing = std::numeric_limits<double>::max();
                    for (auto const& box : boxes)
                    {
                        for (unsigned i = 0; i != 3; ++i)
                        {
                            if (box.size[i] < min_spacing)
                                min_spacing = box.size[i];
                        }
                    }
                    return min_spacing;
                },
                grid_boxes);
        draw_grid_boxes(ctx, c3d, grid_boxes, min_spacing);
    }
};

void
add_sliced_grid_boxes(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    accessor<request<std::vector<box3d> > > const& grid_boxes,
    canvas_layer layer)
{
    grid_boxes_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        refresh_accessor_clone(object->grid_boxes, grid_boxes);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

}
