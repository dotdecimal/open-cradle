#include <cradle/gui/background.hpp>

#include <alia/ui/utilities.hpp>

#include <cradle/background/system.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/widgets.hpp>

namespace cradle {

void deflickerer::begin(gui_context& ctx, unsigned delay)
{
    ctx_ = &ctx;
    if (get_cached_data(ctx, &data_))
    {
        data_->is_current = false;
        data_->children = 0;
    }
    if (is_refresh_pass(ctx))
        data_->children = 0;
    delay_ = delay;
    valid_so_far_ = true;
    change_detected_ = false;
}
void deflickerer::end()
{
    gui_context& ctx = *ctx_;
    deflickering_data& data = *data_;

    if (is_refresh_pass(ctx))
    {
        // If an input changes, record that the output is no longer current
        // and start a timer to track when the output values should expire.
        if (change_detected_)
        {
            if (data.is_current)
            {
                alia::start_timer(ctx, data_, delay_);
                data.is_current = false;
            }
        }
        else if (!data.is_current && valid_so_far_)
        {
            for (auto* i = data.children; i; i = i->next)
                i->copy_input();
            data.is_current = true;
        }
    }
    // If the timer expires and the output is still not current, invalidate it.
    else if (!data.is_current && detect_timer_event(ctx, data_))
    {
        for (auto* i = data.children; i; i = i->next)
            i->clear();
    }
}

// GUI REPORTS OF BACKGROUND SYSTEM

void issue_permanent_failure_notifications(gui_context& ctx)
{
    auto reports = get_permanent_failures(*ctx.gui_system->bg);
    for (auto const& report : reports)
        record_failure(ctx, report.message);
}

size_t static
get_active_job_count(background_execution_pool_status const& status)
{
    return get_active_thread_count(status) + status.queued_job_count;
}

size_t static
get_active_job_count(background_execution_system_status const& status,
    background_job_queue_type queue)
{
    auto const& pool = status.pools[int(queue)];
    assert(is_valid(pool));
    return get_active_job_count(get(pool));
}

size_t static
get_relevant_active_job_count(
    background_execution_system_status const& status)
{
    size_t relevant_count = 0;
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
    {
        if (i != unsigned(background_job_queue_type::NOTIFICATION_WATCH)/* &&
            i != unsigned(background_job_queue_type::REMOTE_CALCULATION)*/)
        {
            auto const& pool = status.pools[i];
            assert(is_valid(pool));
            relevant_count += get_active_job_count(get(pool));
        }
    }
    return relevant_count;
}

size_t static
get_transient_failure_count(background_execution_pool_status const& status)
{
    return status.transient_failures.size();
}

size_t static
get_transient_failure_count(background_execution_system_status const& status)
{
    size_t failed_count = 0;
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
    {
        auto const& pool = status.pools[i];
        assert(is_valid(pool));
        failed_count += get_transient_failure_count(get(pool));
    }
    return failed_count;
}

string static
pluralize(string const& item, size_t count)
{
    if (count == 1)
        return item;
    return item + "s";
}

void static
do_job_count(gui_context& ctx,
    accessor<size_t> const& job_count,
    accessor<string> const& label)
{
    alia_if (is_gettable(job_count) && get(job_count) != 0)
    {
        row_layout row(ctx);

        do_styled_text(ctx, text("job-count"),
            printf(ctx, "%lu", job_count));

        do_styled_text(ctx, text("job-type"),
            gui_apply(ctx, pluralize, label, job_count));
    }
    alia_end
}

void static
retry_failures(background_execution_system& system,
    background_execution_system_status const& status)
{
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
    {
        auto const& pool = status.pools[i];
        assert(is_valid(pool));
        for (auto const& f : get(pool).transient_failures)
        {
            retry_background_job(system,
                static_cast<background_job_queue_type>(i), f.job);
        }
    }
}

void static
show_failure_reports(gui_context& ctx,
    background_execution_system_status const& status)
{
    naming_context nc(ctx);
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
    {
        auto const& pool = status.pools[i];
        assert(is_valid(pool));
        for (auto const& f : get(pool).transient_failures)
        {
            named_block nb(nc, make_id(&f));
            do_paragraph(ctx, in_ptr(&f.message));
        }
    }
}

void static
show_job_info(gui_context& ctx,
    background_execution_system_status const& status)
{
    naming_context nc(ctx);
    for (unsigned i = 0; i != unsigned(background_job_queue_type::COUNT); ++i)
    {
        auto const& pool = status.pools[i];
        assert(is_valid(pool));
        for (auto const& i : get(pool).job_info)
        {
            named_block nb(nc, make_id(i.first));
            do_paragraph(ctx, in_ptr(&i.second.description));
            alia_untracked_if (do_link(ctx, text("copy")))
            {
                ctx.system->os->set_clipboard_text(i.second.description);
                end_pass(ctx);
            }
            alia_end
        }
    }
}

void do_background_status_report(gui_context& ctx)
{
    background_execution_system_status* status;
    get_data(ctx, &status);
    if (is_refresh_pass(ctx))
        update_status(*status, get_background_system(ctx));

    // Determine if anything is going on.
    auto active_job_count = get_relevant_active_job_count(*status);
    auto transient_failure_count = get_transient_failure_count(*status);
    bool processing = active_job_count != 0 || transient_failure_count != 0;

    // Starting it at a negative number when nothing's happening means that it
    // won't immediately show up when a calculation starts, so very transient
    // calculations won't cause flickering.
    float opacity = smooth_raw_value(ctx, processing ? 1.f : -2.f,
        animated_transition(default_curve, 600));

    auto minimized = get_state(ctx, true);

    alia_if (opacity > 0)
    {
        auto base_retry_time = 5;
        auto time_between_retries = get_state(ctx, base_retry_time);
        auto max_time_between_retries = 30.0;
        auto retry_countdown = get_state(ctx, 0);

        alia_if (transient_failure_count > 0)
        {
            timer t(ctx);
            if (t.triggered())
            {
                set(retry_countdown, get(retry_countdown) - 1);
                if (get(retry_countdown) == 0)
                {
                    retry_failures(get_background_system(ctx), *status);
                    alia_if (time_between_retries < in(max_time_between_retries))
                    {
                        set(time_between_retries, get(time_between_retries) + 5);
                    }
                    alia_end
                }
                else
                t.start(1000);
                end_pass(ctx);
            }
            if (!t.is_active())
            {
                set(retry_countdown, get(time_between_retries));
                t.start(1000);
            }
        }
        alia_end

        alia_if (transient_failure_count == 0 &&
            !is_equal(time_between_retries, base_retry_time))
        {
            // There's no failures and the time between failures is maxed
            set(time_between_retries, base_retry_time);
        }
        alia_end

        alia_if(!is_true(minimized))
        {
            scoped_surface_opacity opacity(ctx, opacity);

            panel p(ctx, text("background-status"), layout(RIGHT | TOP),
                PANEL_HORIZONTAL);

            do_animated_astroid(ctx, layout(size(10, 10, EM), TOP));

            {
                column_layout column(ctx, GROW);

                do_heading(ctx, text("heading"), text("processing"));

                {
                    do_job_count(ctx,
                        in(get_active_job_count(
                            *status, background_job_queue_type::REMOTE_CALCULATION)),
                        text("remote calculation"));

                    do_job_count(ctx,
                        in(get_active_job_count(
                            *status, background_job_queue_type::WEB_READ)),
                        text("thinknode read"));

                    do_job_count(ctx,
                        in(get_active_job_count(
                            *status, background_job_queue_type::WEB_WRITE)),
                        text("thinknode write"));

                    do_job_count(ctx,
                        in(get_active_job_count(
                            *status, background_job_queue_type::DISK)),
                        text("disk read"));

                    auto relevant_background_calculations =
                        get_active_job_count(
                            *status, background_job_queue_type::CALCULATION);
                    do_job_count(ctx,
                        smooth_value(ctx, in(relevant_background_calculations)),
                        text("local calculation"));
                }

                alia_if (transient_failure_count > 0)
                {
                    {
                        flow_layout flow(ctx, PADDED);
                        do_styled_text(ctx, text("transient-failure-count"),
                            printf(ctx, "%lu", in(transient_failure_count)));
                        do_text(ctx, text(transient_failure_count > 1 ?
                            "background jobs have failed (network disruption)." :
                            "background job has failed (network disruption)."));
                    }

                    {
                        row_layout row(ctx);
                        do_text(ctx,
                            printf(ctx, "Retrying in %i %s...",
                                retry_countdown,
                                text(get(retry_countdown) > 1 ?
                                    "seconds" : "second")));
                        if (do_link(ctx, text("retry now")))
                        {
                            retry_failures(get_background_system(ctx), *status);
                            alia_if (time_between_retries < in(max_time_between_retries))
                            {
                                set(time_between_retries, get(time_between_retries) + 5);
                            }
                            alia_end
                            end_pass(ctx);
                        }
                    }
                }
                alia_end

                auto show_details = get_state(ctx, true);
                //do_spacer(ctx, layout(height(0, EM), GROW));
                {
                    // Scrollable panels apparently don't work correctly inside
                    // collapsibles.
                    //collapsible_content collapsible(ctx, is_true(show_details));
                    //alia_if (collapsible.do_content())
                    alia_if (is_true(show_details))
                    {
                        scrollable_panel sp(ctx, text("content"),
                            height(15, EM),
                            PANEL_NO_HORIZONTAL_SCROLLING |
                            (is_true(show_details) ?
                                NO_FLAGS : PANEL_NO_VERTICAL_SCROLLING));
                        show_job_info(ctx, *status);
                        show_failure_reports(ctx, *status);
                    }
                    alia_end
                }
                // With new minimized window, no need to have this option anymore
                //alia_if (!get(show_details))
                //{
                //    if (do_link(ctx, text("show details")))
                //    {
                //        set(show_details, true);
                //        end_pass(ctx);
                //    }
                //}
                //alia_else
                //{
                //    if (do_link(ctx, text("hide details")))
                //    {
                //        set(show_details, false);
                //        end_pass(ctx);
                //    }
                //}
                //alia_end
            }

            alia_untracked_if(do_icon_button(ctx, icon_type::MINUS_ICON))
            {
                set(minimized, true);
            }
            alia_end
        }
        alia_else
        {
            scoped_surface_opacity opacity(ctx, opacity);

            alia_if (transient_failure_count > 0)
            {
                panel p(ctx, text("validation-error-panel"), layout(RIGHT | TOP),
                    PANEL_HORIZONTAL);

                do_animated_astroid(ctx, layout(size(1, 1, EM), RIGHT));

                do_styled_text(ctx,
                    text("heading"), text("network disruption, retrying..."));

                alia_untracked_if(
                    do_icon_button(ctx, icon_type::PLUS_ICON, layout(RIGHT)))
                {
                    set(minimized, false);
                }
                alia_end
            }
            alia_else
            {
                panel p(ctx, text("background-status-min"), layout(RIGHT | TOP),
                    PANEL_HORIZONTAL);

                do_animated_astroid(ctx, layout(size(1, 1, EM), RIGHT));

                auto remotes = get_active_job_count(
                    *status, background_job_queue_type::REMOTE_CALCULATION);
                auto webs =
                    get_active_job_count(
                        *status,
                        background_job_queue_type::WEB_READ) +
                    get_active_job_count(
                        *status,
                        background_job_queue_type::WEB_WRITE);
                auto disks = get_active_job_count(
                    *status, background_job_queue_type::DISK);
                auto relevant_background_calculations =
                    get_active_job_count(
                    *status, background_job_queue_type::CALCULATION);
                auto locals = smooth_value(ctx, in(relevant_background_calculations));

                {
                    row_layout r(ctx);
                    do_styled_text(ctx, text("heading"), text("Processing"));
                    alia_if(remotes > 0)
                    {
                        {
                            do_styled_text(ctx, text("job-type"), text("R :"));
                            do_styled_text(ctx,
                                text("job-count"), in(to_string(remotes)));
                        }
                    }
                    alia_end
                    alia_if(webs > 0)
                    {
                        {
                            do_styled_text(ctx, text("job-type"), text("W :"));
                            do_styled_text(ctx, text("job-count"), in(to_string(webs)));
                        }
                    }
                    alia_end
                    alia_if(disks > 0)
                    {
                        {
                            do_styled_text(ctx, text("job-type"), text("D :"));
                            do_styled_text(ctx, text("job-count"), in(to_string(disks)));
                        }
                    }
                    alia_end
                    alia_if(is_gettable(locals) && get(locals) > 0)
                    {
                        {
                            do_styled_text(ctx, text("job-type"), text("L :"));
                            do_styled_text(ctx,
                                text("job-count"), in(to_string(get(locals))));
                        }
                    }
                    alia_end
                }

                alia_untracked_if(
                    do_icon_button(ctx, icon_type::PLUS_ICON, layout(RIGHT)))
                {
                    set(minimized, false);
                }
                alia_end
            }
            alia_end
        }
        alia_end
    }
    alia_else
    {
        alia_if(!is_true(minimized))
        {
            panel p(ctx, text("background-status-min"), layout(RIGHT | TOP),
                PANEL_HORIZONTAL);
            //do_text(ctx, text("Idle"));
            //do_spacer(ctx, GROW_X);
            alia_untracked_if(do_icon_button(ctx, icon_type::MINUS_ICON, layout(TOP | RIGHT)))
            {
                set(minimized, true);
            }
            alia_end
        }
        alia_else
        {
            panel p(ctx, text("background-status-min"), layout(RIGHT | TOP),
                PANEL_HORIZONTAL);
            //do_spacer(ctx, GROW_X);
            alia_untracked_if(do_icon_button(ctx, icon_type::PLUS_ICON, layout(TOP | RIGHT)))
            {
                set(minimized, false);
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

styled_text static
data_size_as_text(size_t size)
{
    styled_text text;
    if (size < 0x400)
    {
        append_styled_text(text,
            make_styled_text_fragment("value", to_string(size)));
        append_styled_text(text,
            make_styled_text_fragment("units", "B"));
    }
    else if (size < 0x100000)
    {
        append_styled_text(text,
            make_styled_text_fragment("value", to_string(size / 0x400)));
        append_styled_text(text,
            make_styled_text_fragment("units", "kB"));
    }
    else
    {
        append_styled_text(text,
            make_styled_text_fragment("value", to_string(size / 0x100000)));
        append_styled_text(text,
            make_styled_text_fragment("units", "MB"));
    }
    return text;
}

void static
do_memory_cache_entry_info(gui_context& ctx,
    grid_layout& grid,
    accessor<memory_cache_entry_info> const& info)
{
    grid_row row(grid);
    do_flow_text(ctx,
        as_text(ctx,
            gui_apply(ctx,
                make_api_type_info,
                _field(info, type))),
        GROW);
    do_text(ctx,
        gui_apply(ctx,
            data_size_as_text,
            _field(info, data_size)));
}

size_t static
cache_entry_list_total_size(
    std::vector<memory_cache_entry_info> const& entries)
{
    size_t total_size = 0;
    for (auto const& entry : entries)
    {
        total_size += entry.data_size;
    }
    return total_size;
}

std::vector<size_t>
indices_ordered(size_t count)
{
    std::vector<size_t> indices;
    for (size_t i = 0; i != count; ++i)
        indices.push_back(i);
    return indices;
}

template<class Resolver>
std::vector<size_t>
sort_indices(size_t item_count, Resolver const& resolver)
{
    auto indices = indices_ordered(item_count);
    sort(indices.begin(), indices.end(),
        [&](size_t a, size_t b) { return resolver(a) < resolver(b); });
    return indices;
}

void static
do_memory_cache_entry_list(gui_context& ctx, accessor<string> const& label,
    accessor<std::vector<memory_cache_entry_info> > const& entries)
{
    do_heading(ctx, text("subheading"), label);
    {
        row_layout row(ctx);
        do_text(ctx,
            gui_apply(ctx,
                data_size_as_text,
                gui_apply(ctx,
                    cache_entry_list_total_size,
                    entries)));
    }
    alia_if(is_gettable(entries))
    {
        auto x = sort_indices(get(entries).size(),
        [&](size_t i) { return make_api_type_info(get(entries)[i].type); });


        tree_node tn(ctx);
        do_text(ctx, text("entries"));
        alia_if (tn.do_children())
        {
            grid_layout grid(ctx);
            for_each(ctx,
                [&](gui_context& ctx, size_t unused, // index of the index
                accessor<size_t> const& index)
                {
                    auto info = select_index_via_accessor(entries, index);
                    do_memory_cache_entry_info(ctx, grid, info);
                },
                in(x));
        }
        alia_end
    }
    alia_end
}

void do_memory_cache_report(gui_context& ctx)
{
    auto snapshot = get_state(ctx, memory_cache_snapshot());

    // Update the snapshot when transitioning into this UI.
    if (detect_transition_into_here(ctx))
    {
        set(snapshot, get_memory_cache_snapshot(*ctx.gui_system->bg));
        end_pass(ctx);
    }
    // Also periodically update the snapshot.
    {
        timer t(ctx);
        if (!t.is_active())
            t.start(1000);
        if (t.triggered())
        {
            set(snapshot, get_memory_cache_snapshot(*ctx.gui_system->bg));
            end_pass(ctx);
        }
    }

    do_memory_cache_entry_list(ctx, text("In Use"),
        _field(snapshot, in_use));
    do_memory_cache_entry_list(ctx, text("Recently Used"),
        _field(snapshot, pending_eviction));
}

}
