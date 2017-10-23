#ifndef CRADLE_IO_SERVICES_CALC_INTERNALS_HPP
#define CRADLE_IO_SERVICES_CALC_INTERNALS_HPP

#include <cradle/common.hpp>

namespace cradle {

api(enum internal)
enum class calculation_queue_type
{
    PENDING,
    READY
};

api(struct internal)
struct calculation_calculating_status
{
    float progress;
    /// Thinknode seems to be misbehaving on the messages right now, so this
    /// is disabled.
    ///string message;
};

api(struct internal)
struct calculation_uploading_status
{
    float progress;
    /// Thinknode seems to be misbehaving on this as well.
    ///string message;
};

api(struct internal)
struct calculation_failure_status
{
    string error, code, message;
};

api(union internal)
union calculation_status
{
    nil_type waiting;
    calculation_queue_type queued;
    nil_type generating;
    calculation_calculating_status calculating;
    calculation_uploading_status uploading;
    nil_type completed;
    calculation_failure_status failed;
    nil_type canceled;
};

}

#endif
