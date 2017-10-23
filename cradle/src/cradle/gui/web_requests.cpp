#include <cradle/gui/web_requests.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/app/internals.hpp>

namespace cradle {

bool update_gui_web_request(
    gui_context& ctx, gui_web_request_data& data,
    accessor<web_request> const& request,
    dynamic_type_interface const& result_interface)
{
    return
        update_generic_gui_web_request(
            ctx, data, request,
            [&]() // create_background_job
            {
                add_untyped_background_job(
                    data.ptr,
                    get_background_system(ctx),
                    get(request).method == web_request_method::GET ?
                        background_job_queue_type::WEB_READ :
                        background_job_queue_type::WEB_WRITE,
                    new background_web_request_job(ctx.gui_system->bg,
                            request.id(), get(request), &result_interface));
            });
}

bool update_generic_gui_web_request(
    gui_context& ctx, gui_web_request_data& data,
    untyped_accessor_base const& request,
    boost::function<void()> const& create_background_job)
{
    assert(is_refresh_pass(ctx));

    bool changed = false;

    if (!request.is_gettable())
    {
        // If the request isn't gettable but the pointer is initialized,
        // reset the pointer.
        if (data.ptr.is_initialized())
        {
            data.ptr.reset();
            inc_version(data.abbreviated_identity);
            changed = true;
        }
        // And since we don't have the request yet, there's nothing else to be
        // done.
        request_refresh(ctx, 1);
        return changed;
    }

    // If the request is gettable, but the pointer isn't initialized or
    // doesn't have the same ID, reset it to the new request.
    if (request.is_gettable() && (!data.ptr.is_initialized() ||
        data.ptr.key() != request.id()))
    {
        auto const& bg = ctx.gui_system->bg;
        data.ptr.reset(*bg, request.id());
        create_background_job();
        inc_version(data.abbreviated_identity);
        changed = true;
    }

    // If we already have the result, we're done.
    if (data.ptr.is_ready())
        return changed;

    // Otherwise, update to bring in changes from the background.
    data.ptr.update();

    // Check again to see if that made the pointer ready.
    if (data.ptr.is_ready())
        changed = true;

    request_refresh(ctx, 1);

    return changed;
}

get_request_entity_id static
make_get_request_entity_id(string const& url,
    std::vector<string> const& headers)
{
    get_request_entity_id entity_id;
    entity_id.url = url;
    entity_id.headers = headers;
    return entity_id;
}

struct http_get_request_job : background_web_job
{
    http_get_request_job(
        alia__shared_ptr<background_execution_system> const& bg,
        get_request_entity_id const& entity_id)
      : entity_id(entity_id)
    {
        this->system = bg;
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
        web_response raw_response =
            perform_web_request(check_in, reporter, *this->connection, session,
                make_get_request(entity_id.url, entity_id.headers));
        check_in();
        auto result = parse_json_response(raw_response);
        set_mutable_value(*this->system, make_id(entity_id),
            swap_in_and_erase_type(result),
            mutable_value_source::RETRIEVAL);
    }

    background_job_info get_info() const
    {
        background_job_info info;
        info.description = "Get data request";
        return info;
    }

    get_request_entity_id entity_id;

    web_session_data session;
};

id_change_minimization_accessor<gui_mutable_value_accessor<value> >
untyped_gui_get_request(gui_context& ctx, accessor<string> const& url,
    accessor<std::vector<string> > const& headers)
{
    return
        gui_mutable_entity_value<value,get_request_entity_id>(
            ctx,
            gui_apply(ctx, make_get_request_entity_id, url, headers),
            [&](get_request_entity_id const& entity_id)
            {
                auto const& bg = ctx.gui_system->bg;
                add_background_job(*bg,
                    background_job_queue_type::WEB_READ,
                    0, // no controller
                    new http_get_request_job(bg, entity_id));
            });
}

indirect_accessor<string>
get_api_url(gui_context& ctx, app_context& app_ctx)
{
    return make_indirect(ctx, in_ptr(&app_ctx.instance->api_url));
}

}
