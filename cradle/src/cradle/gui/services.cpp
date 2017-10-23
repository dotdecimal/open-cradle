#include <cradle/gui/services.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/io/generic_io.hpp>
#include <cradle/io/services/iss.hpp>
#include <cradle/io/services/rks.hpp>
#include <cradle/io/services/state_service.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/app/internals.hpp>
#include <cradle/gui/collections.hpp>

namespace cradle {

// CAS

indirect_accessor<session_info>
get_session_info(gui_context& ctx, app_context& app_ctx)
{
    return make_indirect(ctx,
        gui_get_request<session_info>(ctx,
            gui_apply(ctx,
                construct_session_info_request_url,
                get_api_url(ctx, app_ctx)),
            in(no_headers)));
}

// IAM

indirect_accessor<user_info>
get_user_info(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& user_id)
{
    return make_indirect(ctx,
        gui_get_request<user_info>(ctx,
            gui_apply(ctx,
                construct_user_info_request_url,
                get_api_url(ctx, app_ctx),
                user_id),
            in(no_headers)));
}

indirect_accessor<realm>
get_realm_info(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& realm_id)
{
    return make_indirect(ctx,
        gui_get_request<realm>(ctx,
            gui_apply(ctx,
                construct_realm_info_request_url,
                get_api_url(ctx, app_ctx),
                realm_id),
            in(no_headers)));
}


// CALC STATUS

calc_status_entity_id
make_calc_status_entity_id(string const& id)
{
    calc_status_entity_id entity_id;
    entity_id.id = id;
    return entity_id;
}

// Given a calculation status, get the next status that would represent
// meaningful progress. If the result is none, no further progress is possible.
optional<calculation_status> static
get_next_calculation_status(calculation_status current)
{
    switch (current.type)
    {
     case calculation_status_type::WAITING:
        return
            make_calculation_status_with_queued(
                calculation_queue_type::PENDING);
     case calculation_status_type::GENERATING:
        return
            make_calculation_status_with_queued(
                calculation_queue_type::READY);
     case calculation_status_type::QUEUED:
        switch (as_queued(current))
        {
         case calculation_queue_type::PENDING:
            return
                make_calculation_status_with_queued(
                    calculation_queue_type::READY);
         case calculation_queue_type::READY:
            return
                make_calculation_status_with_calculating(
                    calculation_calculating_status(0));
         default:
            throw exception("internal error: invalid calculation_queue_type");
        }
     case calculation_status_type::CALCULATING:
      {
        // Wait for progress in increments of 1%.
        auto next_progress =
            std::floor(as_calculating(current).progress * 100 + 1) / 100;
        // Once we get to the end of the calculating phase, we want to wait
        // for the upload.
        return
            next_progress < 1
          ? make_calculation_status_with_calculating(
                calculation_calculating_status(next_progress))
          : make_calculation_status_with_uploading(
                calculation_uploading_status());
      }
     case calculation_status_type::UPLOADING:
      {
        // Wait for progress in increments of 1%.
        auto next_progress =
            std::floor(as_uploading(current).progress * 100 + 1) / 100;
        // Once we get to the end of the calculating phase, we want to wait
        // for the completed status.
        return
            next_progress < 1
          ? make_calculation_status_with_uploading(
                calculation_uploading_status(next_progress))
          : make_calculation_status_with_completed(nil);
      }
     case calculation_status_type::COMPLETED:
     case calculation_status_type::FAILED:
     case calculation_status_type::CANCELED:
        return none;
     default:
        throw exception("internal error: invalid calculation_status_type");
    }
}

// Get the query string repesentation of a calculation status.
string static
calc_status_as_query_string(calculation_status status)
{
    switch (status.type)
    {
     case calculation_status_type::WAITING:
        return "status=waiting";
     case calculation_status_type::GENERATING:
        return "status=generating";
     case calculation_status_type::QUEUED:
        switch (as_queued(status))
        {
         case calculation_queue_type::PENDING:
            return "status=queued&queued=pending";
         case calculation_queue_type::READY:
            return "status=queued&queued=ready";
         default:
            throw exception("internal error: invalid calculation_queue_type");
        }
     case calculation_status_type::CALCULATING:
        return "status=calculating&progress=" +
            to_string(as_calculating(status).progress);
     case calculation_status_type::UPLOADING:
        return "status=uploading&progress=0";
     case calculation_status_type::COMPLETED:
        return "status=completed";
     case calculation_status_type::FAILED:
        return "status=failed";
     case calculation_status_type::CANCELED:
         return "status=canceled";
     default:
        throw exception("internal error: invalid calculation_status_type");
    }
}

struct calc_status_request_job : background_web_job
{
    calc_status_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        calc_status_entity_id const& calc_id)
      : calc_id(calc_id)
    {
        this->system = bg;
    }

