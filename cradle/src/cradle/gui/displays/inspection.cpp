#include <cradle/gui/displays/inspection.hpp>
#include <alia/ui/utilities.hpp>

namespace cradle {

void position_overlay(dataless_ui_context& ctx, popup_positioning& positioning,
    layout_box const& bounding_region)
{
    layout_vector lower = bounding_region.corner;
    layout_vector upper = get_high_corner(bounding_region);
    positioning.lower_bound = make_vector(upper[0], upper[1]);
    positioning.upper_bound = make_vector(lower[0], lower[1]);
    positioning.absolute_lower = layout_vector(
        transform(get_transformation(ctx),
            vector<2,double>(positioning.lower_bound) +
            make_vector<double>(0.5, 0.5)));
    positioning.absolute_upper = layout_vector(
        transform(get_transformation(ctx),
            vector<2,double>(positioning.upper_bound) +
            make_vector<double>(0.5, 0.5)));
    positioning.minimum_size = make_layout_vector(-1, -1);
}

layout_vector static
get_default_position(
    popup_positioning const& positioning,
    bool alignment_possible[2][2],
    floating_layout const& layout)
{
    vector<2,int> position;
    for (unsigned i = 0; i != 2; ++i)
    {
        if (alignment_possible[i][0])
            position[i] = positioning.upper_bound[i] - layout.size()[i];
        else
            position[i] = positioning.lower_bound[i];
    }
    return position;
}

void nonmodal_popup::begin(ui_context& ctx, widget_id id,
    popup_positioning const& positioning,
    nonmodal_popup_flag_set flags)
{
    ctx_ = &ctx;
    id_ = id;

    layout_vector surface_size = layout_vector(ctx.system->surface_size);
    layout_vector maximum_size;
    for (unsigned i = 0; i != 2; ++i)
    {
        maximum_size[i] = (std::max)(
            positioning.absolute_upper[i],
            surface_size[i] - positioning.absolute_lower[i]);
    }

    layout_.begin(ctx, positioning.minimum_size, maximum_size);

    if (!is_refresh_pass(ctx))
    {
        layout_vector position;
        // Determine which alignments are possible for each layout axis.
        bool alignment_possible[2][2];
        for (unsigned i = 0; i != 2; ++i)
        {
            alignment_possible[i][0] =
                positioning.absolute_upper[i] >= layout_.size()[i];
            alignment_possible[i][1] =
                surface_size[i] - positioning.absolute_lower[i] >=
                    layout_.size()[i];
        }
        // TODO: All this assumes at least one of the alignments is possible
        // for each axis.
        if (flags & NONMODAL_POPUP_DESCENDING_GRAPH_PLACEMENT)
        {
            if (alignment_possible[0][1] && alignment_possible[1][0])
            {
                position[0] = positioning.lower_bound[0];
                position[1] = positioning.upper_bound[1] - layout_.size()[1];
            }
            else if (alignment_possible[0][0] && alignment_possible[1][1])
            {
                position[0] = positioning.upper_bound[0] - layout_.size()[0];
                position[1] = positioning.lower_bound[1];
            }
            else
            {
                position =
                    get_default_position(positioning, alignment_possible,
                        layout_);
            }
        }
        else
        {
            position =
                get_default_position(positioning, alignment_possible, layout_);
        }
        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(vector<2,double>(position)));
    }

    overlay_.begin(ctx, id);
}
void nonmodal_popup::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;

        overlay_.end();
        transform_.end();
        layout_.end();

        ctx_ = 0;
    }
}

}
