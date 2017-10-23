#include <cradle/io/services/state_service.hpp>
#include <cradle/io/services/state_internals.hpp>
#include <cradle/io/tcp_messaging.hpp>

// This file implements the client interface to the state service.

using boost::asio::ip::tcp;

namespace cradle {

// Currently the state service isn't functional, so this is a stub
// implementation that simply stores the data locally, temporarily.

#define USE_LOCAL_STUB

#ifdef USE_LOCAL_STUB

struct state_service_connection_impl
{
    string session_id, user_id;
};

void initialize(state_service_connection& conn,
    string const& host, unsigned port, string const& session_id,
    string const& user_id)
{
    reset(conn);
    conn.impl = new state_service_connection_impl;
    state_service_connection_impl& impl = *conn.impl;
    impl.session_id = session_id;
    impl.user_id = user_id;
}

state_service_connection::~state_service_connection()
{
    delete impl;
}

void reset(state_service_connection& conn)
{
    delete conn.impl;
    conn.impl = 0;
}

string const& get_user_id(state_service_connection& conn)
{
    return conn.impl->user_id;
}

string const& get_session_id(state_service_connection& conn)
{
    return conn.impl->session_id;
}

bool is_authenticated(state_service_connection& conn)
{
    return true;
}

string const& get_connection_id(state_service_connection& conn)
{
    static string id = "local_stub";
    return id;
}

void open_session(state_service_session& session,
    state_service_connection& conn, string const& slot_id)
{
    reset(session);
    session.conn = &conn;
    session.valid = true;
}

void reset(state_service_session& session)
{
    session = state_service_session();
}

bool local_client_is_presenter(state_service_session& session)
{
    return true;
}

bool process_io(state_service_session& session)
{
    return false;
}

// Get a mutable reference to the local client's view of its own
// client-specific shared state.
// Note that calling this triggers a change in the local ID associated with
// that value, so only do it if you intend to actually change its value.
static state_service_client_state&
get_local_client_state(state_service_session& session)
{
    return session.state.nonconst_get().session.clients[
        get_connection_id(*session.conn)];
}

value const&
get_client_state(state_service_session& session, string const& connection_id)
{
    auto& clients = session.state.get().session.clients;
    auto client = clients.find(connection_id);
    static value no_data(nil);
    return client == clients.end() ? no_data : client->second.data;
}

// Update the activity of the local client to the current time.
// This must be done whenever the client sends updates to the server since
// the server (by design) doesn't echo those changes back to the client.
// (This of course means that the client's local value of activity_at might
// differ from the server's value, which is the value that other clients see.
// However, this isn't considered a big enough problem to warrant sending
// extra messages.)
void static
record_local_client_activity(state_service_session& session)
{
    get_local_client_state(session).activity_at =
        boost::posix_time::second_clock::universal_time();
}

void write_shared_data(state_service_session& session, value const& data)
{
    session.state.nonconst_get().data = data;
    record_local_client_activity(session);
}

string const& get_presenter_user_id(state_service_session& session)
{
    auto const& state = session.state.get().session;
    auto const i = state.clients.find(state.presenter);
    assert(i != state.clients.end());
    static string dummy;
    return i == state.clients.end() ? dummy : i->second.user_id;
}

bool can_make_presenter(state_service_session& session)
{
    return get_presenter_user_id(session) == get_user_id(*session.conn);
}

void make_presenter(state_service_session& session, string const& client_id)
{
}

void write_client_data(state_service_session& session, value const& data)
{
    get_local_client_state(session).data = data;
    record_local_client_activity(session);
}

#else // not USE_LOCAL_STUB

struct state_service_connection_impl
{
    string session_id, user_id;

    tcp_messaging_connection<state_service_server_message,
        state_service_client_message> messaging;

