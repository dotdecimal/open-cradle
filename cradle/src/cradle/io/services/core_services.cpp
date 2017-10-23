#include <cradle/io/services/core_services.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace cradle {

string construct_session_info_request_url(string const& api_url)
{
    return api_url + "/cas/session";
}

string construct_context_request_url(
    framework_usage_info const& framework,
    context_request_parameters const& parameters)
{
    return framework.api_url + "/iam/realms/" + framework.realm_id +
        "/context?account=" + parameters.app_account +
        "&app=" + parameters.app_name +
        "&version=" + parameters.app_version;
}

string construct_realm_app_request_url(
    framework_usage_info const& framework)
{
    return framework.api_url + "/iam/realms/" + framework.realm_id + "/versions";
}


string construct_realm_info_request_url(
    string const& api_url, string const& realm_id)
{
    return api_url + "/iam/realms/" + realm_id;
}

string construct_user_info_request_url(
    string const& api_url, string const& username)
{
    return api_url + "/iam/users/" + username;
}

}
