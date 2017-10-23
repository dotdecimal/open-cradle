#ifndef CRADLE_IO_SERVICES_CALC_SERVICE_HPP
#define CRADLE_IO_SERVICES_CALC_SERVICE_HPP

#include <cradle/date_time.hpp>
#include <cradle/io/calc_requests.hpp>

namespace cradle {

api(struct internal)
struct calculation_request_response
{
    string id;
};

struct web_connection;
struct web_session_data;

struct calculation_failure : exception
{
    calculation_failure(string const& code, optional<string> const& message)
      : code_(new string(code)), message_(new optional<string>(message))
      , exception("calculation failed: " +
            (message ? get(message) : "no message"))
    {}
    ~calculation_failure() throw() {}
 private:
    alia__shared_ptr<string> code_;
    alia__shared_ptr<optional<string> > message_;
};

struct framework_context;

// POST a calculation request to the calculation service and wait for it to
// complete. This provides a monitoring interface so that it can report the
// progress of the calculation, but due to implementation details, it
// only checks in at about the same rate that it reports progress.
// If the calculation fails for any reason, an exception is thrown.
// If the calculation is successful, the return value is the ID of the result
// within the caching service.
string
perform_remote_calculation(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation);

// The above can be broken down into calls to the following two functions.

// Request a calculation and return its ID.
string
request_remote_calculation(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation);

// Wait for a calculation to be ready, checking in and reporting progress
// along the way.
void
wait_for_remote_calculation(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    string const& uid);

// Construct the URL for a calculation result.
string
make_calc_result_url(framework_context const& context, string const& id);

// Perform a dry run calculation request.
optional<string>
request_dry_run_calculation(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation);

// CALCULATION QUEUE

api(enum internal)
enum class calculation_priority
{
    HIGH,
    MEDIUM,
    LOW
};

api(enum internal)
enum class calculation_queue_item_status
{
    DEFERRED,
    READY,
    RUNNING
};

api(struct internal)
struct calculation_queue_item
{
    string id;
    calculation_queue_item_status status;
    calculation_priority priority;
    cradle::time issued_at;
    cradle::time queued_at;
    omissible<cradle::time> started_at;
};

// Get the list of items in the Thinknode calculation queue.
std::vector<calculation_queue_item>
query_calculation_queue(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session);

}

#endif
