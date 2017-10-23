#include <cradle/gui/requests.hpp>

#include <boost/algorithm/cxx11/any_of.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/gui/app/internals.hpp>
#include <cradle/gui/background.hpp>
#include <cradle/gui/collections.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/services.hpp>
#include <cradle/io/generic_io.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/gui/background_job_request.hpp>

namespace cradle {

// a more flexible form of update_gui_request that can be reused for other
// purposes
bool static
update_generic_gui_request(
    gui_context& ctx, gui_request_data& data,
    accessor<framework_context> const& framework_context,
    accessor<untyped_request> const& request,
    background_request_interest_type interest)
{
    assert(is_refresh_pass(ctx));

    bool changed = false;

    if (!is_gettable(request) || !is_gettable(framework_context))
    {
        // If the request isn't gettable but the pointer is initialized,
        // reset the pointer.
        if (data.ptr.is_initialized())
        {
            data.ptr.reset();
            inc_version(data.output_id);
            changed = true;
        }
        // And since we don't have the request yet, there's nothing else to be
        // done.
        request_refresh(ctx, 1);
        return changed;
    }

    // If the request is gettable, but the pointer isn't initialized or
    // doesn't have the same ID, reset it to the new request.
    auto id_change_minimized_request =
        minimize_id_changes(ctx, &data.input_id, request);
    if (!data.ptr.is_initialized() ||
        data.ptr.requester_id() != id_change_minimized_request.id())
    {
        data.ptr.reset(
            ctx.gui_system->requests,
            id_change_minimized_request.id(),
            get(framework_context),
            get(request),
            interest);
        inc_version(data.output_id);
        changed = true;
    }

    // If we already have the result, we're done.
    if (data.ptr.is_resolved())
        return changed;

    // Otherwise, update to bring in changes from the background.
    data.ptr.update();

    // Check again to see if that made the pointer ready.
    if (data.ptr.is_resolved())
        changed = true;

    request_refresh(ctx, 1);

    return changed;
}

bool update_gui_request(
    gui_context& ctx, gui_request_data& data,
    accessor<framework_context> const& framework_context,
    accessor<untyped_request> const& request)
{
    return update_generic_gui_request(ctx, data, framework_context, request,
        background_request_interest_type::RESULT);
}

struct gui_request_objectified_form_accessor
 : accessor<optional<untyped_request> >
{
    gui_request_objectified_form_accessor() {}
    gui_request_objectified_form_accessor(gui_request_data& data)
      : data_(&data)
    {}
    background_request_ptr& request_ptr() const { return data_->ptr; }
    id_interface const& id() const
    {
        if (data_->ptr.is_initialized())
        {
            id_ = get_id(data_->output_id);
            return id_;
        }
        else
        {
            return no_id;
        }
    }
    optional<untyped_request> const& get() const
    { return data_->ptr.objectified_form(); }
    bool is_gettable() const { return data_->ptr.is_resolved(); }
    bool is_settable() const { return false; }
    void set(optional<untyped_request> const& value) const {}
 private:
    gui_request_data* data_;
    mutable value_id_by_reference<local_id> id_;
};

indirect_accessor<untyped_request>
gui_untyped_request_objectified_form(
    gui_context& ctx,
    accessor<framework_context> const& framework_context,
    accessor<untyped_request> const& request)
{
    gui_request_data* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        update_generic_gui_request(ctx, *data, framework_context, request,
            background_request_interest_type::OBJECTIFIED_FORM);
    }
    return make_indirect(ctx,
        unwrap_optional(gui_request_objectified_form_accessor(*data)));
}

// THINKNODE "LET" REQUESTS

