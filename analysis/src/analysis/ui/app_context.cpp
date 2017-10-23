#include <analysis/ui/app_context.hpp>

#include <cradle/gui/services.hpp>

namespace analysis {

void static
precache_app_data(gui_context &ctx, app_context &app_ctx)
{
}

void static
initialize_app_context(
    app_context* app_ctx,
    gui_context& ctx,
    cradle::app_context& cradle_app_ctx)
{
    app_ctx->session_state =
        make_indirect(ctx, get_state(ctx, session_info()));

    precache_app_data(ctx, *app_ctx);
}

app_context&
get_app_context(
    gui_context& ctx,
    cradle::app_context& cradle_app_ctx)
{
    app_context* app_ctx;
    get_data(ctx, &app_ctx);
    if (is_refresh_pass(ctx))
        static_cast<cradle::app_context&>(*app_ctx) = cradle_app_ctx;

    initialize_app_context(app_ctx, ctx, cradle_app_ctx);

    return *app_ctx;
}

}
