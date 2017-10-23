#ifndef CRADLE_IO_SERVICES_STATE_INTERNALS_HPP
#define CRADLE_IO_SERVICES_STATE_INTERNALS_HPP

#include <cradle/common.hpp>
#include <cradle/io/services/state_service.hpp>
#include <cradle/diff.hpp>

// This file defines the internal data structures used in the state service
// protocol.

namespace cradle {

// authentication message sent by the client
api(struct internal)
struct state_service_authentication
{
    // session ID
    string sid;
    // user ID
    string uid;
    // protocol version
    unsigned ver;
};

// status codes returned by the server
api(enum internal)
enum class state_service_response_status
{
    OK
};

// general response from server (for errors and simple confirmations)
api(struct internal)
struct state_service_response
{
    state_service_response_status status;
    // If status is not OK, this will be a message explaining the error.
    optional<string> message;
};

// a message sent by the client
api(union internal)
union state_service_client_message
{
    state_service_authentication authenticate;

    // Request to open a session.
    // The value is the ID of the state object.
    string open;

    // Close the current session.
    nil_type close;

    // Sent when the primary state data is changed.
    value_diff data_changes;

    // Sent to change the presenter.
    // The value is the ID of the new presenter.
    string presenter_change;

    // Sent when the local client changes its client-specific session data.
    value_diff client_changes;
};

// a message sent by the server
api(union internal)
union state_service_server_message
{
    state_service_response response;

    // Sent by server when authentication succeeds.
    // The value is the connection_id of the local client.
    string authenticated;

    // Sent by server to give the client the complete initial state.
    state_service_state state;

    // Sent by server to inform the client of any state changes.
    value_diff changes;
};

}

#endif
