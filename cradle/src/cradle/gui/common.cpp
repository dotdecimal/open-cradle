#include <cradle/gui/common.hpp>

#include <alia/ui/api.hpp>

#include <cradle/gui/collections.hpp>
#include <cradle/gui/internals.hpp>

namespace cradle {

struct error_notification : notification_controller
{
    error_notification(string const& message)
      : message_(message) {}
    void do_ui(gui_context& ctx)
    {
        do_heading(ctx, text("heading"), text("Error"));
        do_paragraph(ctx, make_custom_getter(&message_, get_id(id_)));
        if (do_link(ctx, text("copy")))
        {
            ctx.system->os->set_clipboard_text(message_);
            end_pass(ctx);
        }
    }
    char const* overlay_style() const
    { return "error-notification-overlay"; }
 private:
    string message_;
    local_identity id_;
};

void record_failure(gui_context& ctx, string const& message)
{
    post_notification(ctx, new error_notification(message));
}

void scoped_gui_context::begin(alia::ui_context& alia_ctx, gui_system& system)
{
    static_cast<ui_context&>(ctx_) = alia_ctx;
    ctx_.gui_system = &system;

    if (is_refresh_pass(ctx_))
        gather_updates(ctx_.gui_system->requests);
}
void scoped_gui_context::end()
{
    if (is_refresh_pass(ctx_))
    {
        auto req_list = issue_new_requests(ctx_.gui_system->requests);
        for(auto const& request : req_list)
        {
            ctx_.gui_system->request_list.push_back(request);
        }
        // TODO: Figure out better data structure for request_list
        if(ctx_.gui_system->request_list.size() > 500)
        {
            ctx_.gui_system->request_list.erase(ctx_.gui_system->request_list.begin(),
                                                    (ctx_.gui_system->request_list.begin() +
                                                        ctx_.gui_system->request_list.size() -500));
        }
        // Only clear out unused updates if we got all the way through the pass.
        // (If we only made it partially through, the may be UI gui_request
        // calls that didn't get a chance to claim their updates.)
        if (!ctx_.pass_aborted)
            clear_updates(ctx_.gui_system->requests);
    }
}

bool detect_transition_into_here(ui_context& ctx)
{
    int* dummy;
    return get_cached_data(ctx, &dummy);
}

void do_empty_display_panel(gui_context& ctx, layout const& layout_spec)
{
    panel p(ctx, text("empty-display"), layout_spec);
}

void do_text(
    gui_context& ctx, accessor<styled_text_fragment> const& st,
    layout const& layout_spec)
{
    auto style = _field(ref(&st), style);
    auto text = _field(ref(&st), text);
    alia_if (is_gettable(style) && get(style))
    {
        do_styled_text(ctx, unwrap_optional(style), text, layout_spec);
    }
    alia_else_if (is_gettable(style) && !get(style))
    {
        do_text(ctx, text, layout_spec);
    }
    alia_end
}

void do_flow_text(
    gui_context& ctx, accessor<styled_text> const& paragraph,
    layout const& layout_spec)
{
    flow_layout flow(ctx, layout_spec);
    do_text(ctx, paragraph);
}

void do_text(
    gui_context& ctx, accessor<styled_text> const& paragraph)
{
    for_each(ctx,
        [&](gui_context& ctx, size_t index,
            accessor<styled_text_fragment> const& text)
        {
            do_text(ctx, text);
        },
        paragraph);
}

// MARKUP

void static
do_markup_block(gui_context& ctx, accessor<markup_block> const& block,
    layout const& layout_spec = default_layout);

void static
do_markup_column(gui_context& ctx,
    accessor<std::vector<markup_block> > const& blocks,
    layout const& layout_spec)
{
    column_layout column(ctx, layout_spec);
    for_each(ctx,
        [ ](gui_context& ctx, size_t index,
            accessor<markup_block> const& block)
        {
            do_markup_block(ctx, block);
        },
        blocks);
}

void static
do_markup_list(gui_context& ctx,
    accessor<std::vector<markup_block> > const& list,
    layout const& layout_spec)
{
    column_layout column(ctx, layout_spec);
    for_each(ctx,
        [ ](gui_context& ctx, size_t index,
            accessor<markup_block> const& block)
        {
            row_layout row(ctx);
            do_bullet(ctx);
            do_markup_block(ctx, block, GROW);
        },
        list);
}

void static
do_markup_form(gui_context& ctx,
    accessor<markup_form> const& form,
    layout const& layout_spec)
{
    alia::form form_layout(ctx, layout_spec);
    for_each(ctx,
        [&](gui_context& ctx, size_t index,
            accessor<markup_form_row> const& row)
        {
            form_field field(form_layout, _field(row, label));
            do_markup_block(ctx, _field(row, value));
        },
        form);
}

void static
do_markup_block(gui_context& ctx, accessor<markup_block> const& block,
    layout const& layout_spec)
{
    _switch_accessor (_field(block, type))
    {
     alia_case (markup_block_type::EMPTY):
        do_spacer(ctx, layout_spec);
        break;
     alia_case (markup_block_type::TEXT):
        do_flow_text(ctx, _union_member(block, text), layout_spec);
        break;
     alia_case (markup_block_type::COLUMN):
        do_markup_column(ctx, _union_member(block, column), layout_spec);
        break;
     alia_case (markup_block_type::BULLETED_LIST):
        do_markup_list(ctx, _union_member(block, bulleted_list), layout_spec);
        break;
     alia_case (markup_block_type::FORM):
        do_markup_form(ctx, _union_member(block, form), layout_spec);
        break;
    }
    _end_switch
}

void
do_markup_document(gui_context& ctx, accessor<markup_document> const& doc,
    layout const& layout_spec)
{
    do_markup_block(ctx, _field(doc, content), layout_spec);
}

indirect_accessor<framework_context>
get_framework_context(gui_context& ctx)
{
    return make_indirect(ctx,
        unwrap_optional(make_accessor(ctx.gui_system->framework_context)));
}

void
periodically_refresh_data(gui_context& ctx, int milliseconds_timer)
{
    auto& bg = get_background_system(ctx);
    // Periodically refresh the data to ensure uploaded patients are shown.
    timer t(ctx);
    if (!t.is_active())
    {
        t.start(milliseconds_timer);
    }
    if (t.triggered())
    {
        // ISSUE: AST-1262
        clear_mutable_data_cache(bg);
    }
    // Ensure the patient list is up-to-date when transitioning into this block
    if (detect_transition_into_here(ctx))
    {
        // ISSUE: AST-1262
        clear_mutable_data_cache(bg);
    }
}

}