    bool inputs_ready()
    {
        return true;
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        // Currently, the system assumes that long-polling jobs don't fail, so
        // this job tries to be as robust as possible.
        while (true)
        {
            try
            {
                framework_context context;
                web_session_data session;
                // If we can't get the context or session yet, just wait a bit
                // and try again.
                if (!get_session_and_context(*this->system, &session,
                        &context))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(1));
                    continue;
                }

                // Query the initial status.
                auto status =
                    cradle::from_value<calculation_status>(
                        parse_json_response(
                            perform_web_request(
                                check_in,
                                reporter,
                                *this->connection,
                                session,
                                make_get_request(
                                    context.framework.api_url + "/calc/" +
                                        calc_id.id + "/status?context=" +
                                        context.context_id,
                                    no_headers))));

                while (true)
                {
                    // Report the latest status to the mutable data cache.
                    set_mutable_value(*this->system, make_id(calc_id),
                        erase_type(make_immutable(status)),
                        mutable_value_source::WATCH);

                   check_in();

                    // Determine the next meaningful calculation status.
                    auto next_status = get_next_calculation_status(status);
                    // If there is none, we're done here.
                    if (!next_status)
                        return;

                    // Long poll for that status and update the actual status
                    // with whatever Thinknode reports back.
                    status =
                        cradle::from_value<calculation_status>(
                            parse_json_response(
                                perform_web_request(
                                    check_in,
                                    reporter,
                                    *this->connection,
                                    session,
                                    make_get_request(
                                        context.framework.api_url +
                                            "/calc/" + calc_id.id +
                                            "/status?" +
                                            calc_status_as_query_string(
                                                get(next_status)) +
                                            "&timeout=120" +
                                            "&context=" + context.context_id,
                                        no_headers))));

                    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
                }
            }
            catch (background_job_canceled&)
            {
                // If someone wants to actually cancel this job, then let that
                // through...
                throw;
            }
            catch (...)
            {
                // If anything else happens, just try again.
                boost::this_thread::sleep_for(boost::chrono::seconds(1));
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "Calc status request " + to_string(calc_id);
        return info;
    }

    calc_status_entity_id calc_id;
};

indirect_accessor<calculation_status>
gui_calc_status(gui_context& ctx,
    accessor<string> const& calc_id)
{
    return
        make_indirect(ctx,
            gui_mutable_entity_value<calculation_status,calc_status_entity_id>(
                ctx,
                gui_apply(ctx, make_calc_status_entity_id, calc_id),
                [&](calc_status_entity_id const& entity_id)
                {
                    auto const& bg = ctx.gui_system->bg;
                    add_background_job(*bg,
                        background_job_queue_type::WEB_READ,
                        0, // no controller
                        new calc_status_request_job(bg, entity_id),
                        BACKGROUND_JOB_HIDDEN);
                }));
}

// CALC QUEUE

struct calc_queue_query_job : background_web_job
{
    calc_queue_query_job(
        alia__shared_ptr<background_execution_system> const& bg)
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
        // Currently, the system assumes that long-polling jobs don't fail, so
        // this job tries to be as robust as possible.
        while (true)
        {
            try
            {
                framework_context context;
                web_session_data session;
                // If we can't get the context or session yet, just wait a bit
                // and try again.
                if (!get_session_and_context(*this->system, &session,
                        &context))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(1));
                    continue;
                }

                // There's currently no long-polling feature for the
                // calculation queue, so just continue checking it every once
                // in a while as long as this job is still going.
                while (true)
                {
                    check_in();
                    auto queue =
                        query_calculation_queue(
                            check_in,
                            *connection,
                            context,
                            session);
                    set_mutable_value(
                        *this->system,
                        make_id(calc_queue_entity_id()),
                        swap_in_and_erase_type(queue),
                        mutable_value_source::WATCH);
                    boost::this_thread::sleep_for(boost::chrono::seconds(10));
                }
            }
            catch (background_job_canceled&)
            {
                // If someone wants to actually cancel this job, then let that
                // through...
                throw;
            }
            catch (...)
            {
                // If anything else happens, just try again.
                boost::this_thread::sleep_for(boost::chrono::seconds(1));
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "calculation queue query";
        return info;
    }

    framework_context context;
    web_session_data session;
};

