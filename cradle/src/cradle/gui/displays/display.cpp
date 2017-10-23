#include <cradle/gui/displays/display.hpp>
#include <alia/ui/utilities.hpp>
#include <cradle/gui/widgets.hpp>
#include <cradle/gui/collections.hpp>

namespace cradle {

void static
do_view_item_selector(
    gui_context& ctx,
    accessor<string> const& active_item_id,
    char const* item_id, bool selected,
    accessor<string> const& label)
{
    auto widget_id = get_widget_id(ctx);
    panel p(ctx, text("item"), UNPADDED, PANEL_UNSAFE_CLICK_DETECTION,
        widget_id,
        get_widget_state(ctx, widget_id,
            selected ? WIDGET_SELECTED : NO_FLAGS));
    if (detect_click(ctx, widget_id, LEFT_BUTTON))
    {
        set(active_item_id, string(item_id));
        end_pass(ctx);
    }
    do_text(ctx, label);
}

struct view_list_content_iteration
{
    view_list_content_iteration(
        display_view_provider_interface& provider,
        accessor<display_view_instance_list> const& views)
      : provider(&provider), views(&views), current_index(0)
    {}
    size_t n_views() const
    { return is_gettable(*views) ? get(*views).size() : 0; }
    bool at_end() const
    { return current_index == n_views(); }
    void do_one(gui_context& ctx, naming_context& nc, bool is_preview)
    {
        alia_if (!at_end())
        {
            auto current_view = get(*views)[current_index];
            // Use a named block that won't automatically delete its contents so that the
            // associated with inactive views isn't reset.
            named_block
                block(nc,
                    make_id_by_reference(current_view.instance_id),
                    manual_delete(true));
            provider->do_view_content(ctx, current_view.type_id,
                current_view.instance_id, is_preview);
            ++current_index;
        }
        alia_else
        {
            do_empty_display_panel(ctx, GROW);
        }
        alia_end
    }
    display_view_provider_interface* provider;
    accessor<display_view_instance_list> const* views;
    size_t current_index;
};

void static
do_view_layout(
    gui_context& ctx, display_view_provider_interface& provider,
    display_layout_type layout_type,
    accessor<display_view_instance_list> const& views,
    bool is_preview)
{
    // Use a shared naming context so that views reuse the same state even when invoked
    // from different compositions.
    naming_context nc(ctx);
    alia_switch (layout_type)
    {
     alia_case (display_layout_type::TWO_COLUMNS):
      {
        uniform_grid_layout grid(ctx, GROW);
        view_list_content_iteration iteration(provider, views);
        while (!iteration.at_end())
        {
            uniform_grid_row row(grid);
            iteration.do_one(ctx, nc, is_preview);
            iteration.do_one(ctx, nc, is_preview);
        }
        break;
      }
     alia_case (display_layout_type::TWO_ROWS):
      {
        uniform_grid_layout grid(ctx, GROW);
        view_list_content_iteration iteration(provider, views);
        size_t views_per_row = (iteration.n_views() + 1) / 2;
        {
            uniform_grid_row row(grid);
            for (size_t i = 0; i != views_per_row; ++i)
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_if (iteration.n_views() > 1)
        {
            uniform_grid_row row(grid);
            for (size_t i = 0; i != views_per_row; ++i)
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_end
        break;
      }
     alia_case (display_layout_type::MAIN_PLUS_ROW):
      {
        view_list_content_iteration iteration(provider, views);
        column_layout column(ctx, GROW);
        {
            row_layout row(ctx, layout(FILL, 3.));
            if (!iteration.at_end())
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_if (iteration.n_views() > 1)
        {
            row_layout row(ctx, layout(FILL, 1.));
            while (!iteration.at_end())
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_end
        break;
      }
     alia_case (display_layout_type::MAIN_PLUS_SMALL_COLUMN):
      {
        view_list_content_iteration iteration(provider, views);
        row_layout row(ctx, GROW);
        {
            column_layout column(ctx, layout(FILL, 3.));
            if (!iteration.at_end())
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_if (iteration.n_views() > 1)
        {
            column_layout column(ctx, layout(FILL, 1.));
            while (!iteration.at_end())
                iteration.do_one(ctx, nc, is_preview);
        }
        alia_end
        break;
      }
     alia_case (display_layout_type::COLUMN_PLUS_MAIN):
      {
        view_list_content_iteration iteration(provider, views);
        row_layout row(ctx, GROW);
        {
            uniform_grid_layout grid(ctx, GROW);
            alia_if (iteration.n_views() > 1)
            {
                auto i = iteration.n_views();
                column_layout column(ctx, layout(FILL, 1.));
                while (!iteration.at_end() && i > 1)
                {
                    uniform_grid_row row(grid);
                    iteration.do_one(ctx, nc, is_preview);
                    --i;
                }
            }
            alia_end
        }
        {
            column_layout column(ctx, layout(FILL, 1));
            if (!iteration.at_end())
            {
                iteration.do_one(ctx, nc, is_preview);
            }
        }
        break;
      }
     alia_case (display_layout_type::MAIN_PLUS_COLUMN):
     {
         view_list_content_iteration iteration(provider, views);
         row_layout row(ctx, GROW);
         {
             column_layout column(ctx, layout(FILL, 1));
             if (!iteration.at_end())
             {
                 iteration.do_one(ctx, nc, is_preview);
             }
         }
         {
             uniform_grid_layout grid(ctx, GROW);
             alia_if (iteration.n_views() > 1)
             {
                 column_layout column(ctx, layout(FILL, 1.));
                 while (!iteration.at_end())
                 {
                     uniform_grid_row row(grid);
                     iteration.do_one(ctx, nc, is_preview);
                 }
             }
             alia_end
         }
         break;
     }
     alia_default:
        do_empty_display_panel(ctx, GROW);
        break;
    }
    alia_end
}

void static
do_composition_selector(
    gui_context& ctx, display_view_provider_interface& provider,
    accessor<string> const& selected_composition_id,
    accessor<display_view_composition> const& composition)
{
    auto widget_id = get_widget_id(ctx);
    auto selected =
        make_radio_accessor(ref(&selected_composition_id),
            _field(composition, id));
    panel p(ctx, text("item"), default_layout, PANEL_UNSAFE_CLICK_DETECTION,
        widget_id,
        get_widget_state(ctx, widget_id,
            is_gettable(selected) && get(selected) ?
            WIDGET_SELECTED : NO_FLAGS));
    if (detect_click(ctx, widget_id, LEFT_BUTTON))
    {
        set(selected, true);
        end_pass(ctx);
    }
    do_text(ctx, _field(ref(&composition), label));
}

display_view_composition static
make_composition_for_view(string const& type_id, string const& type_label)
{
    display_view_composition composition;
    composition.label = type_label;
    composition.id = type_id;
    composition.layout = display_layout_type::MAIN_PLUS_SMALL_COLUMN;
    composition.views.push_back(display_view_instance(type_id, type_id));
    return composition;
}

display_view_composition_list static
generate_single_view_compositions(
    display_view_provider_interface& provider)
{
    display_view_composition_list compositions;
    for (unsigned i = 0; i != provider.get_count(); ++i)
    {
        auto const& type_id = provider.get_type_id(i);
        compositions.push_back(make_composition_for_view(type_id,
            provider.get_type_label(type_id)));
    }
    return compositions;
}

void static
do_display_controls(
    gui_context& ctx, display_view_provider_interface& provider,
    boost::function<void (
        gui_context& ctx, accessor<display_state> const& state,
        accordion& accordion)> const& do_controls,
    accessor<display_state> const& state)
{
    //auto selected_view = get_state(ctx, (display_view_interface*)(0));

    accordion accordion(ctx);

    auto selected_composition = _field(ref(&state), selected_composition);

    //{
    //    //accordion_section section(accordion);
    //    do_heading(ctx, text("section-heading"), text("Composition"));
    //    //alia_if (section.do_content())
    //    {
    //        scoped_substyle style(ctx, text("view-composition-selection"));
    //        for_each(ctx,
    //            [&](gui_context& ctx, accessor<string> const& id,
    //                accessor<display_view_composition> const& composition)
    //            {
    //                //do_separator(ctx);
    //                do_composition_selector(ctx, provider,
    //                    make_radio_accessor(selected_composition, get(id)),
    //                    composition);
    //            },
    //            _field(ref(&state), compositions));
    //    }
    //    //alia_end
    //}

    //auto composition_state =
    //    select_map_index(_field(ref(&state), compositions),
    //        selected_composition);

    do_controls(ctx, state, accordion);

    //naming_context nc(ctx);
    //for_each(ctx,
    //    [&](gui_context& ctx, size_t index,
    //        accessor<display_view_instance> const& view)
    //    {
    //        auto type_index = get(view).type_index;
    //        auto state = _field(ref(&view), state);
    //        named_block nb(nc,
    //            combine_ids(make_id(index), make_id(type_index)));
    //        accordion_section section(accordion);
    //        do_text(ctx, provider.get_view_label(ctx, type_index, state));
    //        alia_if (section.do_content())
    //        {
    //            provider.do_view_controls(ctx, type_index, state);
    //        }
    //        alia_end
    //    },
    //    _field(composition_state, views));
}

enum class select_display_group
{
    VIEWS,
    COMPOSITIONS
};

void static
do_composition_group_selection_ui(
    gui_context& ctx, display_view_provider_interface& provider,
    accessor<bool> const& selected,
    accessor<string> const& group_label,
    accessor<display_view_composition_list> const& compositions,
    accessor<string> const& selected_composition_id)
{
    alia_if (is_gettable(compositions) && !get(compositions).empty())
    {
        horizontal_accordion_section section(ctx, selected);
        {
            panel p(ctx, text("heading"));
            do_text(ctx, group_label);
        }
        alia_if (section.do_content())
        {
            for_each(ctx,
                [&](gui_context& ctx, size_t index,
                    accessor<display_view_composition> const& composition)
                {
                    do_composition_selector(ctx, provider,
                        selected_composition_id, composition);
                },
                compositions);
        }
        alia_end
    }
    alia_end
}

void static
do_composition_selection_row(gui_context& ctx,
    display_view_provider_interface& provider,
    accessor<display_view_composition_list> const& single_views,
    accessor<display_view_composition_list> const& compositions,
    accessor<string> const& selected_composition_id)
{
    scrollable_panel p(ctx, text("view-composition-selection"),
        FILL, PANEL_HORIZONTAL | PANEL_NO_VERTICAL_SCROLLING);

    auto selection = get_state(ctx, select_display_group::COMPOSITIONS);

    do_composition_group_selection_ui(ctx, provider,
        //make_radio_accessor(selection, select_display_group::VIEWS),
        in(true),
        text("Views"), single_views, selected_composition_id);

    do_composition_group_selection_ui(ctx, provider,
        //make_radio_accessor(selection, select_display_group::COMPOSITIONS),
        in(true),
        text("Compositions"), compositions, selected_composition_id);
}

string static
get_default_composition_id(
    display_view_composition_list const& single_views,
    display_view_composition_list const& compositions)
{
    if (!compositions.empty())
        return compositions.front().id;
    else if (!single_views.empty())
        return single_views.front().id;
    else
        return "";
}

display_view_composition static
get_composition_by_id(
    display_view_composition_list const& single_views,
    display_view_composition_list const& compositions,
    string const& id)
{
    for (auto const& composition : single_views)
    {
        if (composition.id == id)
            return composition;
    }
    for (auto const& composition : compositions)
    {
        if (composition.id == id)
            return composition;
    }
    return single_views[0];
    throw exception("unknown composition ID");
}

void do_display(gui_context& ctx,
    display_view_provider_interface& provider,
    accessor<display_view_composition_list> const& compositions,
    accessor<display_state> const& state,
    accessor<float> const& controls_width,
    boost::function<void (
        gui_context& ctx, accessor<display_state> const& state,
        accordion& accordion)> const& do_controls)
{
    state_accessor<display_view_composition_list> single_views;
    if (get_state(ctx, &single_views))
        set(single_views, generate_single_view_compositions(provider));

    auto selected_composition_id =
        add_fallback_value(
            unwrap_optional(_field(ref(&state), selected_composition)),
            gui_apply(ctx,
                get_default_composition_id,
                single_views,
                compositions));

    auto selected_composition =
        gui_apply(ctx,
            get_composition_by_id,
            single_views,
            compositions,
            selected_composition_id);

    column_layout column(ctx, GROW);

    do_composition_selection_row(ctx, provider, single_views, compositions,
        selected_composition_id);

    {
    row_layout row(ctx, GROW);

    // Do display.
    alia_if (is_gettable(selected_composition))
    {
        do_view_layout(ctx, provider, get(selected_composition).layout,
            _field(selected_composition, views), false);
    }
    alia_else
    {
        do_empty_display_panel(ctx, GROW);
    }
    alia_end

    //{
    //    naming_context nc(ctx);
    //    {
    //        named_block nb(nc, simplify_id(ctx, active_view_id).id(),
    //            manual_delete(true));
    //        alia_if (active_view)
    //        {
    //            active_view->do_display(ctx);
    //        }
    //        alia_else
    //        {
    //            do_empty_display_panel(ctx, GROW);
    //        }
    //        alia_end
    //    }
    //}

    auto controls_expanded = _field(ref(&state), controls_expanded);

    if (detect_key_press(ctx, key_code(']')))
    {
        set(controls_expanded, !get(controls_expanded));
        end_pass(ctx);
    }

    alia_if(is_gettable(controls_expanded))
    {
        // Do controls.
        {
            horizontal_collapsible_content
                collapsible(ctx, get(controls_expanded), default_transition, 0.);

            alia_if (collapsible.do_content())
            {
                // resizable_content works in screen pixels, but we want the UI
                // magnification factor to apply to the pixel count that we're storing in
                // :controls_width.
                auto width_in_pixels =
                    accessor_cast<int>(
                        scale(controls_width, ctx.system->style.magnification));
                resizable_content
                    resizable(ctx, width_in_pixels, RESIZABLE_CONTENT_PREPEND_SEPARATOR);

                scrollable_panel
                    controls(ctx,
                        text("display-controls"),
                        GROW);

                do_display_controls(ctx, provider, do_controls, state);
            }
            alia_end
        }
        do_right_panel_expander(ctx, controls_expanded, FILL_Y | UNPADDED);
    }
    alia_end

    }
}

}
