#ifndef CRADLE_IO_SERVICES_STATE_SERVICE_HPP
#define CRADLE_IO_SERVICES_STATE_SERVICE_HPP

#include <cradle/common.hpp>
#include <alia/accessors.hpp>
#include <cradle/date_time.hpp>

// This file defines the client interface to the state service.
// The state service provides storage of arbitrary state values and allows
// multiple clients to observe changes in that state in real-time as a single
// presenter manipulates it.

namespace cradle {

// state_service_connection represents a connection to the state service.
// The default constructor creates an uninitialized connection which must be
// initialized with initialize(), below.
struct state_service_connection_impl;
struct state_service_connection : noncopyable
{
    state_service_connection() : impl(0) {}
    ~state_service_connection();

    state_service_connection_impl* impl;
};

// Initialize a connection to a state server.
// host and port give the address of the server.
// session_id and user_id are used to authenticate.
void initialize(state_service_connection& conn,
    string const& host, unsigned port, string const& session_id,
    string const& user_id);

// Is this connection initialized?
bool static inline
is_initialized(state_service_connection& conn)
{ return conn.impl != 0; }

// Reset the connection.
void reset(state_service_connection& conn);

// Get the user ID associated with this connection.
string const& get_user_id(state_service_connection& conn);

// Get the session ID associated with this connection.
string const& get_session_id(state_service_connection& conn);

// Is the local client authenticated?
bool is_authenticated(state_service_connection& conn);

// Get the server-assigned connection ID associated with this connection.
// The local client must be authenticated before calling this.
string const& get_connection_id(state_service_connection& conn);

// state_service_client_state defines the state of a client connected to a
// state service session.
// This is part of the state service protocol.
api(struct internal)
struct state_service_client_state
{
    // the ID of the user that's connected on this client
    // A single user may connect multiple times to the same session and will
    // appear as separate clients.
    string user_id;
    // time of last activity
    cradle::time activity_at;
    // application-defined client state
    value data;
};

// state_service_session_state defines the temporary, session-specific state
// associated with a a state service session.
// This is part of the state service protocol.
api(struct internal)
struct state_service_session_state
{
    // connection ID of presenter
    string presenter;
    // list of connected clients as a map from connection IDs to states
    std::map<string,state_service_client_state> clients;
};

// state_service_state defines the full data associated with a state service
// session. This includes the persistent state that's being manipulated.
// This is part of the state service protocol.
api(struct internal)
struct state_service_state
{
    // persistent, application-defined state
    value data;
    // info about the current session.
    state_service_session_state session;
};

// state_service_session represents a single session of observing and/or
// manipulating the state within a single slot.
// A state_service_session must be conducted over a state_service_connection.
// Currently, the implementation assumes that connections are not reused, so
// you should maintain a separate connection for each session.
struct state_service_session
{
    state_service_connection* conn;
    alia::state<state_service_state> state;
    bool valid;

    state_service_session() : conn(0), valid(false) {}
};

static inline state_service_connection&
get_connection(state_service_session& session)
{ return *session.conn; }

// Open a session.
void open_session(state_service_session& session,
    state_service_connection& conn, string const& slot_id);

// Is this session valid?
bool static inline
is_session_valid(state_service_session& session)
{
    return session.valid;
}

void reset(state_service_session& session);

// Do asynchronous I/O for this session and update the associated state if
// any changes arrived from the serer.
// The return value indicates whether or not anything changed.
bool process_io(state_service_session& session);

// Is the local client the presenter of this session?
bool local_client_is_presenter(state_service_session& session);

// Get the user_id of the presenter.
string const& get_presenter_user_id(state_service_session& session);

// Does the local client have permission to change the presenter?
bool can_make_presenter(state_service_session& session);

// Give the presenter status to another user.
// To call this, the user_id of the local client must match the user_id of
// the presenter.
void make_presenter(state_service_session& session,
    string const& new_presenter);

// Update the state associated with a session and communicate those changes
// to the server.
// To call this, the local client must be the presenter.
void write_shared_data(state_service_session& session, value const& data);

// Get the state associated with a particular client.
value const&
get_client_state(state_service_session& session, string const& user_id);

// Update the application-defined session data associated with the local
// client.
void write_client_data(state_service_session& session, value const& data);

}

#endif