void static
refresh_calc_queue_watch(
    alia__shared_ptr<background_execution_system> const& bg,
    mutable_entity_watch& watch)
{
    if (!watch.is_active() ||
        watch.entity_id() != make_id(calc_queue_entity_id()))
    {
        watch.watch(
            bg,
            make_id(calc_queue_entity_id()),
            [&]()
            {
                return new calc_queue_query_job(bg);
            });
    }
}

indirect_accessor<std::vector<calculation_queue_item> >
gui_calc_queue_status(gui_context& ctx)
{
    mutable_entity_watch* watch;
    get_cached_data(ctx, &watch);

    auto const& bg = ctx.gui_system->bg;

    if (is_refresh_pass(ctx))
    {
        refresh_calc_queue_watch(bg, *watch);
        // Request a refresh so we can pick up updates in this.
        // All this continuous refreshing due to background jobs needs to be
        // revisited at some point, but for now this is the only easy way to
        // pick up updates.
        request_refresh(ctx, 100);
    }

    return
        make_indirect(ctx,
            gui_mutable_entity_value<
                std::vector<calculation_queue_item>,
                calc_queue_entity_id
              >(ctx,
                in(calc_queue_entity_id()),
                [&](calc_queue_entity_id const& entity_id)
                {
                    assert(0); // Shouldn't get here because of the watch.
                }));
}

// RKS

rks_entry_entity_id
make_rks_entry_entity_id(string const& id)
{
    rks_entry_entity_id entity_id;
    entity_id.id = id;
    return entity_id;
}

struct rks_entry_request_job : background_web_job
{
    rks_entry_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        rks_entry_entity_id const& entry_id)
      : entry_id(entry_id)
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
        web_response rks_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                make_get_request(context.framework.api_url + "/rks/" +
                    entry_id.id + "?context=" + context.context_id, no_headers));
        check_in();
        auto entry = cradle::from_value<rks_entry>(parse_json_response(rks_response));
        set_mutable_value(*this->system, make_id(entry_id),
            swap_in_and_erase_type(entry), mutable_value_source::RETRIEVAL);
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "RKS entry request";
        return info;
    }

    rks_entry_entity_id entry_id;

    framework_context context;
    web_session_data session;
};

struct rks_write_entry_job : background_web_job
{
    rks_write_entry_job(
        alia__shared_ptr<background_execution_system> const& bg,
        rks_entry const& existing_entry,
        rks_entry const& new_entry)
      : existing_entry(existing_entry)
      , new_entry(new_entry)
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
        // Update the RKS entry.
        auto entry_update = as_rks_entry_update(new_entry);
        web_response rks_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                web_request(web_request_method::PUT,
                    context.framework.api_url + "/rks/" +
                    existing_entry.id + "?context=" + context.context_id,
                    value_to_json_blob(to_value(entry_update)),
                    make_header_list("Content-Type: application/json")));
        // And cache the response.
        auto entry =
            from_value<rks_entry>(parse_json_response(rks_response));
        check_in();
        set_mutable_value(*this->system,
            make_id(make_rks_entry_entity_id(existing_entry.id)),
            swap_in_and_erase_type(entry),
            mutable_value_source::RETRIEVAL);
        check_in();
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "RKS write job";
        return info;
    }

    rks_entry existing_entry;
    rks_entry new_entry;

    framework_context context;
    web_session_data session;
};

void static
set_gui_rks_entry(
    gui_context& ctx,
    rks_entry const& entry,
    rks_entry const& new_entry)
{
    // Ignore updates if they're the same as the current value.
    // (Updates to the same value are actually causing problems right now
    // (see AST-1429.))
    if (as_rks_entry_update(entry) != as_rks_entry_update(new_entry))
    {
        auto const& bg = ctx.gui_system->bg;
        // Refresh the corresponding mutable cache entity.
        refresh_mutable_value(*bg, make_id(make_rks_entry_entity_id(entry.id)),
            // We're dispatching our own job. If anyone else dispatched one,
            // it would just try to query the current value for this ID, which
            // would create a race condition.
            MUTABLE_REFRESH_NO_JOB_NEEDED);
        // Add a job to write the new entry to the RKS and update our copy of
        // it. (Thinknode sends back revision IDs, official timestamps, etc.)
        add_background_job(*bg,
            background_job_queue_type::WEB_WRITE,
            0, // no controller
            new rks_write_entry_job(bg, entry, new_entry));
    }
}

