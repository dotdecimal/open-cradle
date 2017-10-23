#include <analysis/ui/controls/common_controls.hpp>

#include <cradle/gui/task_interface.hpp>

#include <analysis/ui/state/common_state.hpp>
#include <analysis/ui/app_context.hpp>

namespace analysis {

void do_standard_button_row(
    gui_context& ctx, app_context& app_ctx,
    string const& task_id, accessor<validation_state> const& state,
    accessor<string> const& ok_button_label,
    boost::function<bool (dataless_ui_context& ctx, string* message)> const&
        ok_button_handler)
{
    alia_if (is_gettable(state))
    {
        do_spacer(ctx, layout(height(1, EM), GROW));

        alia_if (is_gettable(unwrap_optional(_field(ref(&state), message))))
        {
            do_text(ctx, unwrap_optional(_field(ref(&state), message)));
        }
        alia_end
        auto button_flags = is_settable(state) ? NO_FLAGS : BUTTON_DISABLED;
        auto show_cancel_warning = _field(ref(&state), show_cancel_warning);
        alia_if (get(show_cancel_warning))
        {
            panel p(ctx, text("warning-panel"));
            do_paragraph(ctx, text("Are you sure you want to cancel? Any work you've done in subtasks within this task will be lost."));
            {
                row_layout row(ctx);
                do_spacer(ctx, GROW);
                alia_untracked_if (
                    do_button(ctx, text("Yes"), default_layout, button_flags))
                {
                    cancel_task(app_ctx, task_id);
                    end_pass(ctx);
                }
                alia_end
                alia_untracked_if (
                    do_button(ctx, text("No"), default_layout, button_flags))
                {
                    set(show_cancel_warning, false);
                    end_pass(ctx);
                }
                alia_end
            }
        }
        alia_else
        {
            row_layout row(ctx);
            do_spacer(ctx, GROW);
            alia_untracked_if (
                do_button(ctx, ok_button_label, default_layout, button_flags))
            {
                string message;
                if (!ok_button_handler(ctx, &message))
                    _field(ref(&state), message).set(message);
                end_pass(ctx);
            }
            alia_end
            alia_untracked_if (
                do_button(ctx, text("Cancel"), default_layout, button_flags))
            {
                auto raw_state = get_raw_task_state(app_ctx, task_id);
                // If any work has been done in subtasks, the user
                // would lose that work by canceling this task.
                // Since this is not obvious to most users (and not
                // consistent with how other UIs work), show a warning.
                if (raw_state.completed_subtask_count != 0 ||
                    raw_state.open_subtask_count != 0)
                {
                    set(show_cancel_warning, true);
                }
                else
                    cancel_task(app_ctx, task_id);
                end_pass(ctx);
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

void do_done_button_row(
    gui_context& ctx, app_context& app_ctx,
    string const& task_id)
{
    do_spacer(ctx, GROW);
    {
        row_layout row(ctx);
        do_spacer(ctx, GROW);
        if (do_button(ctx, text("Done")))
        {
            pop_singleton_task(app_ctx, task_id);
            end_pass(ctx);
        }
    }
}

}
