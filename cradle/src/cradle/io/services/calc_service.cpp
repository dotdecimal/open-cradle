#include <cradle/io/services/calc_service.hpp>
#include <cradle/io/services/core_services.hpp>
#include <cradle/io/web_io.hpp>
#include <cradle/io/services/calc_internals.hpp>
#include <cradle/io/services/iss.hpp>
#include <cradle/io/generic_io.hpp>

#ifdef WIN32
#include <windows.h>
#endif

namespace cradle {

string
make_calc_status_long_polling_url(framework_context const& context, string const& id)
{
    return context.framework.api_url + "/calc/" + id + "/status?status=completed&progress=1&timeout=30&context=" +
        context.context_id;
}

string
perform_remote_calculation(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation)
{
    auto uid =
        request_remote_calculation(check_in, connection, context,
            session, calculation);
    wait_for_remote_calculation(check_in, reporter, connection, context,
        session, uid);
    return uid;
}

string
request_remote_calculation(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation)
{
    // Uncomment this to write the request out to a file.
    // (This can be useful for debugging.)
    //{
    //    std::ofstream f("request.json");
    //    f << value_to_json(to_value(calculation));
    //}
    null_progress_reporter null_reporter;
    auto data = value_to_msgpack_blob(to_value(calculation));
    // Post the calculation to ISS using message pack
    auto object_id =
        post_iss_data(
            check_in,
            null_reporter,
            connection,
            session,
            context,
            data,
            "dynamic");
    // Make the calc request using the resulting ISS object id
    web_request initial_request(web_request_method::POST,
        context.framework.api_url + "/calc/" + object_id + "?context=" +
        context.context_id,
        blob(),
        no_headers);
    auto response =
        perform_web_request(check_in, null_reporter, connection, session,
            initial_request);
    return
        from_value<calculation_request_response>(
            parse_json_response(response)).id;
}

void
wait_for_remote_calculation(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    string const& uid)
{
    while (1)
    {
        auto status_query =
                make_get_request(
                    make_calc_status_long_polling_url(context, uid),
                    no_headers);
        null_progress_reporter null_reporter;
        auto response =
            perform_web_request(check_in, null_reporter, connection, session,
                status_query);

        calculation_status status;
        from_value(&status, parse_json_response(response));

        switch (status.type)
        {
         case calculation_status_type::CALCULATING:
            reporter(as_calculating(status).progress);
            break;
         case calculation_status_type::FAILED:
          {
            auto const& failure = as_failed(status);
            throw calculation_failure(failure.code, failure.message);
          }
         case calculation_status_type::COMPLETED:
            return;
         case calculation_status_type::CANCELED:
            return;
        }
    }
}

string
make_calc_result_url(framework_context const& context, string const& id)
{
    return context.framework.api_url + "/iss/" + id + "?context=" +
        context.context_id;
}

optional<string>
request_dry_run_calculation(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session,
    calculation_request const& calculation)
{
    // Uncomment this to write the request out to a file.
    // (This can be useful for debugging.)
    //{
    //    std::ofstream f("dry_run.json");
    //    f << value_to_json(to_value(calculation));
    //}
    null_progress_reporter null_reporter;
    auto data = value_to_msgpack_blob(to_value(calculation));
    // Post the calculation to ISS using message pack
    auto object_id =
        post_iss_data(
            check_in,
            null_reporter,
            connection,
            session,
            context,
            data,
            "dynamic");
    // Make the calc request using the resulting ISS object id
    web_request initial_request(web_request_method::POST,
        context.framework.api_url + "/calc/" + object_id + "?context=" +
        context.context_id + "&dry_run=true",
        blob(),
        no_headers);
    auto response =
        perform_web_request(check_in, null_reporter, connection, session,
            initial_request);
    return from_value<optional<string> >(parse_json_response(response));
}

std::vector<calculation_queue_item>
query_calculation_queue(
    check_in_interface& check_in,
    web_connection& connection,
    framework_context const& context,
    web_session_data const& session)
{
    null_progress_reporter reporter;
    return
        from_value<std::vector<calculation_queue_item> >(
            parse_json_response(
                perform_web_request(
                    check_in,
                    reporter,
                    connection,
                    session,
                    make_get_request(
                        context.framework.api_url + "/calc/queue",
                        no_headers))));
}

}
