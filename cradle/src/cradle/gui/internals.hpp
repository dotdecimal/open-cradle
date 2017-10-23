#ifndef CRADLE_GUI_INTERNALS_HPP
#define CRADLE_GUI_INTERNALS_HPP

#include <cradle/background/api.hpp>
#include <cradle/background/requests.hpp>
#include <cradle/background/system.hpp>
#include <cradle/disk_cache.hpp>
#include <cradle/gui/common.hpp>
#include <cradle/gui/notifications.hpp>
#include <cradle/io/services/core_services.hpp>

namespace cradle {

struct gui_system
{
    alia__shared_ptr<background_execution_system> bg;
    alia__shared_ptr<cradle::disk_cache> disk_cache;
    notification_system notifications;
    background_request_system requests;
    std::vector<untyped_request> request_list;
    state<optional<cradle::framework_context> > framework_context;

    ~gui_system();
};

void initialize_gui_system(gui_system* system,
    file_path const& cache_dir, string const& key_prefix, int64_t cache_size,
    file_path const& web_certificate_file);

static inline background_execution_system&
get_background_system(gui_context& ctx)
{
    return *ctx.gui_system->bg;
}

}

#endif