struct let_request_job : background_web_job
{
    let_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        objectified_result_entity_id const& entity_id,
        augmented_calculation_request const& request)
      : entity_id(entity_id)
      , request(request)
    {
        this->system = bg;
    }

    bool inputs_ready()
    {
        return get_session_and_context(*this->system, &session, &context);
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        // Perform the calculation.
        auto result_info =
            submit_let_calculation_request(
                this->system, *this->connection, context,
                session, this->request, !entity_id.is_explicit);

        // If we get a result, this means the calculation exists, so we want
        // to cache it for both forms of requests. This ensures that dry run
        // results get filled in if an explicit request returns results.
        //
        // If there's no result, this must have been a dry run, so we only
        // write the result back to the dry run form.
        //
        // (The consequence of the above logic is that the dry run form is
        // always written to and the explicit form is only written if there's
        // a result.)
        if (result_info)
        {
            set_mutable_value(*this->system,
                make_id(
                    objectified_result_entity_id(
                        this->request.request,
                        true)),
                erase_type(make_immutable(result_info)),
                mutable_value_source::RETRIEVAL);
        }
        set_mutable_value(*this->system,
            make_id(
                objectified_result_entity_id(
                    this->request.request,
                    false)),
            erase_type(make_immutable(result_info)),
            mutable_value_source::RETRIEVAL);
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = string("let request submission\n") +
            (entity_id.is_explicit ? "explicit" : "dry run");
        return info;
    }

    objectified_result_entity_id entity_id;
    augmented_calculation_request request;

    framework_context context;
    web_session_data session;
};

// Make the entity ID for a meta request for the given request generator.
objectified_result_entity_id static
make_objectified_result_entity_id(
    augmented_calculation_request const& augmented_request,
    bool is_explicit)
{
    return objectified_result_entity_id(augmented_request.request, is_explicit);
}

indirect_accessor<optional<let_calculation_submission_info> >
gui_untyped_thinknode_calculation_request(
    gui_context& ctx,
    accessor<augmented_calculation_request> const& request,
    accessor<bool> const& is_explicit)
{
    auto entity_id =
        gui_apply(ctx,
            make_objectified_result_entity_id,
            request,
            is_explicit);

    // TODO: Technically this should use the immutable data cache in cases
    // where is_explicit is true, but the difference isn't really that
    // significant, so I'm leaving that as future work.
    auto result_info =
        gui_mutable_entity_value<
            optional<let_calculation_submission_info>,
            objectified_result_entity_id
          >(ctx,
            entity_id,
            [&](objectified_result_entity_id const& entity_id)
            {
                auto const& bg = ctx.gui_system->bg;
                add_background_job(*bg,
                    background_job_queue_type::REMOTE_CALCULATION,
                    0, // no controller
                    // The get(request) here is a little sketchy, but in
                    // reality there's no way for this to get called if it's
                    // not gettable, so this is fine for now.
                    new let_request_job(bg, entity_id, get(request)));
            });

    return make_indirect(ctx, result_info);
}

// Show a calculation ID and allow copying it.
void static
do_calculation_id(
    gui_context& ctx,
    accessor<string> const& id)
{
    row_layout row(ctx);
    do_text(ctx, id);
    alia_if (is_gettable(id))
    {
        alia_untracked_if (do_link(ctx, text("copy")))
        {
            ctx.system->os->set_clipboard_text(get(id));
            end_pass(ctx);
        }
        alia_end
    }
    alia_end
}

web_response
remove_interest_from_calc_id(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data const& session,
    framework_context const& context,
    string const& calc_id)
{
    auto request =
        make_delete_request(
            context.framework.api_url + "/calc/" + calc_id +
                "/interest?context=" + context.context_id,
            make_header_list("Accept: application/json"));

    web_response response;
    try
    {
        response =
            perform_web_request(
                check_in,
                reporter,
                connection,
                session,
                request);
    }
    catch (web_request_failure& failure)
    {
        throw exception("remove_interest_from_calc_id with failure code of " +
            to_string(failure.response_code()) + "\n" + request.url);
    }
    return response;
}

background_job_result
perform_remove_interest_web_request(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data const& session,
    framework_context const& context,
    let_calculation_submission_info const& info)
{
    auto interest_res =
        remove_interest_from_calc_id(
            check_in,
            reporter,
            connection,
            session,
            context,
            info.main_calc_id);

    for (auto const& sub_calc : info.reported_subcalcs)
    {
        auto sub_interest_res =
            remove_interest_from_calc_id(
                check_in,
                reporter,
                connection,
                session,
                context,
                sub_calc.id);
    }

    background_job_result res;
    res.message = string();
    res.error = false;

    return res;
}