struct rks_entry_accessor : accessor<rks_entry>
{
    typedef id_change_minimization_accessor<gui_mutable_value_accessor<rks_entry> >
        cache_accessor;
    rks_entry_accessor() {}
    rks_entry_accessor(gui_context& ctx, cache_accessor const& getter)
      : ctx_(&ctx), getter_(getter)
    {}
    id_interface const& id() const { return getter_.id(); }
    bool is_gettable() const { return getter_.is_gettable(); }
    rks_entry const& get() const { return getter_.get(); }
    bool is_settable() const { return getter_.is_gettable(); }
    void set(rks_entry const& value) const
    { set_gui_rks_entry(*ctx_, getter_.get(), value); }
 private:
    gui_context* ctx_;
    cache_accessor getter_;
};

indirect_accessor<rks_entry>
gui_rks_entry(gui_context& ctx, app_context& app_ctx,
    rks_entry_resolution_data& data, accessor<string> const& entry_id)
{
    auto entity_id = gui_apply(ctx, make_rks_entry_entity_id, entry_id);
    auto cache_accessor =
        gui_mutable_entity_value<rks_entry,rks_entry_entity_id>(
            ctx, entity_id,
            [&](rks_entry_entity_id const& entity_id)
            {
                auto const& bg = ctx.gui_system->bg;
                add_background_job(*bg,
                    background_job_queue_type::WEB_READ,
                    0, // no controller
                    new rks_entry_request_job(bg, entity_id));
            },
            &data);
    return
        make_indirect(ctx,
            rks_entry_accessor(ctx, cache_accessor));
}

indirect_accessor<rks_entry>
gui_rks_entry(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& entry_id)
{
    rks_entry_resolution_data* data;
    get_data(ctx, &data);
    return gui_rks_entry(ctx, app_ctx, *data, entry_id);
}

struct rks_write_value_job : background_web_job
{
    rks_write_value_job(
        alia__shared_ptr<background_execution_system> const& bg,
        rks_entry const& existing_entry,
        dynamic_type_interface const* value_interface,
        untyped_immutable const& new_value)
      : existing_entry(existing_entry)
      , value_interface(value_interface)
      , new_value(new_value)
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
        // First post the new value to ISS to get the ISS ID.
        web_response iss_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                web_request(web_request_method::POST,
                    context.framework.api_url + "/iss" +
                    url_type_string(make_api_type_info(
                        value_interface->type_info())) +
                    "?context=" + context.context_id,
                    value_to_msgpack_blob(
                        value_interface->immutable_to_value(new_value)),
                    make_header_list("Content-Type: application/octet-stream")));
        auto iss_id =
            from_value<cradle::iss_response>(parse_json_response(iss_response)).id;
        // Now update the actual RKS entry.
        auto entry_update = as_rks_entry_update(existing_entry);
        entry_update.immutable = iss_id;
        web_response rks_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                web_request(web_request_method::PUT,
                    context.framework.api_url + "/rks/" +
                    existing_entry.id + "?context=" + context.context_id,
                    value_to_json_blob(to_value(entry_update)),
                    make_header_list("Content-Type: application/json")));
        // And cache the response.
        auto entry =
            from_value<rks_entry>(parse_json_response(rks_response));
        check_in();
        set_mutable_value(*this->system,
            make_id(make_rks_entry_entity_id(existing_entry.id)),
            swap_in_and_erase_type(entry),
            mutable_value_source::RETRIEVAL);
        check_in();
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "RKS write job";
        return info;
    }

    rks_entry existing_entry;
    dynamic_type_interface const* value_interface;
    untyped_immutable new_value;

    framework_context context;
    web_session_data session;
};

void set_gui_rks_entry_value(
    gui_context& ctx,
    rks_entry const& entry,
    dynamic_type_interface const& value_interface,
    untyped_immutable const& new_value)
{
    auto const& bg = ctx.gui_system->bg;
    // Refresh the entry's mutable cache entity.
    refresh_mutable_value(*bg, make_id(make_rks_entry_entity_id(entry.id)),
        // We're dispatching our own job. If anyone else dispatched one,
        // it would just try to query the current value for this ID, which
        // would create a race condition.
        MUTABLE_REFRESH_NO_JOB_NEEDED);
    // Add a job to write the new entry to the RKS and update our copy of it.
    // (Thinknode sends back revision IDs, official timestamps, etc.)
    add_background_job(*bg,
        background_job_queue_type::WEB_WRITE,
        0, // no controller
        new rks_write_value_job(bg, entry, &value_interface, new_value));
}

