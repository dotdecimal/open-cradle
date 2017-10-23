#include <cradle/gui/task_controls.hpp>

#include <alia/ui/utilities.hpp>

namespace cradle {

// CONTROL BLOCKS

void
do_control_block(
    gui_context& ctx,
    accessor<bool> const& active,
    accessor<bool> const& disabled,
    accessor<styled_text> const& label,
    control_block_interface& block)
{
    // TODO: Use external state for this.
    auto menu_expanded = get_state(ctx, false);

    scoped_substyle style(ctx, text("control-block"));
    {
        transitioning_container container(ctx);
        {
            transitioning_container_content
                disabled_block(ctx, container, is_true(disabled));
            auto id = get_widget_id(ctx);
            alia_if (disabled_block.do_content())
            {
                {
                    panel header(ctx, text("disabled-header"),
                        UNPADDED, PANEL_HORIZONTAL, id);
                    do_text(ctx, label);
                    // Do a spacer here with a very small growth proportion
                    // so that it'll take up extra space if the summary
                    // doesn't want it but won't really interfere if it does.
                    do_spacer(ctx, layout(FILL, 0.0001f));
                }
                {
                    panel content(ctx, text("disabled-content"),
                        UNPADDED, NO_FLAGS, id);
                    block.do_disabled_content(ctx);

                }
            }
            alia_end
        }
        {
            transitioning_container_content
                inactive_block(ctx, container, !is_true(active) && is_false(disabled));
            auto id = get_widget_id(ctx);
            alia_if (inactive_block.do_content())
            {
                {
                    clickable_panel header(ctx, text("inactive-header"),
                        UNPADDED, PANEL_HORIZONTAL, id);
                    if (header.clicked())
                    {
                        set(active, true);
                        end_pass(ctx);
                    }
                    {
                        do_styled_text(ctx, text("arrow-font"), text("\342\226\272"), CENTER_Y);
                    }
                    do_text(ctx, label);
                    // Do a spacer here with a very small growth proportion
                    // so that it'll take up extra space if the summary
                    // doesn't want it but won't really interfere if it does.
                    do_spacer(ctx, layout(FILL, 0.0001f));
                    {
                        scoped_substyle style(ctx, text("summary"));
                        block.do_inactive_summary(ctx);
                    }
                }
                {
                    clickable_panel content(ctx, text("inactive-content"),
                        UNPADDED, NO_FLAGS, id);
                    block.do_inactive_content(ctx);
                }
            }
            alia_end
        }
        {
            transitioning_container_content
                section(ctx, container, is_true(active) && is_false(disabled));
            auto id = get_widget_id(ctx);
            alia_if (section.do_content())
            {
                {
                    clickable_panel header(ctx, text("active-header"), UNPADDED, NO_FLAGS, id);
                    {
                        row_layout row(ctx);
                        {
                            do_styled_text(ctx, text("arrow-font"), text("\342\226\274"), CENTER_Y);
                        }
                        do_text(ctx, label);
                        alia_if (block.has_menu())
                        {
                            do_spacer(ctx, GROW);
                            if (do_icon_button(ctx, MENU_ICON))
                            {
                                set(menu_expanded, is_false(menu_expanded));
                                end_pass(ctx);
                            }
                        }
                        alia_end
                        // Do a spacer here with a very small growth proportion
                        // so that it'll take up extra space if the summary
                        // doesn't want it but won't really interfere if it does.
                        do_spacer(ctx, layout(FILL, 0.0001f));
                        {
                            scoped_substyle style(ctx, text("summary"));
                            block.do_active_summary(ctx);
                        }
                    }
                    if(header.clicked())
                    {
                        set(active, false);
                        end_pass(ctx);
                    }
                }
                {
                    collapsible_content collapsible(ctx, is_true(menu_expanded));
                    alia_if (collapsible.do_content())
                    {
                        panel menu(ctx, text("active-menu"), UNPADDED);
                        block.do_menu(ctx);
                    }
                    alia_end
                }
                {
                    panel content(ctx, text("active-content"), UNPADDED,
                        PANEL_NO_INTERNAL_PADDING);
                    block.do_active_ui(ctx);
                }
            }
            alia_end
        }
    }

    do_separator(ctx, UNPADDED);
}

// ITEM LISTS

bool static
is_list_item_expanded(
    item_list_view_mode view_mode,
    bool is_active,
    list_item_mode item_status)
{
    return view_mode == item_list_view_mode::DETAILED ||
        is_active ||
        item_status != list_item_mode::NORMAL;
}

void static
do_list_item_options(
    gui_context& ctx, list_item_interface& item,
    accessor<list_item_mode> const& item_status)
{
    auto options = item.get_available_options();
    alia_if (options != list_item_options_flag_set(NO_FLAGS))
    {
        panel panel(ctx, text("options"), UNPADDED, PANEL_HORIZONTAL);
        do_spacer(ctx, GROW);
        alia_if (options & LIST_ITEM_COPY)
        {
            if (do_button(ctx, text("Copy")))
            {
                item.on_copy();
                end_pass(ctx);
            }
        }
        alia_end
        alia_if (options & LIST_ITEM_CLONE)
        {
            if (do_button(ctx, text("Clone")))
            {
                item.on_clone();
                end_pass(ctx);
            }
        }
        alia_end
        alia_if (options & LIST_ITEM_EDIT)
        {
            if (do_button(ctx, text("Edit")))
            {
                set(item_status, list_item_mode::EDITING);
                item.on_edit();
                end_pass(ctx);
            }
        }
        alia_end
        alia_if (options & LIST_ITEM_DELETE)
        {
            if (do_button(ctx, text("Delete")))
            {
                set(item_status, list_item_mode::DELETING);
                end_pass(ctx);
            }
        }
        alia_end
        alia_if (options & LIST_ITEM_EDIT_DISABLED)
        {
            do_button(ctx, text("Edit"), default_layout, BUTTON_DISABLED);
        }
        alia_end
    }
    alia_end
}

void do_list_item_ui(
    gui_context& ctx, list_item_interface& item,
    accessor<item_list_view_mode> const& view_mode,
    accessor<bool> const& is_active,
    accessor<list_item_mode> const& item_mode)
{
    panel padded_panel(ctx, text("item-panel"),
        UNPADDED, PANEL_NO_INTERNAL_PADDING);

    panel item_panel(ctx,
        text("active-item"),
        UNPADDED, PANEL_NO_INTERNAL_PADDING);

    {
        // Only allow the collapsable panel if the item mode is not editing
        alia_if (!is_equal(item_mode, list_item_mode::EDITING))
        {
            // updated so panel can be closed when header is clicked
            clickable_panel header(ctx, text("header"), GROW | UNPADDED,
                PANEL_HORIZONTAL);
            if (header.clicked())
            {
                if (is_gettable(is_active))
                {
                    auto was_active = get(is_active);
                    set(is_active, !was_active);
                    if (!was_active)
                        item.on_activate();
                    else
                        item.on_deactivate();
                }
                end_pass(ctx);
            }
            item.do_header_ui(ctx);
        }
        alia_else
        {
            panel header(ctx, text("header"), GROW | UNPADDED,
                PANEL_HORIZONTAL);
            item.do_header_ui(ctx);
        }
        alia_end
    }

    {
        collapsible_content
            content_section(ctx,
                is_true(
                    gui_apply(ctx,
                        is_list_item_expanded,
                        view_mode,
                        is_active,
                        item_mode)));

        // This is an unfortunate little hack to get the transitions to work
        // right. Because alia requires UIs that are transitioning out to be
        // reproduced during the transition, we have to track the state that
        // they were in. This generally isn't a problem, but here there are
        // multiple state variables interacting, so we need to remember how we
        // got to certain states.
        auto show_compact_options = get_state(ctx, false);
        alia_untracked_if (is_refresh_pass(ctx))
        {
            if (is_equal(view_mode, item_list_view_mode::COMPACT) &&
                is_true(is_active))
            {
                set(show_compact_options, true);
            }
            else if (
                is_equal(view_mode, item_list_view_mode::DETAILED) &&
                is_false(is_active))
            {
                set(show_compact_options, false);
            }
        }
        alia_end

        alia_if (content_section.do_content())
        {
            transitioning_container container(ctx);
            {
                transitioning_container_content
                    normal_ui(ctx, container,
                        is_equal(item_mode, list_item_mode::NORMAL));
                alia_if (normal_ui.do_content())
                {
                    alia_if (is_equal(view_mode, item_list_view_mode::DETAILED))
                    {
                        {
                            panel panel(ctx, text("info"), UNPADDED);
                            item.do_info_ui(ctx);
                        }
                        {
                            collapsible_content
                                options_section(ctx, is_true(is_active));
                            alia_if (options_section.do_content())
                            {
                                do_list_item_options(ctx, item, item_mode);
                            }
                            alia_end
                        }
                    }
                    alia_else
                    {
                        {
                            panel panel(ctx, text("info"), UNPADDED);
                            item.do_info_ui(ctx);
                        }
                        alia_if (is_true(show_compact_options))
                        {
                            do_list_item_options(ctx, item, item_mode);
                        }
                        alia_end
                    }
                    alia_end
                }
                alia_end
            }
            {
                transitioning_container_content
                    editing_ui(ctx, container,
                        is_equal(item_mode, list_item_mode::EDITING));
                alia_if (editing_ui.do_content())
                {
                    panel panel(ctx, text("editing"));
                    item.do_editing_ui(ctx);
                }
                alia_end
            }
            {
                transitioning_container_content
                    deleting_ui(ctx, container,
                        is_equal(item_mode, list_item_mode::DELETING));
                alia_if (deleting_ui.do_content())
                {
                    panel panel(ctx, text("deleting"));
                    item.do_deletion_ui(ctx);
                }
                alia_end
            }
        }
        alia_end
    }
}

void do_item_list_group_heading(
    gui_context& ctx, accessor<string> const& label)
{
    panel panel(ctx, text("group-heading"));
    do_text(ctx, label);
}

void static
do_view_mode_option(
    gui_context& ctx,
    drop_down_menu_context& menu_ctx,
    accessor<item_list_view_mode> const& active_mode,
    accessor<string> const& this_label,
    accessor<item_list_view_mode> const& this_mode)
{
    alia_if (active_mode != this_mode)
    {
        do_menu_option(menu_ctx, this_label, make_setter(active_mode, this_mode));
    }
    alia_else
    {
        panel
            item_panel(ctx,
                text("item"),
                UNPADDED,
                PANEL_NO_INTERNAL_PADDING | PANEL_NO_CLICK_DETECTION,
                auto_id,
                WIDGET_SELECTED);
        do_text(ctx, this_label);
    }
    alia_end
}

void do_item_list_ui(
    gui_context& ctx,
    accessor<string> const& label,
    accessor<item_list_view_mode> const& view_mode,
    boost::function<void()> const& do_items,
    boost::function<void(drop_down_menu_context& menu_ctx)> const& do_custom_options)
{
    scoped_substyle style(ctx, text("item-list"));

    // header
    {
        row_layout row(ctx);

        do_drop_down_menu(ctx, width(1, EM),
            [&](drop_down_menu_context& menu_ctx)
            {
                do_view_mode_option(ctx, menu_ctx, view_mode,
                    text("Collapsed"), in(item_list_view_mode::COLLAPSED));
                do_view_mode_option(ctx, menu_ctx, view_mode,
                    text("Compact"), in(item_list_view_mode::COMPACT));
                do_view_mode_option(ctx, menu_ctx, view_mode,
                    text("Detailed"), in(item_list_view_mode::DETAILED));
                do_custom_options(menu_ctx);
            });

        do_heading(ctx, text("header"), label);

    }

    // items
    {
        collapsible_content collapsible(ctx,
            !is_equal(view_mode, item_list_view_mode::COLLAPSED));

        alia_if (collapsible.do_content())
        {
            do_items();
        }
        alia_end
    }
}

void do_item_list_ui(
    gui_context& ctx,
    accessor<string> const& label,
    accessor<item_list_view_mode> const& view_mode,
    boost::function<void()> const& do_items)
{
    do_item_list_ui(ctx, label, view_mode, do_items, [](drop_down_menu_context&){});
}

}
