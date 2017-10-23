#ifndef VISUALIZATION_UI_VIEWS_STATISTICAL_DOSE_VIEWS_HPP
#define VISUALIZATION_UI_VIEWS_STATISTICAL_DOSE_VIEWS_HPP

#include <cradle/gui/displays/canvas.hpp>
#include <cradle/gui/displays/display.hpp>

#include <visualization/ui/common.hpp>
#include <visualization/data/types/image_types.hpp>

namespace visualization {

struct statistical_dose_view_contents;

// Generate up-to-date an statistical_dose_view_contents object for the given
// content. Note that :dose must remain valid as long as the contents is in
// use.
statistical_dose_view_contents*
generate_statistical_dose_view_contents(
    gui_context& ctx,
    image_interface_3d const* dose,
    accessor<std::vector<gui_structure> > const& structures);

struct dvh_view : display_view_interface<null_display_context>
{
    dvh_view() {}

    void
    initialize(
        statistical_dose_view_contents* contents,
        keyed_data<dvh_view_state>* state_);

    // Implementation of the display_view_interface interface...

    string const& get_type_id() const;

    string const&
    get_type_label(null_display_context const& display_ctx);

    indirect_accessor<string>
    get_view_label(gui_context& ctx, null_display_context const& display_ctx,
        string const& instance_id);

    void
    do_view_content(gui_context& ctx, null_display_context const& display_ctx,
        string const& instance_id, bool is_preview);

 private:
    statistical_dose_view_contents* contents_;
    keyed_data<dvh_view_state>* state_;
};

// Initialize a DVH view and add it to the given provider.
// :view must be allocated in the stack frame of the provider, as usual.
// TODO: This should eventually be a utility that adds the DVH lines to a
// 2D graphing scene as scene objects. That would allow it to integrate with
// other content (like DVHs for other doses).
void
add_dvh_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    dvh_view* view,
    statistical_dose_view_contents* contents,
    accessor<dvh_view_state> const& state);

struct dose_stats_view : display_view_interface<null_display_context>
{
    dose_stats_view() {}

    void initialize(statistical_dose_view_contents* contents);

    // Implementation of the display_view_interface interface...

    string const& get_type_id() const;

    string const&
    get_type_label(null_display_context const& display_ctx);

    indirect_accessor<string>
    get_view_label(gui_context& ctx, null_display_context const& display_ctx,
        string const& instance_id);

    void
    do_view_content(gui_context& ctx, null_display_context const& display_ctx,
        string const& instance_id, bool is_preview);

 private:
    statistical_dose_view_contents* contents_;
};

// Initialize a dose stats view and add it to the given provider.
// :view must be allocated in the stack frame of the provider, as usual.
void
add_dose_stats_view(
    gui_context& ctx,
    display_view_provider<null_display_context>& provider,
    dose_stats_view* view,
    statistical_dose_view_contents* contents);

}

#endif
