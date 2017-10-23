#ifndef CRADLE_IO_SERVICES_RKS_HPP
#define CRADLE_IO_SERVICES_RKS_HPP

#include <cradle/io/services/core_services.hpp>
#include <cradle/io/services/iss.hpp>
#include <cradle/io/web_io.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// various types and utilities for communicating with the RKS

namespace cradle {

// info required to create a new RKS entry
api(struct internal)
struct rks_entry_creation
{
    string name;
    omissible<string> parent;
    string immutable;
    bool active;
};

// info required to update an existing RKS entry
api(struct internal)
struct rks_entry_update : rks_entry_creation
{
    string revision;
};

// info reported by thinknode about an RKS record
api(struct internal)
struct rks_record
{
    string account;
    string app;
    string name;
};

api(enum internal)
enum class lock_type
{
    // RKS entry is unlocked (unpublished)
    UNLOCKED,
    // RKS entry is locked (published)
    SHALLOW,
    DEEP
};

// info reported by thinknode about an RKS entry
api(struct internal)
struct rks_entry : rks_entry_update
{
    string id;
    rks_record record;
    time modified_at;
    user_reference modified_by;
    lock_type lock;
};

// This defines the available parameters for RKS search queries.
// (This isn't actually a thinknode data structure, but it matches the URL
// query parameters for RKS searches.)
api(struct internal)
struct rks_search_parameters
{
    // If provided, this will limit the results to record entries with a
    // particular parent.
    optional<string> parent;

    // If provided, will limit the results to a particular depth relative to
    // the parent (or the root scope, if no parent is provided).
    optional<unsigned> depth;

    // If true, will perform a recursive search through the entire entry
    // hierarchy. Note that the recursive search parameter will override any
    // specified depth setting.
    bool recursive;

    // If provided, this will limit the results to record entries with a
    // particular name.
    optional<string> name;

    // If provided, this will limit the results to record entries of a
    // particular record type. Note that the record must be a fully qualified
    // type name, i.e., it must be of the form ":account/:app/:record".
    optional<string> record;

    // If true, will include inactive entries in any query results.
    bool inactive;
};

void static inline
ensure_default_initialization(rks_search_parameters& x)
{
    x.recursive = false;
    x.inactive = false;
}

rks_entry_creation static
make_rks_entry_creation(
    string const& parent_id,
    iss_response const& iss,
    optional<string> const& name)
{
    rks_entry_creation e;
    e.name = name ? get(name) : to_string(boost::uuids::random_generator()());
    e.parent = parent_id;
    e.immutable = iss.id;
    e.active = true;
    return e;
}

// Construct an RKS search request.
web_request
make_rks_search_request(
    framework_context const& fc,
    rks_search_parameters const& parameters);

// Construct the search parameters for getting all the RKS entries descended
// from the given entry.
rks_search_parameters
make_rks_descendent_search(string const& entry_id);

// Filter a list of RKS entries to get only those matching the given record.
std::vector<rks_entry>
filter_rks_entries_by_record(
    std::vector<rks_entry> const& entries,
    rks_record const& record);

// Filter a list of RKS entries to get only those matching the given record
// and parent.
std::vector<rks_entry>
filter_rks_entries_by_record_and_parent(
    std::vector<rks_entry> const& entries,
    rks_record const& record,
    optional<string> const& parent);

// activity_record captures the time and user associated with a specific
// activity. It's the same convention as Thinknode, but with these fields
// isolated.
api(struct internal)
struct activity_record
{
    cradle::time at;
    user_reference by;
};

// Resolve the first activity of a list of RKS entries.
optional<activity_record>
resolve_first_activity(std::vector<rks_entry> const& records);

// Resolve the last activity of a list of RKS entries.
optional<activity_record>
resolve_last_activity(std::vector<rks_entry> const& records);

// This is a simple utility for extracting the timestamp associated with an
// optional activity record.
optional<cradle::time> static inline
extract_optional_activity_time(optional<activity_record> const& activity)
{
    return activity ? some(get(activity).at) : none;
}

// Post a new RKS entry
string
create_rks_entry(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    rks_entry_creation const& rks_entry_data,
    string const& qualified_record);

// This updates an RKS entry with new data
web_response
update_rks_entry(
    check_in_interface& check_in,
    progress_reporter_interface& reporter,
    web_connection &connection,
    web_session_data session,
    framework_context context,
    rks_entry_update const& rks_entry_data,
    string const& rks_id);

}

#endif