struct remove_interest_background_job : background_web_job
{
    remove_interest_background_job(
        alia__shared_ptr<background_execution_system> const& bg,
        id_interface const& id,
        let_calculation_submission_info const& info,
        dynamic_type_interface const* result_interface);

    bool inputs_ready();
    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter);
    background_job_info get_info() const;

    owned_id id;
    let_calculation_submission_info info;
    dynamic_type_interface const* result_interface;

    web_session_data session;
    framework_context context;
};

remove_interest_background_job::remove_interest_background_job(
    alia__shared_ptr<background_execution_system> const& bg,
    id_interface const& id,
    let_calculation_submission_info const& info,
    dynamic_type_interface const* result_interface)
    : info(info),
    result_interface(result_interface)
{
    this->system = bg;
    this->id.store(id);
}

bool remove_interest_background_job::inputs_ready()
{
    return get_session_and_context(*this->system, &session, &context);
}

void remove_interest_background_job::execute(
    check_in_interface& check_in,
    progress_reporter_interface& reporter)
{
    auto res =
        perform_remove_interest_web_request(
            check_in,
            reporter,
            *this->connection,
            session,
            context,
            info);

    auto result = result_interface->value_to_immutable(to_value(res));
    set_cached_data(*this->system, this->id.get(), result);
}

background_job_info remove_interest_background_job::get_info() const
{
    background_job_info info;
    info.description = "Removing interest in calculation";
    return info;
}

bool remove_interest(
    gui_context& ctx,
    background_job_data& data,
    let_calculation_submission_info const& info,
    dynamic_type_interface const& result_interface)
{
    local_identity* id;
    get_cached_data(ctx, &id);

    return
        update_general_background_job(ctx, data, get_id(*id),
            [&]() -> background_job_interface* // create_background_job
            {
                return new
                    remove_interest_background_job(
                        ctx.gui_system->bg,
                        get_id(*id),
                        info,
                        &result_interface);
            });
}

background_job_accessor
remove_interest_in_calculation(
    gui_context& ctx,
    accessor<let_calculation_submission_info> const& info)
{
    typed_background_job_data* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx) && is_gettable(info))
    {
        static dynamic_type_implementation<background_job_result> result_interface;
        if (remove_interest(ctx,
            data->untyped,
            get(info),
            result_interface))
        {
            if (data->untyped.ptr.is_ready())
            {
                cast_immutable_value(
                    &data->result,
                    data->untyped.ptr.data().ptr.get());
            }
            else
            {
                data->result = 0;
            }
        }
    }
    return background_job_accessor(*data);
}

