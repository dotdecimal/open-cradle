#ifndef ANALYSIS_UI_CONTROLS_COMMON_CONTROLS_HPP
#define ANALYSIS_UI_CONTROLS_COMMON_CONTROLS_HPP

#include <analysis/ui/common.hpp>
#include <boost/function.hpp>

namespace analysis {

struct app_context;
struct validation_state;

// This functions does the button row for a non-singleton task.
// It produces a row with two buttons: an OK button and a cancel button.
// (The actual label of the OK button is configurable.)
//
// When the OK button is pressed, ok_button_handler is called.
// It's passed a dataless_ui_context (to emphasize that it shouldn't be
// getting data from the context) and a pointer to a string.
// If the handler succeeds, it should return true.
// If it fails, it should return false and write an error message to *message.
//
void do_standard_button_row(
    gui_context& ctx, app_context& app_ctx,
    string const& task_id, accessor<validation_state> const& state,
    accessor<string> const& ok_button_label,
    boost::function<bool (dataless_ui_context& ctx, string* message)> const&
        ok_button_handler);

void do_done_button_row(
    gui_context& ctx, app_context& app_ctx,
    string const& task_id);

}

#endif
