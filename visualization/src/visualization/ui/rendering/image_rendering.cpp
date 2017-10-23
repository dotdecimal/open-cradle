#include <visualization/ui/rendering/image_rendering.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/displays/drawing.hpp>
#include <cradle/gui/displays/types.hpp>
#include <cradle/gui/displays/geometry_utilities.hpp>

namespace visualization {

// GRAYSCALE

struct gray_image_3d_scene_object : spatial_3d_scene_graph_sliced_object
{
    image_interface_3d const* image;
    keyed_data<gray_image_rendering_parameters> rendering;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        draw_gray_image(ctx,
            get_image_slice(ctx, c3d, *this->image),
            make_accessor(&this->rendering));
    }
};

void
add_gray_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<gray_image_rendering_parameters> const& rendering,
    canvas_layer layer)
{
    gray_image_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        refresh_accessor_clone(object->rendering, rendering);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

// COLOR WASH

rgba8 static inline
apply_opacity(rgb8 const& color, double opacity)
{
    return apply_alpha(color, uint8_t(opacity * 0xff + 0.5));
}

color_map static
make_color_wash_map(
    color_wash_rendering_parameters const& color_wash,
    image_level_list const& levels,
    min_max<double> const& value_range)
{
    color_map map;

    if (levels.empty())
    {
        return map;
    }

    auto opacity = color_wash.opacity;

    // If the bottom level has a low color and the dose level is above the
    // image min, add a band to ramp up to that level.
    auto const& bottom = levels.front();
    if (bottom.lower_color && bottom.value > value_range.min)
    {
        color_map_level<rgba8> low;
        low.level = value_range.min;
        low.color = apply_opacity(get(bottom.lower_color), 0.);
        map.push_back(low);
    }
    // If the bottom level does not have a low color and the dose level is above the
    // image min, add a band to ramp up to that level.
    else if (!bottom.lower_color && bottom.value > value_range.min)
    {
        color_map_level<rgba8> low;
        low.level = value_range.min;
        low.color = apply_opacity(bottom.color, 0.);
        map.push_back(low);
    }

    for (auto const& level : levels)
    {
        if (level.lower_color)
        {
            color_map_level<rgba8> low;
            low.level = level.value;
            low.color = apply_opacity(get(level.lower_color), opacity);
            map.push_back(low);
        }
        color_map_level<rgba8> high;
        high.level = level.value;
        high.color = apply_opacity(level.color, opacity);
        map.push_back(high);
    }

    // Removed because the ramp down to transparent color past max dose looks very ambiguous
    // about the presence of dose
    //// If the last level is below the max, add a ramp down.
    //auto const& top = levels.back();
    //if (top.value < value_range.max)
    //{
    //    color_map_level<rgba8> high;
    //    high.level = value_range.max;
    //    high.color = apply_opacity(top.color, 0.);
    //    map.push_back(high);
    //}

    return map;
}

struct image_color_wash_3d_scene_object : spatial_3d_scene_graph_sliced_object
{
    image_interface_3d const* image;
    indirect_accessor<color_map> color_map;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        draw_color_mapped_image(ctx,
            get_image_slice(ctx, c3d, *image),
            color_map);
    }
};

void
add_image_color_wash(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<color_wash_rendering_parameters> const& rendering,
    canvas_layer layer)
{
    auto color_map =
        make_indirect(ctx,
            gui_apply(ctx,
                make_color_wash_map,
                rendering,
                levels,
                unwrap_optional(image->get_value_range(ctx))));

    image_color_wash_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        object->color_map = color_map;
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

// ISOLINES

struct image_isolines_3d_scene_object : spatial_3d_scene_graph_sliced_object
{
    image_interface_3d const* image;
    keyed_data<isoline_rendering_parameters> rendering;
    keyed_data<image_level_list> levels;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto line_style =
            gui_apply(ctx,
                make_line_style,
                _field(make_accessor(&this->rendering), type),
                _field(make_accessor(&this->rendering), width));
        auto& image_slice = get_image_slice(ctx, c3d, *this->image);
        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<image_level> const& level)
            {
                draw_image_isoline(ctx,
                    gui_apply(ctx,
                        apply_opacity,
                        _field(level, color),
                        _field(make_accessor(&this->rendering), opacity)),
                    line_style,
                    image_slice,
                    _field(level, value));
            },
            make_accessor(&this->levels));
    }
};

