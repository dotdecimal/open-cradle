#ifndef CRADLE_IO_SERVICES_CORE_SERVICES_HPP
#define CRADLE_IO_SERVICES_CORE_SERVICES_HPP

#include <cradle/common.hpp>
#include <cradle/date_time.hpp>

namespace cradle {

api(struct internal)
struct framework_usage_info
{
    string api_url;
    string realm_id;
};

api(struct internal)
struct session_info
{
    string id;
    bool owner;
    string username;
    string domain;
    time created_at;
    time expires_at;
};

string construct_session_info_request_url(string const& api_url);

api(struct internal)
struct context_request_parameters
{
    string app_account;
    string app_name;
    string app_version;
};

string construct_context_request_url(
    framework_usage_info const& framework,
    context_request_parameters const& parameters);

string construct_realm_app_request_url(
    framework_usage_info const& framework);

api(struct internal)
struct context_response
{
    string id;
};

api(struct internal)
struct realm_app_response
{
    string account;
    string app;
    string version;
    string status;
};

api(struct internal)
struct framework_context
{
    framework_usage_info framework;
    string context_id;
};

api(enum internal)
enum class service_identifier
{
    CALC,
    ISS
};

api(struct internal)
struct realm_version_dependency
{
    string account;
    string app;
    string version;
};

api(struct internal)
struct realm_version
{
    string account;
    string app;
    string version;
    string status;
    std::vector<realm_version_dependency> dependencies;
};

api(struct internal)
struct realm
{
    string name;
    string description;
    string bucket;
    bool development;
    time created_at;
};

string construct_realm_info_request_url(
    string const& api_url, string const& realm_id);

api(struct internal)
struct user_info
{
    string username;
    string name;
    string email;
    bool active;
    time created_at;
    time updated_at;
};

string construct_user_info_request_url(
    string const& api_url, string const& username);

api(struct internal)
struct user_reference
{
    string username;
    string name;
};

}

#endif
