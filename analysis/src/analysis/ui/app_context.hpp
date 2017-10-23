#ifndef ANALYSIS_UI_APP_CONTEXT_HPP
#define ANALYSIS_UI_APP_CONTEXT_HPP

#include <cradle/gui/app/interface.hpp>

#include <analysis/ui/common.hpp>
#include <analysis/session.hpp>

namespace analysis {

// This is the app context for the analysis app. It extends the CRADLE app
// context with info that's specific to the analysis app but not specific to a
// particular context within the analysis app.
struct app_context : cradle::app_context
{
    indirect_accessor<session_info> session_state;
};

app_context&
get_app_context(
    gui_context& ctx,
    cradle::app_context& cradle_app_ctx);

indirect_accessor<session_info> static inline
get_session_state(app_context& app_ctx)
{ return app_ctx.session_state; }

}

#endif