void
add_image_isolines(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<isoline_rendering_parameters> const& rendering,
    canvas_layer layer)
{
    image_isolines_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        refresh_accessor_clone(object->rendering, rendering);
        refresh_accessor_clone(object->levels, levels);
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

// ISOBANDS

struct raw_image_level
{
    double value;
    rgba8 color;

    raw_image_level() {}
    raw_image_level(double value, rgba8 color) : value(value), color(color) {}
};

struct raw_image_level_pair
{
    raw_image_level low, high;

    raw_image_level_pair() {}
    raw_image_level_pair(raw_image_level const& low, raw_image_level const& high)
      : low(low), high(high)
    {}
};

std::vector<raw_image_level_pair> static
make_isoband_list(
    isoband_rendering_parameters const& isobands,
    image_level_list const& levels,
    min_max<double> const& value_range)
{
    std::vector<raw_image_level_pair> bands;

    if (levels.empty())
    {
        return bands;
    }

    auto opacity = isobands.opacity;

    // If the bottom level has a low color and the image level is above the
    // min, add a band to ramp up to that level.
    {
        auto const& bottom = levels.front();
        if (bottom.lower_color && bottom.value > value_range.min)
        {
            bands.push_back(
                raw_image_level_pair(
                    raw_image_level(
                        value_range.min,
                        apply_opacity(get(bottom.lower_color), 0.)),
                    raw_image_level(
                        bottom.value,
                        apply_opacity(get(bottom.lower_color), opacity))));
        }
    }

    // Do the bands in between the levels.
    {
        auto prev = levels.begin();
        for (auto i = next(prev); i != levels.end(); prev = i++)
        {
            bands.push_back(
                raw_image_level_pair(
                    raw_image_level(
                        prev->value,
                        apply_opacity(prev->color, opacity)),
                    raw_image_level(
                        i->value,
                        apply_opacity(
                            i->lower_color ? get(i->lower_color) : prev->color,
                            opacity))));
        }
    }

    // Do a band between the top level and the max image value.
    {
        auto const& top = levels.back();
        if (top.value < value_range.max)
        {
            bands.push_back(
                raw_image_level_pair(
                    raw_image_level(
                        top.value,
                        apply_opacity(top.color, opacity)),
                    raw_image_level(
                        value_range.max,
                        apply_opacity(top.color, opacity))));
        }
    }

    return bands;
}

struct image_isobands_3d_scene_object : spatial_3d_scene_graph_sliced_object
{
    image_interface_3d const* image;
    indirect_accessor<std::vector<raw_image_level_pair> > isobands;

    void
    render(
        gui_context& ctx,
        sliced_3d_canvas& c3d,
        embedded_canvas& c2d) const
    {
        auto& image_slice = get_image_slice(ctx, c3d, *image);
        for_each(ctx,
            [&](gui_context& ctx, size_t index,
                accessor<raw_image_level_pair> const& band)
            {
                auto low = _field(band, low);
                auto high = _field(band, high);
                draw_shaded_image_isoband(ctx, image_slice,
                    _field(low, color), _field(low, value),
                    _field(high, color), _field(high, value));
            },
            this->isobands);
    }
};

void
add_image_isobands(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_3d const* image,
    accessor<image_level_list> const& levels,
    accessor<isoband_rendering_parameters> const& rendering,
    canvas_layer layer)
{
    auto isobands =
        make_indirect(ctx,
            gui_apply(ctx,
                make_isoband_list,
                rendering,
                levels,
                unwrap_optional(image->get_value_range(ctx))));

    image_isobands_3d_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        object->isobands = isobands;
        add_sliced_scene_object(scene_graph, object, layer);
    }
}

// PROJECTED

struct projected_gray_image_scene_object
  : spatial_3d_scene_graph_projected_3d_object
{
    image_interface_2d const* image;
    keyed_data<gray_image_rendering_parameters> rendering;
    keyed_data<rgba8> color;
    keyed_data<plane<double> > draw_plane;
    keyed_data<vector3d> draw_plane_up;

    void
    render(
        gui_context& ctx,
        projected_canvas& canvas) const
    {
        canvas.draw_image(ctx,
            *this->image,
            make_accessor(&this->rendering),
            make_accessor(&this->color),
            make_accessor(&this->draw_plane),
            make_accessor(&this->draw_plane_up));
    }

    virtual indirect_accessor<double>
    get_z_depth(
        gui_context& ctx,
        projected_canvas& canvas) const
    {
        // We're just using the point that defines the draw plane for now.
        auto z_depth =
            gui_apply(ctx,
                [ ](auto const& camera_view, auto const& point)
                {
                    return transform_point(create_modelview(camera_view), point)[2];
                },
                in(canvas.view()),
                _field(make_accessor(&this->draw_plane), point));
        return make_indirect(ctx, z_depth);
    }

    virtual indirect_accessor<double>
    get_opacity(gui_context& ctx) const
    {
        return
            make_indirect(ctx,
                gui_apply(ctx,
                    [ ](rgba8 const& color)
                    {
                        return double(color.a) / 255.;
                    },
                    make_accessor(&this->color)));
    }
};

void
add_projected_gray_image(
    gui_context& ctx,
    spatial_3d_scene_graph& scene_graph,
    image_interface_2d const* image,
    accessor<gray_image_rendering_parameters> const& rendering,
    accessor<rgba8> const& color,
    accessor<plane<double> > const& draw_plane,
    accessor<vector3d> const& draw_plane_up)
{
    projected_gray_image_scene_object* object;
    get_cached_data(ctx, &object);
    if (is_refresh_pass(ctx))
    {
        object->image = image;
        refresh_accessor_clone(object->rendering, rendering);
        refresh_accessor_clone(object->color, color);
        refresh_accessor_clone(object->draw_plane, draw_plane);
        refresh_accessor_clone(object->draw_plane_up, draw_plane_up);
        add_projected_3d_scene_object(scene_graph, object);
    }
}

}