calc_status_summary static
generate_calc_status_summary(
    let_calculation_submission_info const& info,
    calculation_status const& main_calc_status,
    std::vector<calculation_status> const& reported_calc_statuses,
    std::vector<calculation_queue_item> const& queue)
{
    // If the main calc status is any of the following, then then we know
    // immediately what the overall status is.
    switch (main_calc_status.type)
    {
     case calculation_status_type::COMPLETED:
        return make_calc_status_summary_with_completed(nil);
     case calculation_status_type::FAILED:
        return make_calc_status_summary_with_failed(nil);
     case calculation_status_type::CALCULATING:
     case calculation_status_type::UPLOADING:
        return make_calc_status_summary_with_running(nil);
     case calculation_status_type::CANCELED:
         return make_calc_status_summary_with_canceled(nil);
    }

    // Similarly, since we already know the status of the reported
    // calculations, if any of those are running, the whole calculation is
    // running. (Theoretically, this step should be unnecessary because those
    // calculations should show up below, but the statuses of the reported
    // calculations generally update more quickly than the queue status, so if
    // we don't check here, we could have an inconsistent report.)
    for (auto const& reported_calc_status : reported_calc_statuses)
    {
        switch (reported_calc_status.type)
        {
         case calculation_status_type::CALCULATING:
         case calculation_status_type::UPLOADING:
            return make_calc_status_summary_with_running(nil);
        }
    }

    // Find the first item in the queue that belongs to this calculation.
    {
        size_t position = 0;
        for (auto const& item : queue)
        {
            // Does this queue item match any of the IDs that we know about in
            // this calculation?
            if (item.id == info.main_calc_id ||
                boost::algorithm::any_of_equal(
                    info.other_subcalc_ids,
                    item.id) ||
                boost::algorithm::any_of(
                    info.reported_subcalcs,
                    [&item](reported_calculation_info const& reported)
                    { return reported.id == item.id; }))
            {
                switch (item.status)
                {
                 case calculation_queue_item_status::RUNNING:
                    return make_calc_status_summary_with_running(nil);
                 case calculation_queue_item_status::READY:
                 case calculation_queue_item_status::DEFERRED:
                    return make_calc_status_summary_with_queued(position);
                }
            }
            ++position;
        }
    }
    // If we get here, it's probably because we have incomplete information
    // about what calculations are actually in the tree and we simply missed
    // one from the queue, but that calculation will probably finish quickly
    // anyway and other calculations that we know about will get pushed into
    // the queue, so it seems reasonable to claim that we are queued at the
    // end.
    return make_calc_status_summary_with_queued(queue.size());
}

styled_text static
generate_calc_status_summary_text(calc_status_summary const& summary,
    optional<string> const& completed_message)
{
    switch (summary.type)
    {
     case calc_status_summary_type::COMPLETED:
     {
        if (completed_message)
        {
             return make_unstyled_text(get(completed_message));
        }
        else
        {
            return make_unstyled_text("completed");
        }
     }
     case calc_status_summary_type::FAILED:
        return make_unstyled_text("failed");
     case calc_status_summary_type::RUNNING:
        return make_unstyled_text("running");
     case calc_status_summary_type::QUEUED:
        return
            make_unstyled_text(
                "queued - #" +
                to_string(as_queued(summary) + 1));
     case calc_status_summary_type::CANCELED:
         return make_unstyled_text("canceled by user");
     default:
        throw exception("internal error: invalid calc_status_summary_type");
    }
}

