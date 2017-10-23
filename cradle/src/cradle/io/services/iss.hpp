#ifndef CRADLE_IO_SERVICES_ISS_HPP
#define CRADLE_IO_SERVICES_ISS_HPP

#include <cradle/common.hpp>
#include <cradle/api.hpp>

// types and utilities for communicating with the Immutable Storage Service

// forward declarations
namespace cradle {
    struct web_request;
    struct framework_context;
    struct web_connection;
    struct web_session_data;
}

namespace cradle {

// Given an api_type_info, this produces the corresponding ISS URL type string.
string url_type_string(api_type_info const& schema);

api(struct internal)
struct iss_response
{
    string id;
};

web_request
make_iss_post_request(
    string const& api_url,
    string const& qualified_type,
    blob const& data,
    framework_context const& fc);

string
post_iss_data(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    blob const& data,
    string const& qualified_type);

}

#endif
