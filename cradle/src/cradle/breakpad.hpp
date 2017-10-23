#ifndef CRADLE_BREAKPAD_HPP
#define CRADLE_BREAKPAD_HPP

#include <cradle/common.hpp>
#include <cradle/io/forward.hpp>

namespace cradle {

// A crash_reporting_context enables crash report generation for an
// application as long as it's active.
struct crash_reporting_implementation;
struct crash_reporting_context
{
    crash_reporting_context() : impl_(0) {}
    ~crash_reporting_context() { end(); }

    // Activate the context.
    // crash_dir is the directory where reports are written.
    // app_id and version will appear in the crash report.
    void begin(file_path const& crash_dir, string const& app_id,
        string const& version);

    void end();

 private:
    crash_reporting_implementation* impl_;
};

}

#endif