// This does a display of the status of a remote calculation and gives an
// option to copy its ID.
void static
do_calculation_status_ui(
    gui_context& ctx,
    accessor<let_calculation_submission_info> const& info,
    accessor<string> const& trigger,
    accessor<optional<string> > const& completed_message)
{
    auto reported_calc_statuses =
        gui_map<calculation_status>(ctx,
            [ ](gui_context& ctx,
                accessor<reported_calculation_info> const& subcalc)
            {
                return gui_calc_status(ctx, _field(subcalc, id));
            },
            _field(info, reported_subcalcs));

    // The main_calc_id from the info may not be the actual calculation we
    // want to report progress on (it could be a higher level untracked
    // calc). So grab the last calculation in the tracked calcs to substitute
    // for this.
    auto main_calc_id =
        gui_apply(ctx,
            [ ] (std::vector<reported_calculation_info> const& calcs,
                string const& main_id)
            {
                //return calcs.back().id;
                return calcs.size() > 0
                    ? calcs.back().id
                    : main_id;
            },
            _field(info, reported_subcalcs),
            _field(info, main_calc_id));

    auto status_summary =
        gui_apply(ctx,
            generate_calc_status_summary,
            info,
            gui_calc_status(ctx, main_calc_id),
            reported_calc_statuses,
            gui_calc_queue_status(ctx));

    auto reported_prereq_subcalcs =
        gui_apply(ctx,
            [ ] (std::vector<reported_calculation_info> const& reported_subcalcs,
                string const& main_calc_id)
            {
                std::vector<reported_calculation_info> subcalcs;
                for (auto const& subcalc : reported_subcalcs)
                {
                    if (subcalc.id != main_calc_id)
                    {
                        subcalcs.push_back(subcalc);
                    }
                }
                return subcalcs;
            },
            _field(info, reported_subcalcs),
            main_calc_id);

    auto reported_prereq_calc_statuses =
        gui_map<calculation_status>(ctx,
            [&](gui_context& ctx,
                accessor<reported_calculation_info > const& subcalc)
            {
                return gui_calc_status(ctx, _field(subcalc, id));
            },
            reported_prereq_subcalcs);

    auto total_prereq_progress =
        gui_apply(ctx,
            [ ](std::vector<calculation_status> const& calc_statuses)
            {
                double total_completed = 0;

                // If there's no subcalcs, return 0 as to not return an nan
                if (calc_statuses.size() == 0)
                {
                    return 0.;
                }

                for (auto const& i : calc_statuses)
                {
                    if (i.type == calculation_status_type::COMPLETED ||
                        i.type == calculation_status_type::UPLOADING)
                    {
                        total_completed += 1.;
                    }
                    else if (i.type == calculation_status_type::CALCULATING)
                    {
                        total_completed += double(as_calculating(i).progress);
                    }
                }
                return
                    // Round down to nearest tenth
                    floor((total_completed / calc_statuses.size()) * 1000) / 1000;
            },
            reported_prereq_calc_statuses);

    auto main_calc_status = gui_calc_status(ctx, main_calc_id);
    auto main_calc_progress =
        gui_apply(ctx,
            [ ](calculation_status const& calc_status)
            {
                if (calc_status.type == calculation_status_type::COMPLETED ||
                    calc_status.type == calculation_status_type::UPLOADING)
                {
                    return 1.;
                }
                else if (calc_status.type == calculation_status_type::CALCULATING)
                {
                    return
                        floor((double(as_calculating(calc_status).progress)) * 1000) / 1000;
                }
                else
                {
                    return 0.;
                }
            },
            main_calc_status);

    auto is_main_calc_pending =
        gui_apply(ctx,
            [ ](calculation_status const& status)
            {
                return
                    is_queued(status) &&
                    as_queued(status) ==
                        calculation_queue_type::PENDING;
            },
            main_calc_status);

    // Show the overall status summary and provide an option to show details.
    {
        row_layout row(ctx);

        alia_if (is_true(is_main_calc_pending))
        {
            do_text(ctx, text("dependent calculations"));
            do_text(ctx,
                gui_apply(ctx,
                    generate_calc_status_summary_text,
                    status_summary,
                    in(optional<string>(none))));
            do_spacer(ctx, GROW_X);
            do_text(ctx, printf(ctx,
                "%.1f%%",
                scale(total_prereq_progress, 100)),
                RIGHT);
        }
        alia_else
        {
            do_text(ctx,
                gui_apply(ctx,
                    generate_calc_status_summary_text,
                    status_summary,
                    completed_message));

            alia_if(!is_equal(_field(status_summary, type),
                        calc_status_summary_type::COMPLETED))
            {
                do_spacer(ctx, GROW_X);
                do_text(ctx, printf(ctx,
                    "%.1f%%",
                    scale(main_calc_progress, 100)),
                    RIGHT);
            }
            alia_end
        }
        alia_end
    }

    auto show_details = get_state(ctx, false);
    auto show_completed = get_state(ctx, false);

    auto cancel_calculation = get_state(ctx, false);
    alia_if(is_true(cancel_calculation))
    {
        auto job_res = remove_interest_in_calculation(ctx, info);
        if (is_false(_field(job_res, error)))
        {
            set(cancel_calculation, false);
            set(trigger, string());

            // ISSUE: AST-1262
            clear_mutable_data_cache(get_background_system(ctx));
            end_pass(ctx);
        }
    }
    alia_end

    // Show some summary information about the reported calculations.
    // (Only show this if the calculation is incomplete.)
    alia_if (is_gettable(status_summary) && !is_completed(get(status_summary)))
    {
        auto summary_text =
            gui_apply(ctx,
                [ ] (std::vector<calculation_status> const& statuses)
                {
                    size_t completed =
                        std::count_if(
                            statuses.begin(), statuses.end(),
                            [ ](calculation_status const& status)
                            {
                                return is_completed(status);
                            });
                    size_t running =
                        std::count_if(
                            statuses.begin(), statuses.end(),
                            [ ](calculation_status const& status)
                            {
                                return is_calculating(status);
                            });
                    size_t ready =
                        std::count_if(
                            statuses.begin(), statuses.end(),
                            [ ](calculation_status const& status)
                            {
                                return
                                    is_queued(status) &&
                                        as_queued(status) ==
                                            calculation_queue_type::READY;
                            });
                    size_t pending =
                        std::count_if(
                            statuses.begin(), statuses.end(),
                            [ ](calculation_status const& status)
                            {
                                return
                                    is_queued(status) &&
                                        as_queued(status) ==
                                            calculation_queue_type::PENDING;
                            });

                    return
                        string(to_string(completed) + " completed, " +
                            to_string(running) + " running, " +
                            to_string(ready) + " ready, " +
                            to_string(pending) + " pending");
                },
                reported_calc_statuses);

        alia_if (is_gettable(summary_text))
        {
            alia_if (is_true(is_main_calc_pending))
            {
                scoped_substyle style(ctx, text("secondary"));
                do_progress_bar(ctx,
                    total_prereq_progress,
                    default_layout);
            }
            alia_else
            {
                do_progress_bar(ctx, main_calc_progress);
            }
            alia_end
            {
                alia_if(!is_equal(_field(status_summary, type),
                    calc_status_summary_type::CANCELED))
                {
                    alia_if(do_link(ctx, text("cancel"), RIGHT))
                    {
                        set(cancel_calculation, true);
                        end_pass(ctx);
                    }
                    alia_end
                }
                alia_end

                row_layout row(ctx);
                do_styled_text(ctx, text("heading"), text("Subcalulations"));
                do_spacer(ctx, GROW);
                alia_if (is_gettable(show_details))
                {
                    char const* label =
                        is_false(show_details) ? "show details" : "hide details";
                    alia_untracked_if (do_link(ctx, text(label)))
                    {
                        set(show_details, !get(show_details));
                    }
                    alia_end
                }
                alia_end
            }
            {
                row_layout row(ctx);
                do_text(ctx, summary_text);
            }
        }
        alia_end
    }
    alia_end

    // If the user wants to see details, show the calculation ID.
    do_collapsible_ui(ctx, show_details,
        [&]()
        {
            do_calculation_id(ctx, _field(info, main_calc_id));
            do_check_box(ctx, show_completed, text("show completed subcalculations"));

            // Show info about the individual reported calculations.
            for_each(ctx,
                [&](gui_context& ctx, size_t index,
                    accessor<calculation_status> const& status)
                {
                    auto subcalc_info =
                        select_index(
                            _field(info, reported_subcalcs),
                            index);

                    // Show a progress bar if this calculation is in progress.
                    _switch_accessor (_field(status, type))
                    {
                        alia_case (calculation_status_type::CALCULATING):
                            do_separator(ctx);
                            do_text(ctx, _field(subcalc_info, label));
                            do_calculation_id(ctx, _field(subcalc_info, id));
                            do_progress_bar(ctx,
                                accessor_cast<double>(
                                    _field(_union_member(status, calculating), progress)),
                                height (15, PIXELS));
                        break;
                        alia_case (calculation_status_type::COMPLETED):
                            do_collapsible_ui(ctx, show_completed,
                                [&]()
                                {
                                    do_separator(ctx);
                                    do_text(ctx, _field(subcalc_info, label));
                                    do_calculation_id(ctx, _field(subcalc_info, id));
                                    do_text(ctx, _field(status, type));
                                });
                        break;
                        alia_case (calculation_status_type::QUEUED):
                            do_separator(ctx);
                            do_text(ctx, _field(subcalc_info, label));
                            do_calculation_id(ctx, _field(subcalc_info, id));
                            do_text(ctx, _union_member(status, queued));
                        break;
                        alia_case (calculation_status_type::GENERATING):
                            do_separator(ctx);
                            do_text(ctx, _field(subcalc_info, label));
                            do_calculation_id(ctx, _field(subcalc_info, id));
                            do_text(ctx, text("generating"));
                        break;
                        alia_default:
                            do_separator(ctx);
                            do_text(ctx, _field(subcalc_info, label));
                            do_calculation_id(ctx, _field(subcalc_info, id));
                            do_text(ctx, _field(status, type));
                        break;
                    }
                    _end_switch
                },
                reported_calc_statuses);
        });
}

