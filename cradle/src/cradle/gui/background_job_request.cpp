#include <cradle/gui/background_job_request.hpp>

#include <cradle/background/internals.hpp>
#include <cradle/gui/internals.hpp>
#include <cradle/gui/app/internals.hpp>

#include <boost/thread/thread.hpp>

namespace cradle {


bool update_general_background_job(
    gui_context& ctx, background_job_data& data,
    id_interface const& id,
    boost::function<background_job_interface*()> const& create_background_job)
{
    assert(is_refresh_pass(ctx));

    bool changed = false;

    // If the request is gettable, but the pointer isn't initialized or
    // doesn't have the same ID, reset it to the new request.
    if (!data.ptr.is_initialized() ||
        data.ptr.key() != id)
    {
        auto const& bg = ctx.gui_system->bg;
        data.ptr.reset(*bg, id);
        add_untyped_background_job(data.ptr, *bg,
            // I think this is a legacy interface, and we don't know what the
            // job is going to do, so mark it as a write job to be safe.
            background_job_queue_type::WEB_WRITE,
            create_background_job());
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

}