struct rks_search_job : background_web_job
{
    rks_search_job(
        alia__shared_ptr<background_execution_system> const& bg,
        rks_search_entity_id const& entity_id)
      : entity_id(entity_id)
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
        web_response raw_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                make_rks_search_request(context, entity_id.parameters));
        auto results =
            cradle::from_value<std::vector<rks_entry> >(
                parse_json_response(raw_response));

        // Since we want this to be cached in a way that's compatible with
        // individual RKS queries, we have to cache it in two layers.
        // (See gui_rks_search comments, below.)

        // Cache all the individual entries that we got back.
        for (auto const& result : results)
        {
            set_mutable_value(*this->system,
                make_id(make_rks_entry_entity_id(result.id)),
                erase_type(make_immutable(result)),
                mutable_value_source::RETRIEVAL);
        }
        // Cache the IDs of those entries as the result of the search.
        auto result_ids =
            map([ ](rks_entry const& entry) { return entry.id; }, results);
        set_mutable_value(*this->system, make_id(entity_id),
            swap_in_and_erase_type(result_ids),
            mutable_value_source::RETRIEVAL);
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "RKS entry request";
        return info;
    }

    rks_search_entity_id entity_id;

    framework_context context;
    web_session_data session;
};

rks_search_entity_id
make_rks_search_entity_id(rks_search_parameters const& parameters)
{
    rks_search_entity_id entity_id;
    entity_id.parameters = parameters;
    return entity_id;
}

indirect_accessor<std::vector<rks_entry> >
gui_rks_search(gui_context& ctx, app_context& app_ctx,
    accessor<rks_search_parameters> const& parameters)
{
    // We want the results of this query to use the same caching scheme that
    // queries for individual RKS entries use. (Otherwise, entries that you get
    // through a search could be out-of-sync with entries that you query
    // individually. Also, there'd be a lot of redundant queries.)
    //
    // So to make that happen, the cached value of an RKS search includes only
    // the IDs of the matching entries. The entries themselves are cached
    // individually using the normal scheme. Thus, to get the full results,
    // we have to do a two-layer query into the cache.

    // Get the IDs of the matching entries via the cache.
    auto result_ids =
        gui_mutable_entity_value<std::vector<string>,rks_search_entity_id>(
            ctx, gui_apply(ctx, make_rks_search_entity_id, parameters),
            [&](rks_search_entity_id const& entity_id)
            {
                // A job is needed, so dispatch one.
                // Note that this job will receive the full entries, not just
                // the IDs, so it'll cache both.
                auto const& bg = ctx.gui_system->bg;
                add_background_job(*bg,
                    background_job_queue_type::WEB_READ,
                    0, // no controller
                    new rks_search_job(bg, entity_id));
            });

    // Get the actual entries that match those IDs and collapse them into a
    // single vector. Note that since the search job fills in the actual
    // entries as well as the IDs, this should never need to dispatch new jobs
    // unless individual entries are refreshed independently.
    return
        gui_map<rks_entry>(ctx,
            [&app_ctx](gui_context& ctx, accessor<string> const& entry_id)
            { return gui_rks_entry(ctx, app_ctx, entry_id); },
            result_ids);
}

web_request static
make_new_rks_entry_request(
    string const& api_url,
    framework_context const& fc,
    string const& qualified_record,
    rks_entry_creation const& entry)
{
    return make_post_request(api_url + "/rks/" + qualified_record +
        "?context=" + fc.context_id, value_to_json_blob(to_value(entry)),
        make_header_list("Content-Type: application/json"));
}

gui_web_request_accessor<rks_entry>
gui_new_rks_entry(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_record,
    accessor<rks_entry_creation> const& entry)
{
    return
        gui_web_request<rks_entry>(ctx,
            gui_apply(ctx,
                make_new_rks_entry_request,
                get_api_url(ctx, app_ctx),
                get_framework_context(ctx, app_ctx),
                qualified_record,
                entry));
}

struct rks_lock_entry_job : background_web_job
{
    rks_lock_entry_job(
        alia__shared_ptr<background_execution_system> const& bg,
        rks_entry const& existing_entry,
        lock_type new_locked_type,
        bool deep_lock,
        dynamic_type_interface const* value_interface,
        optional<untyped_immutable> const& new_value)
        : existing_entry(existing_entry)
        , new_locked_type(new_locked_type)
        , deep_lock(deep_lock)
        , value_interface(value_interface)
        , new_value(new_value)
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
        value_map rks_revision;

