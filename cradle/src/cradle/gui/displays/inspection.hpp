#ifndef CRADLE_GUI_DISPLAYS_INSPECTION_HPP
#define CRADLE_GUI_DISPLAYS_INSPECTION_HPP

#include <cradle/gui/common.hpp>

namespace cradle {

void position_overlay(dataless_ui_context& ctx, popup_positioning& positioning,
    layout_box const& bounding_region);

ALIA_DEFINE_FLAG_TYPE(nonmodal_popup)
// When using this flag, the popup will attempt to be placed in a position that
// won't cover up a descending graph (i.e., bottom-left or top-right).
ALIA_DEFINE_FLAG(nonmodal_popup, 0x0001,
    NONMODAL_POPUP_DESCENDING_GRAPH_PLACEMENT)

struct nonmodal_popup
{
    nonmodal_popup() : ctx_(0) {}
    nonmodal_popup(ui_context& ctx, widget_id id,
        popup_positioning const& positioning,
        nonmodal_popup_flag_set flags = NO_FLAGS)
    { begin(ctx, id, positioning, flags); }
    ~nonmodal_popup() { end(); }
    void begin(ui_context& ctx, widget_id id,
        popup_positioning const& positioning,
        nonmodal_popup_flag_set flags = NO_FLAGS);
    void end();
 private:
    ui_context* ctx_;
    widget_id id_, background_id_;
    floating_layout layout_;
    scoped_transformation transform_;
    overlay_event_transformer overlay_;
};

}

#endif
