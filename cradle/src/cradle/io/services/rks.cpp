#include <cradle/io/services/rks.hpp>

#include <cradle/io/generic_io.hpp>
#include <cradle/io/services/iss.hpp>

namespace cradle {

web_request
make_rks_search_request(
    framework_context const& fc,
    rks_search_parameters const& parameters)
{
    auto url = fc.framework.api_url + "/rks" + "?context=" + fc.context_id;
    if (parameters.parent)
        url += "&parent=" + get(parameters.parent);
    if (parameters.depth)
        url += "&depth=" + to_string(get(parameters.depth));
    if (parameters.recursive)
        url += "&recursive=true";
    if (parameters.name)
        url += "&name=" + get(parameters.name);
    if (parameters.record)
        url += "&record=" + get(parameters.record);
    if (parameters.inactive)
        url += "&inactive=true";
    return make_get_request(url, no_headers);
}

rks_search_parameters
make_rks_descendent_search(string const& entry_id)
{
    rks_search_parameters parameters;
    parameters.parent = entry_id;
    parameters.recursive = true;
    return parameters;
}

std::vector<rks_entry>
filter_rks_entries_by_record(
    std::vector<rks_entry> const& entries,
    rks_record const& record)
{
    std::vector<rks_entry> matching;
    for (auto const& entry : entries)
    {
        if (entry.record == record)
            matching.push_back(entry);
    }
    return matching;
}

std::vector<rks_entry>
filter_rks_entries_by_record_and_parent(
    std::vector<rks_entry> const& entries,
    rks_record const& record,
    optional<string> const& parent)
{
    std::vector<rks_entry> matching;
    for (auto const& entry : entries)
    {
        if (entry.record == record && as_optional(entry.parent) == parent)
            matching.push_back(entry);
    }
    return matching;
}

optional<activity_record>
resolve_first_activity(std::vector<rks_entry> const& records)
{
    optional<activity_record> last_activity;
    for (auto const& r : records)
    {
        if (!last_activity || get(last_activity).at > r.modified_at)
        {
            activity_record record;
            record.at = r.modified_at;
            record.by = r.modified_by;
            last_activity = record;
        }
    }
    return last_activity;
}

optional<activity_record>
resolve_last_activity(std::vector<rks_entry> const& records)
{
    optional<activity_record> last_activity;
    for (auto const& r : records)
    {
        if (!last_activity || get(last_activity).at < r.modified_at)
        {
            activity_record record;
            record.at = r.modified_at;
            record.by = r.modified_by;
            last_activity = record;
        }
    }
    return last_activity;
}

string
create_rks_entry(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    rks_entry_creation const& rks_entry_data,
    string const& qualified_record)
{
    web_response rks_response =
        perform_web_request(
            check_in,
            reporter,
            connection,
            session,
            web_request(
                web_request_method::POST,
                context.framework.api_url + "/rks/" + qualified_record +
                "?context=" + context.context_id,
                value_to_json_blob(to_value(rks_entry_data)),
                make_header_list("Content-Type: application/json")));
    auto rks_id =
        from_value<cradle::iss_response>(parse_json_response(rks_response)).id;
    return rks_id;
}

web_response
update_rks_entry(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    rks_entry_update const& rks_entry_data,
    string const& rks_id)
{
    web_response rks_response_update =
        perform_web_request(
            check_in,
            reporter,
            connection,
            session,
            web_request(
                web_request_method::PUT,
                context.framework.api_url + "/rks/" + rks_id +
                "?context=" + context.context_id,
                value_to_json_blob(to_value(rks_entry_data)),
                make_header_list("Content-Type: application/json")));

    return rks_response_update;
}

}