        // Only post the plan to ISS when publishing
        if (new_value)
        {
            // First post the new value to ISS to get the ISS ID.
            web_response iss_response =
                perform_web_request(check_in, reporter, *this->connection, session,
                    web_request(web_request_method::POST,
                        context.framework.api_url + "/iss" +
                        url_type_string(make_api_type_info(
                            value_interface->type_info())) +
                        "?context=" + context.context_id,
                        value_to_msgpack_blob(
                            value_interface->immutable_to_value(get(new_value))),
                        make_header_list("Content-Type: application/octet-stream")));
            auto iss_id =
                from_value<cradle::iss_response>(parse_json_response(iss_response)).id;
            check_in();

            // Now update the actual RKS entry for the new treatment plan.
            auto entry_update = as_rks_entry_update(existing_entry);
            entry_update.immutable = iss_id;
            web_response rks_plan_response =
                perform_web_request(check_in, reporter, *this->connection, session,
                    web_request(web_request_method::PUT,
                        context.framework.api_url + "/rks/" +
                        existing_entry.id + "?context=" + context.context_id,
                        value_to_json_blob(to_value(entry_update)),
                        make_header_list("Content-Type: application/json")));

            auto plan_update_entry =
                from_value<rks_entry>(parse_json_response(rks_plan_response));
            check_in();
            rks_revision[value("revision")] = value(plan_update_entry.revision);
        }
        else
        {
            rks_revision[value("revision")] = value(existing_entry.revision);
        }

        // Now update the RKS entry for the locked flag
        string lock_state =
            (new_locked_type == lock_type::UNLOCKED) ? "unlock" : "lock";
        web_response rks_lock_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                web_request(web_request_method::PUT,
                    context.framework.api_url + "/rks/" +
                    existing_entry.id + "/" + lock_state + "?context=" +
                    context.context_id + "&deep=" + (deep_lock ? "true" : "false"),
                    value_to_json_blob(to_value(rks_revision)),
                    make_header_list("Content-Type: application/json")));

        // And cache the response.
        auto lock_entry =
            from_value<rks_entry>(parse_json_response(rks_lock_response));
        check_in();
        set_mutable_value(*this->system,
            make_id(make_rks_entry_entity_id(existing_entry.id)),
            swap_in_and_erase_type(lock_entry),
            mutable_value_source::RETRIEVAL);
        check_in();
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "RKS write lock/unlock job";
        return info;
    }

    rks_entry existing_entry;
    lock_type new_locked_type;
    bool deep_lock;
    dynamic_type_interface const* value_interface;
    optional<untyped_immutable> new_value;

    framework_context context;
    web_session_data session;
};

void
set_gui_rks_lock_entry(
    gui_context& ctx,
    rks_entry const& entry,
    lock_type new_locked_type,
    bool deep_lock,
    dynamic_type_interface const& value_interface,
    optional<untyped_immutable> const& new_value)
{
    auto const& bg = ctx.gui_system->bg;
    // Refresh the corresponding the mutable cache entity.
    refresh_mutable_value(*bg, make_id(make_rks_entry_entity_id(entry.id)),
        // We're dispatching our own job. If anyone else dispatched one,
        // it would just try to query the current value for this ID, which
        // would create a race condition.
        MUTABLE_REFRESH_NO_JOB_NEEDED);
    // Add a job to write the new entry to the RKS and update our copy of it.
    // (Thinknode sends back revision IDs, official timestamps, etc.)
    add_background_job(*bg,
        background_job_queue_type::WEB_WRITE,
        0, // no controller
        new rks_lock_entry_job(
            bg, entry,
            new_locked_type, deep_lock, &value_interface, new_value));
}

// The following is all responsible for implementing gui_rks_entry_id_by_name.

struct entry_id_request_parameters
{
    framework_context context;
    string qualified_record;
    optional<string> parent_id;
    string name;
    string default_immutable_id;
};

entry_id_request_parameters static
make_entry_id_request_parameters(
    framework_context const& context,
    string const& qualified_record,
    optional<string> const& parent_id,
    string const& name,
    string const& default_immutable_id)
{
    entry_id_request_parameters request;
    request.context = context;
    request.qualified_record = qualified_record;
    request.parent_id = parent_id;
    request.name = name;
    request.default_immutable_id = default_immutable_id;
    return request;
}