    // server-assigned connection ID
    // Once this is set, the local client is successfully authentication.
    optional<string> connection_id;
};

void initialize(state_service_connection& conn,
    string const& host, unsigned port, string const& session_id,
    string const& user_id)
{
    reset(conn);

    conn.impl = new state_service_connection_impl;
    state_service_connection_impl& impl = *conn.impl;
    impl.session_id = session_id;
    impl.user_id = user_id;

    initialize(impl.messaging, 0x0A5F5C14, host, port);

    // Send the authentication message.
    state_service_authentication authentication(session_id, user_id, 1);
    post_message(impl.messaging,
        make_state_service_client_message_with_authenticate(authentication));
}

state_service_connection::~state_service_connection()
{
    delete impl;
}

void reset(state_service_connection& conn)
{
    delete conn.impl;
    conn.impl = 0;
}

string const& get_user_id(state_service_connection& conn)
{
    return conn.impl->user_id;
}

string const& get_session_id(state_service_connection& conn)
{
    return conn.impl->session_id;
}

bool is_authenticated(state_service_connection& conn)
{
    return conn.impl->connection_id ? true : false;
}

string const& get_connection_id(state_service_connection& conn)
{
    assert(conn.impl->connection_id);
    return get(conn.impl->connection_id);
}

void open_session(state_service_session& session,
    state_service_connection& conn, string const& slot_id)
{
    session.conn = &conn;
    session.valid = false;
    post_message(conn.impl->messaging,
        make_state_service_client_message_with_open(slot_id));
}

void reset(state_service_session& session)
{
    session = state_service_session();
}

bool local_client_is_presenter(state_service_session& session)
{
    auto const& presenter = session.state.get().session.presenter;
    return presenter == get_connection_id(*session.conn);
}

bool process_io(state_service_session& session)
{
    if (!session.conn)
        return false;

    auto& impl = *session.conn->impl;

    poll(impl.messaging);

    bool changes_detected = false;

    auto& messages = impl.messaging.received_messages;
    while (!messages.empty())
    {
        auto message = messages.front();
        messages.pop();
        switch (message.type)
        {
         case state_service_server_message_type::RESPONSE:
          {
            auto const& response = as_response(message);
            if (response.status != state_service_response_status::OK)
                throw exception(get(response.message));
            break;
          }
         case state_service_server_message_type::AUTHENTICATED:
          {
            impl.connection_id = as_authenticated(message);
            break;
          }
         case state_service_server_message_type::STATE:
          {
            auto const& state = as_state(message);
            session.state = state;
            changes_detected = true;
            session.valid = true;
            break;
          }
         case state_service_server_message_type::CHANGES:
          {
            auto const& diff = as_changes(message);
            session.state = from_value<state_service_state>(
                apply_value_diff(to_value(session.state.get()), diff));
            changes_detected = true;
            break;
          }
        }
    }

    return changes_detected;
}

// Get a mutable reference to the local client's view of its own
// client-specific shared state.
// Note that calling this triggers a change in the local ID associated with
// that value, so only do it if you intend to actually change its value.
static state_service_client_state&
get_local_client_state(state_service_session& session)
{
    return session.state.nonconst_get().session.clients[
        get_connection_id(*session.conn)];
}

value const&
get_client_state(state_service_session& session, string const& connection_id)
{
    auto& clients = session.state.get().session.clients;
    auto client = clients.find(connection_id);
    static value no_data(nil);
    return client == clients.end() ? no_data : client->second.data;
}

// Update the activity of the local client to the current time.
// This must be done whenever the client sends updates to the server since
// the server (by design) doesn't echo those changes back to the client.
// (This of course means that the client's local value of activity_at might
// differ from the server's value, which is the value that other clients see.
// However, this isn't considered a big enough problem to warrant sending
// extra messages.)
void static
record_local_client_activity(state_service_session& session)
{
    get_local_client_state(session).activity_at =
        boost::posix_time::second_clock::universal_time();
}

void write_shared_data(state_service_session& session, value const& data)
{
    auto& shared_data = session.state.nonconst_get().data;
    auto diff = compute_value_diff(shared_data, data);
    shared_data = data;
    post_message(session.conn->impl->messaging,
        make_state_service_client_message_with_data_changes(diff));
    record_local_client_activity(session);
}

string const& get_presenter_user_id(state_service_session& session)
{
    auto const& state = session.state.get().session;
    auto const i = state.clients.find(state.presenter);
    assert(i != state.clients.end());
    static string dummy;
    return i == state.clients.end() ? dummy : i->second.user_id;
}

bool can_make_presenter(state_service_session& session)
{
    return get_presenter_user_id(session) == get_user_id(*session.conn);
}

void make_presenter(state_service_session& session, string const& client_id)
{
    post_message(session.conn->impl->messaging,
        make_state_service_client_message_with_presenter_change(client_id));
    record_local_client_activity(session);
}

void write_client_data(state_service_session& session, value const& data)
{
    auto& client_state = get_local_client_state(session);
    auto diff = compute_value_diff(client_state.data, data);
    client_state.data = data;
    post_message(session.conn->impl->messaging,
        make_state_service_client_message_with_client_changes(diff));
    record_local_client_activity(session);
}

#endif

}