string static
generate_request_hash_string(augmented_calculation_request const& augmented)
{
    // This is only a 32-bit hash, but the consequences of a collision aren't
    // that severe, so it's fine.
    return value_to_base64_string(to_value(invoke_hash(augmented.request)));
}

void do_explicit_calculation_ui(gui_context& ctx,
    accessor<string> const& trigger,
    accessor<augmented_calculation_request> const& request,
    accessor<optional<string> > const& completed_message)
{
    // Calculate the hash of the current request.
    auto request_hash =
        gui_apply(ctx, generate_request_hash_string, request);

    // Uncomment to see the triggered and calculated hashes.
    // Useful for debugging.
    //do_text(ctx, trigger);
    //do_text(ctx, request_hash);

    alia_if (is_gettable(trigger) && is_gettable(request_hash))
    {
        alia_if (get(trigger) == get(request_hash))
        {
            // The user has explicitly requested the calculation, so do the
            // real request.

            // Changed the explicit real request flag from in(true) to in(false) to work
            // with managing the feasibility and mco requests together. The below statement should
            // no longer submit the real request. -Daniel (AST-1032)
            auto calc_id =
                gui_untyped_thinknode_calculation_request(
                    ctx, request, in(false));

            // Do a UI to show the status of it.
            alia_if (has_value(calc_id))
            {
                do_calculation_status_ui(ctx, unwrap_optional(calc_id), trigger, completed_message);
            }
            alia_else
            {
                do_text(ctx, text("waiting..."));
            }
            alia_end
        }
        alia_else
        {
            alia_if(is_gettable(request))
            {
                // The user hasn't explicitly requested the calculation, so do a
                // dry run to see if it happens to already be available.
                auto dry_run =
                    gui_untyped_thinknode_calculation_request(
                        ctx, request, in(false));
                // Check if the dry run is canceled
                auto dry_run_id = _field(unwrap_optional(dry_run), main_calc_id);
                auto dry_run_status = gui_calc_status(ctx, dry_run_id);

                alia_if (has_value(dry_run) &&
                    !is_equal(_field(dry_run_status, type), calculation_status_type::CANCELED))
                {
                    // The calculation is already requested, so show its status.
                    do_calculation_status_ui(ctx, unwrap_optional(dry_run), trigger, completed_message);
                }
                alia_else_if (is_gettable(dry_run))
                {
                    // The calculation hasn't been requested, so show a UI for
                    // requesting it.
                    do_text(ctx, text("not calculated"));
                    bool triggered = do_link(ctx, text("calculate"));
                    alia_untracked_if(triggered)
                    {
                        // Uncomment to see what request is being triggered.
                        // Useful for debugging.
                        //if (is_gettable(request))
                        //{
                        //    std::ofstream f("triggered.json");
                        //    f << value_to_json(to_value(as_request_object(get(request))));
                        //}
                        set(trigger, get(request_hash));
                        end_pass(ctx);
                    }
                    alia_end
                }
                alia_else
                {
                    // The dry run is still in progress.
                    do_text(ctx, text("querying calculation status"));
                }
                alia_end
            }
            alia_end
        }
        alia_end
    }
    alia_end
}

indirect_accessor<bool>
gui_request_is_triggered(
    gui_context& ctx, accessor<string> const& trigger,
    accessor<augmented_calculation_request> const& request)
{
    auto request_hash =
        gui_apply(ctx, generate_request_hash_string, request);
    return make_indirect(ctx,
        in(is_gettable(trigger) && is_gettable(request_hash) &&
            get(trigger) == get(request_hash)));
}

}