struct background_entry_id_request_job : background_web_job
{
    background_entry_id_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        id_interface const& id,
        entry_id_request_parameters const& request)
      : request(request)
    {
        this->system = bg;
        this->id.store(id);
    }

    bool inputs_ready()
    {
        background_authentication_status status;
        get_authentication_result(*this->system, &status, &this->session);
        return status.state == background_authentication_state::SUCCEEDED;
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        string entry_id;
        // First, attempt to search for the ID.
        {
            rks_search_parameters search;
            search.parent = this->request.parent_id;
            search.name = this->request.name;
            search.record = this->request.qualified_record;
            auto search_request =
                make_rks_search_request(this->request.context, search);
            auto entries =
                from_value<std::vector<rks_entry> >(
                    parse_json_response(
                        perform_web_request(
                            check_in, reporter, *this->connection,
                            this->session, search_request)));
            if (entries.size() > 1)
            {
                // That combination of request parameters should be unique, so
                // this should never happen.
                throw exception("duplicate RKS names");
            }
            if (!entries.empty())
            {
                entry_id = entries.front().id;
                goto found_entry_id;
            }
        }
        // If we didn't find it, create it.
        {
            auto creation_request =
                make_new_rks_entry_request(
                    this->request.context.framework.api_url,
                    this->request.context,
                    this->request.qualified_record,
                    rks_entry_creation(
                        this->request.name,
                        this->request.parent_id,
                        this->request.default_immutable_id,
                        true));
            auto entry =
                from_value<rks_entry>(
                    parse_json_response(
                        perform_web_request(
                            check_in, reporter, *this->connection,
                            this->session, creation_request)));
            entry_id = entry.id;
        }
      found_entry_id:
        check_in();
        auto immutable = make_immutable(entry_id);
        set_cached_data(*this->system, this->id.get(), erase_type(immutable));
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description =
            "RKS entry ID lookup\n" +
            request.qualified_record + "\n" +
            to_string(request.parent_id) + "\n" +
            request.name;
        return info;
    }

    owned_id id;
    entry_id_request_parameters request;

    web_session_data session;
};

gui_web_request_accessor<string>
gui_rks_entry_id_by_name(
    gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_record,
    accessor<optional<string> > const& parent_id,
    accessor<string> const& name,
    accessor<string> const& default_immutable_id)
{
    typed_gui_web_request_data<string>* data;
    get_data(ctx, &data);
    auto request =
        gui_apply(ctx,
            make_entry_id_request_parameters,
            get_framework_context(ctx, app_ctx),
            qualified_record,
            parent_id,
            name,
            default_immutable_id);
    if (is_refresh_pass(ctx))
    {
        static dynamic_type_implementation<string> result_interface;
        bool changed =
            update_generic_gui_web_request(ctx, data->untyped, request,
                [&]() // create_background_job
                {
                    add_untyped_background_job(
                        data->untyped.ptr,
                        get_background_system(ctx),
                        // Writing might be involved.
                        background_job_queue_type::WEB_WRITE,
                        new background_entry_id_request_job(
                                ctx.gui_system->bg,
                                request.id(),
                                get(request)));
                });
        if (changed)
        {
            if (data->untyped.ptr.is_ready())
            {
                cast_immutable_value(&data->result,
                    data->untyped.ptr.data().ptr.get());
            }
            else
                data->result = 0;
        }
    }
    return gui_web_request_accessor<string>(*data);
}

// RKS long polling

struct rks_entry_long_poll_job : background_web_job
{
    rks_entry_long_poll_job(
        alia__shared_ptr<background_execution_system> const& bg,
        string const& entry_id)
      : entry_id(entry_id)
    {
        this->system = bg;
    }

    bool inputs_ready()
    {
        return true;
    }

