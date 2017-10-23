#include <cradle/gui/internals.hpp>

namespace cradle {

void initialize_gui_system(gui_system* system,
    file_path const& cache_dir, string const& key_prefix, int64_t cache_size,
    file_path const& web_certificate_file)
{
    set_web_certificate_file(web_certificate_file);

    system->bg.reset(new background_execution_system);

    system->disk_cache.reset(new disk_cache);
    initialize(*system->disk_cache, cache_dir, key_prefix, cache_size);

    set_disk_cache(*system->bg, system->disk_cache);

    initialize_background_request_system(system->requests, system->bg);
}

gui_system::~gui_system()
{
}

}
