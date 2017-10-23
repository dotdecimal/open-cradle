#ifndef CRADLE_GUI_TASK_CONTROLS_HPP
#define CRADLE_GUI_TASK_CONTROLS_HPP

#include <boost/function.hpp>

#include <cradle/gui/common.hpp>

// This file defines some utilities useful for implementing task control UIs.

namespace cradle {

// CONTROLS BLOCKS
//
// Most task control UIs are divided up into blocks. Only a single block is
// active at once. The active block shows actual widgets for interacting with
// the data in that section, and the inactive blocks show a non-interactive
// summary view of their data.
//
// This section defines the interface required to implement a control block
// and the functions for actually instantiating it in the UI.

struct control_block_interface
{
    // When the block is inactive, its header can be used to show summary
    // information. This is optional.
    virtual void do_inactive_summary(gui_context& ctx) {}

    // When the block is active, its header can be used to show summary
    // information. This is optional.
    virtual void do_active_summary(gui_context& ctx) {}

    // Do the actual contents of the block when it's inactive.
    // This is also optional.
    virtual void do_inactive_content(gui_context& ctx) {}

    // Do the actual contents of the block when it's disabled.
    // This is also optional.
    virtual void do_disabled_content(gui_context& ctx) {}

    // Do the UI for the block when it's active.
    virtual void do_active_ui(gui_context& ctx) = 0;

    // Does the block have a pull-down menu?
    // This doesn't take a UI context because it's assumed to be independent
    // of any state associated with the block (i.e, when you write the code,
    // you know the block will either always have one or never have one).
    bool has_menu() { return false; }

    // If the block has a menu, this does the UI for it.
    void do_menu(gui_context& ctx) {}
};

// A control block implementation will often require exactly three members:
//   * the app context
//   * the task ID
//   * the task state
// In this case, you can use the following macro to define those members and
// the corresponding constructor.
#define CRADLE_CONTROL_BLOCK_HEADER(Block, AppContext, State) \
    Block( \
        AppContext& app_ctx, \
        string const& task_id, \
        accessor<State> const& state) \
      : app_ctx(app_ctx), \
        task_id(task_id), \
        state(state) \
    {} \
    AppContext& app_ctx; \
    string const& task_id; \
    accessor<State> const& state;

// Do a control block.
void
do_control_block(
    gui_context& ctx,
    accessor<bool> const& active,
    accessor<bool> const& disabled,
    accessor<styled_text> const& label,
    control_block_interface& block);

// Do a control block.
void static
do_control_block(
    gui_context& ctx,
    accessor<bool> const& active,
    accessor<styled_text> const& label,
    control_block_interface& block)
{
    do_control_block(ctx, active,
        in(false), label, block);
}

// Do a control block with an unstyled label.
void static
do_control_block(
    gui_context& ctx,
    accessor<bool> const& active,
    accessor<bool> const& disabled,
    accessor<string> const& label,
    control_block_interface& block)
{
    do_control_block(ctx, active, disabled,
        gui_apply(ctx, make_unstyled_text, label),
        block);
}

// Do a control block with an unstyled label.
void static
do_control_block(
    gui_context& ctx,
    accessor<bool> const& active,
    accessor<string> const& label,
    control_block_interface& block)
{
    do_control_block(ctx, active, in(false),
        gui_apply(ctx, make_unstyled_text, label),
        block);
}

// ITEM LISTS
//
// This section provides a rich UI for integrating with lists of items.
// The UI provides options for different views of the items in the list and
// options for the copying, deleting and editing them.

api(enum internal)
enum class item_list_view_mode
{
    COLLAPSED, COMPACT, DETAILED
};

void static inline
ensure_default_initialization(item_list_view_mode& x)
{ x = item_list_view_mode::COMPACT; }

api(enum internal)
enum class list_item_mode
{
    NORMAL,
    EDITING,
    DELETING
};

void static inline
ensure_default_initialization(list_item_mode& x)
{ x = list_item_mode::NORMAL; }

ALIA_DEFINE_FLAG_TYPE(list_item_options)
ALIA_DEFINE_FLAG(list_item_options, 0x0001, LIST_ITEM_COPY)
ALIA_DEFINE_FLAG(list_item_options, 0x0002, LIST_ITEM_CLONE)
ALIA_DEFINE_FLAG(list_item_options, 0x0004, LIST_ITEM_EDIT)
ALIA_DEFINE_FLAG(list_item_options, 0x0010, LIST_ITEM_DELETE)
ALIA_DEFINE_FLAG(list_item_options, 0x0020, LIST_ITEM_EDIT_DISABLED)

// Items in an item list UI must provide the following interface.
struct list_item_interface
{
    // QUERIES

    // Get the set of valid options for the item.
    virtual list_item_options_flag_set
    get_available_options()
    {
        return LIST_ITEM_COPY | LIST_ITEM_CLONE | LIST_ITEM_EDIT |
            LIST_ITEM_DELETE | LIST_ITEM_EDIT_DISABLED;
    }

    // UI CONTROLLERS

    // Do the item's header UI.
    // This includes the items label and anything else that should always be
    // visible.
    virtual void do_header_ui(gui_context& ctx) = 0;

    // Do the item's info UI.
    // This is generally a non-interactive UI that shows useful information
    // about the item.
    virtual void do_info_ui(gui_context& ctx) = 0;

    // Do the item's editing UI.
    virtual void do_editing_ui(gui_context& ctx) = 0;

    // Do the item's deletion UI.
    virtual void do_deletion_ui(gui_context& ctx) = 0;

    // EVENT HANDLERS

    // The item has just become the active one.
    virtual void on_activate() {}

    // The item has just become inactive.
    virtual void on_deactivate() {}

    // The item has just switched into edit mode.
    virtual void on_edit() {}

    // Copy the item to the clipboard.
    virtual void on_copy() = 0;

    // Create a clone of the item within the same list.
    virtual void on_clone() = 0;
};

// Do the UI for an individual list item.
void do_list_item_ui(
    gui_context& ctx, list_item_interface& item,
    accessor<item_list_view_mode> const& view_mode,
    accessor<bool> const& is_active,
    accessor<list_item_mode> const& item_mode);

// Do a heading at the start of a group of items.
void do_item_list_group_heading(
    gui_context& ctx, accessor<string> const& label);

// Do the whole item list UI.
// This takes care of the header and the fact that the items should only
// appear in certain views.
void do_item_list_ui(
    gui_context& ctx,
    accessor<string> const& label,
    accessor<item_list_view_mode> const& view_mode,
    boost::function<void()> const& do_items);

// Do the whole item list UI.
// This version allows custom options in the list header.
void do_item_list_ui(
    gui_context& ctx,
    accessor<string> const& label,
    accessor<item_list_view_mode> const& view_mode,
    boost::function<void()> const& do_items,
    boost::function<void(drop_down_menu_context& menu_ctx)> const& do_custom_options);

}

#endif