    void execute(check_in_interface& check_in,
        progress_reporter_interface& reporter)
    {
        // Currently, the system assumes that long-polling jobs don't fail, so
        // this job tries to be as robust as possible.
        while (true)
        {
            try
            {
                framework_context context;
                web_session_data session;
                // If we can't get the context or session yet, just wait a bit
                // and try again.
                if (!get_session_and_context(*this->system, &session,
                        &context))
                {
                    boost::this_thread::sleep_for(boost::chrono::seconds(1));
                    continue;
                }

                // Get the initial revision.
                web_response rks_response =
                    perform_web_request(
                        check_in,
                        reporter,
                        *this->connection,
                        session,
                        make_get_request(
                            context.framework.api_url + "/rks/" +
                                entry_id + "?context=" + context.context_id,
                            no_headers));
                check_in();
                auto existing_entry =
                    cradle::from_value<rks_entry>(
                        parse_json_response(rks_response));
                // Update the cached value, just in case we got something new
                // here.
                set_mutable_value(*this->system,
                    make_id(rks_entry_entity_id(entry_id)),
                    erase_type(make_immutable(existing_entry)),
                    mutable_value_source::WATCH);

                // Long poll for changes.
                while (true)
                {
                    check_in();
                    web_response rks_response =
                        perform_web_request(
                            check_in,
                            reporter,
                            *this->connection,
                            session,
                            make_get_request(
                                context.framework.api_url +
                                    "/rks/" + entry_id +
                                    "?context=" + context.context_id +
                                    "&revision=" + existing_entry.revision +
                                    "&timeout=120",
                                no_headers));
                    auto new_entry =
                        cradle::from_value<rks_entry>(
                            parse_json_response(rks_response));
                    if (new_entry.revision != existing_entry.revision)
                    {
                        set_mutable_value(*this->system,
                            make_id(rks_entry_entity_id(entry_id)),
                            erase_type(make_immutable(new_entry)),
                            mutable_value_source::WATCH);
                        existing_entry = new_entry;
                    }
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
                }
            }
            catch (background_job_canceled&)
            {
                // If someone wants to actually cancel this job, then let that
                // through...
                throw;
            }
            catch (...)
            {
                // If anything else happens, just try again.
                boost::this_thread::sleep_for(boost::chrono::seconds(1));
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "long polling job for RKS entry " + entry_id;
        return info;
    }

    string entry_id;

    framework_context context;
    web_session_data session;
};

void static
refresh_rks_entry_watch(
    alia__shared_ptr<background_execution_system> const& bg,
    mutable_entity_watch& watch,
    accessor<rks_entry_entity_id> const& entity_id)
{
    // If :entity_id isn't gettable, we don't know what to watch, so do
    // nothing. - We could reset :watch here, but I'm not sure if that's
    // really better. It could be that when :entity_id becomes gettable again,
    // it's going to refer to the same entity that the :watch is already
    // watching, so we'd end up wanting the same job back again.
    if (!is_gettable(entity_id))
        return;

    // If :watch is already watching :entity_id, there's nothing to do.
    if (watch.is_active() && watch.entity_id() == make_id(get(entity_id)))
        return;

    // If we get here, we know which entity we want to watch, but it's not the
    // one we're actually watching, so correct that.
    watch.watch(
        bg,
        make_id(get(entity_id)),
        [&]()
        {
            return new rks_entry_long_poll_job(bg, get(entity_id).id);
        });
}

void
watch_rks_entry(
    gui_context& ctx, app_context& app_ctx,
    accessor<string> const& entry_id)
{
    auto entity_id = gui_apply(ctx, make_rks_entry_entity_id, entry_id);

    mutable_entity_watch* watch;
    get_cached_data(ctx, &watch);

    auto const& bg = ctx.gui_system->bg;

    if (is_refresh_pass(ctx))
    {
        refresh_rks_entry_watch(bg, *watch, entity_id);
        // Request a refresh so we can pick up updates in this.
        // All this continuous refreshing due to background jobs needs to be
        // revisited at some point, but for now this is the only easy way to
        // pick up updates.
        request_refresh(ctx, 100);
    }
}

// ISS

// Request for data to be posted to the ISS.
// The data must already be formatted into a JSON blob.
gui_web_request_accessor<iss_response>
gui_post_iss_msgpack_blob(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_type, accessor<blob> const& data)
{
    return
        gui_web_request<iss_response>(ctx,
            gui_apply(ctx,
                make_iss_post_request,
                get_api_url(ctx, app_ctx),
                qualified_type,
                data,
                get_framework_context(ctx, app_ctx)));
}

// RKS

gui_web_request_accessor<std::vector<rks_entry>>
gui_get_history_data(gui_context& ctx,
    accessor<framework_context> const& fc, accessor<string> const& id)
{
    auto api_url = _field(_field(fc, framework), api_url);
    auto ctx_id = _field(fc, context_id);

    auto url =
        minimize_id_changes(ctx,
            printf(ctx, "%s/rks/%s/history?context=%s", api_url, id, ctx_id));

    return
        gui_web_request<std::vector<rks_entry>>(ctx,
            gui_apply(ctx,
                make_get_request,
                url,
                in(no_headers)));
}

}
