#ifndef CRADLE_GUI_SERVICES_HPP
#define CRADLE_GUI_SERVICES_HPP

#include <cradle/gui/requests.hpp>
#include <cradle/gui/web_requests.hpp>
#include <cradle/io/services/calc_internals.hpp>
#include <cradle/io/services/core_services.hpp>
#include <cradle/io/services/iss.hpp>
#include <cradle/io/services/rks.hpp>

namespace cradle {

// CAS

indirect_accessor<session_info>
get_session_info(gui_context& ctx, app_context& app_ctx);

// IAM

indirect_accessor<user_info>
get_user_info(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& user_id);

indirect_accessor<realm>
get_realm_info(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& realm_id);

// CALC

api(struct internal)
struct calc_status_entity_id
{
    string id;
};

indirect_accessor<calculation_status>
gui_calc_status(gui_context& ctx,
    accessor<string> const& entry_id);

// This is the entity ID of the calculation queue. There's only one queue, so
// no ID is needed.
api(struct internal)
struct calc_queue_entity_id
{
};

// Get the current status of the calculation queue.
indirect_accessor<std::vector<calculation_queue_item> >
gui_calc_queue_status(gui_context& ctx);

// RKS

typedef gui_mutable_value_data<rks_entry> rks_entry_resolution_data;

// This is used as the mutable cache's 'entity' ID for RKS entries.
api(struct internal)
struct rks_entry_entity_id
{
    string id;
};

// gui_rks_entry(ctx, app_ctx, entry_id) yields an accessor to the RKS entry
// identified by entry_id.
indirect_accessor<rks_entry>
gui_rks_entry(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& entry_id);
// same, but allows you to supply your own data
indirect_accessor<rks_entry>
gui_rks_entry(gui_context& ctx, app_context& app_ctx,
    rks_entry_resolution_data& data, accessor<string> const& entry_id);

template<class Value>
struct gui_rks_entry_value_data
{
    rks_entry_resolution_data entry_data;
    // If this is valid and matches the current refresh ID for the entry's ID,
    // then the record is being written to.
    // When this is the case, this stores the data that was written.
    keyed_data<Value> written_data;
    gui_rks_entry_value_data() {}
};

// Set an RKS entry to a new value through the GUI.
// This is intended to be used primarily through gui_rks_entry_value() below.
void set_gui_rks_entry_value(
    gui_context& ctx,
    rks_entry const& entry,
    dynamic_type_interface const& value_interface,
    untyped_immutable const& new_value);

template<class Value>
struct rks_fallback_value
{
    owned_id input_id;
    local_identity output_id;
    optional<Value> value;
};

template<class Value>
struct rks_entry_value_accessor : accessor<Value>
{
    rks_entry_value_accessor() {}
    rks_entry_value_accessor(
        gui_context& ctx,
        indirect_accessor<rks_entry> const& entry_accessor_,
        dynamic_type_interface const& value_interface,
        gui_request_accessor<Value> const& iss_accessor,
        rks_fallback_value<Value>& fallback)
      : ctx_(&ctx), entry_accessor_(entry_accessor_),
        value_interface_(&value_interface), iss_accessor_(iss_accessor),
        fallback_(&fallback)
    {}

    id_interface const& id() const
    {
        if (iss_accessor_.is_gettable())
        {
            return iss_accessor_.id();
        }
        else if (fallback_->value)
        {
            fallback_id_ = get_id(fallback_->output_id);
            return fallback_id_;
        }
        else
            return no_id;
    }

    Value const& get() const
    {
        if (iss_accessor_.is_gettable())
            return iss_accessor_.get();
        else
            return fallback_->value.get();
    }

    bool is_gettable() const
    {
        return iss_accessor_.is_gettable() || fallback_->value;
    }

    // We can only write to the entry if we have the existing one.
    bool is_settable() const
    {
        return entry_accessor_.is_gettable();
    }

    void set(Value const& value) const
    {
        // Ignore updates if they're the same as the current value.
        // (Updates to the same value are actually causing problems right now
        // (see AST-1429.))
        if (value != this->get())
        {
            set_gui_rks_entry_value(*ctx_, entry_accessor_.get(),
                *value_interface_, erase_type(make_immutable(value)));

            // Update fallback value.
            fallback_->input_id.clear();
            fallback_->value = value;
            inc_version(fallback_->output_id);
        }
    }

 private:
    gui_context* ctx_;
    indirect_accessor<rks_entry> entry_accessor_;
    dynamic_type_interface const* value_interface_;
    gui_request_accessor<Value> iss_accessor_;
    rks_fallback_value<Value>* fallback_;
    mutable value_id_by_reference<local_id> fallback_id_;
};

// gui_rks_entry_value provides a read-write accessor to the value held by an
// RKS entry (identified by the supplied ID).
template<class Value>
rks_entry_value_accessor<Value>
gui_rks_entry_value(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& entry_id)
{
    rks_entry_resolution_data* entry_data;
    get_data(ctx, &entry_data);

    auto entry_accessor =
        gui_rks_entry(ctx, app_ctx, *entry_data, entry_id);

    auto iss_id =
        _field(entry_accessor, immutable);

    auto iss_accessor =
        gui_request(ctx,
            gui_apply(ctx,
                [](string const& id)
                {
                    return rq_object(object_reference<Value>(id));
                },
                iss_id));

    rks_fallback_value<Value>* fallback;
    get_data(ctx, &fallback);

    if (is_refresh_pass(ctx))
    {
        if (is_gettable(iss_accessor) &&
            !fallback->input_id.matches(iss_accessor.id()))
        {
            fallback->input_id.store(iss_accessor.id());
            fallback->value = get(iss_accessor);
            inc_version(fallback->output_id);
        }
    }

    static dynamic_type_implementation<Value> value_interface;

    return rks_entry_value_accessor<Value>(ctx, entry_accessor,
        value_interface, iss_accessor, *fallback);
}

// This is used as the mutable cache's entity ID for RKS searches.
api(struct internal)
struct rks_search_entity_id
{
    rks_search_parameters parameters;
};

rks_search_entity_id
make_rks_search_entity_id(rks_search_parameters const& parameters);

// Get an accessor to the list of records matching specified search parameters.
indirect_accessor<std::vector<rks_entry> >
gui_rks_search(gui_context& ctx, app_context& app_ctx,
    accessor<rks_search_parameters> const& parameters);

// Request for a new RKS entry to be created.
// qualified_record includes the account, app, and record name.
// The returned accessor will yield the entry's initial state.
gui_web_request_accessor<rks_entry>
gui_new_rks_entry(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_record,
    accessor<rks_entry_creation> const& entry);

// Worker for updating an RKS lock/unlock status
void
set_gui_rks_lock_entry(
    gui_context& ctx,
    rks_entry const& entry,
    lock_type new_locked_type,
    bool deep_lock,
    dynamic_type_interface const& value_interface,
    optional<untyped_immutable> const& new_value);

// Get the ID for the RKS entry with the given name, parent, and record.
// If no entry exists with that name and parent, one is created using
// default_immutable_id as the initial value.
// Note that if the entry does exist but belongs to the wrong record, this
// will trigger an error.
gui_web_request_accessor<string>
gui_rks_entry_id_by_name(
    gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_record,
    accessor<optional<string> > const& parent_id,
    accessor<string> const& name,
    accessor<string> const& default_immutable_id);

// Call this to watch an RKS entry for changes. This will dispatch a job to
// long poll that entry and, when it detects changes, will update the mutable
// data cache so that any other UI elements referencing the same entry ID will
// pick up those changes.
void
watch_rks_entry(
    gui_context& ctx, app_context& app_ctx,
    accessor<string> const& entry_id);

// ISS

// Request for data to be posted as an ISS object.
// The data must already be formatted as a MessagePack blob.
// (This version is intended to be used via gui_post_iss_object, below.)
gui_web_request_accessor<iss_response>
gui_post_iss_msgpack_blob(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_type, accessor<blob> const& data);

// Request for data to be posted as an ISS object.
// The returned accessor will yield the object's ID.
template<class Value>
gui_web_request_accessor<iss_response>
gui_post_iss_object(gui_context& ctx, app_context& app_ctx,
    accessor<string> const& qualified_type, accessor<Value> const& value)
{
    return
        gui_post_iss_msgpack_blob(ctx, app_ctx,
            qualified_type,
            gui_apply(ctx,
                value_to_msgpack_blob,
                gui_apply(ctx,
                    static_cast<cradle::value(*)(Value const&)>(&to_value),
                    value)));
}

// RKS

gui_web_request_accessor<std::vector<rks_entry>>
gui_get_history_data(gui_context& ctx,
    accessor<framework_context> const& fc, accessor<string> const& id);

//// STATE SERVICE
//
//struct state_service_session;
//struct state_service_session_state;
//
//// state_service_state_accessor provides read-write access to the primary state
//// associated with a state service session.
//struct state_service_state_accessor : accessor<value>
//{
//    state_service_state_accessor() {}
//    state_service_state_accessor(state_service_session* session)
//      : session_(session)
//    {}
//    id_interface const& id() const;
//    bool is_gettable() const;
//    value const& get() const;
//    bool is_settable() const;
//    void set(value const& x) const;
// private:
//    state_service_session* session_;
//    mutable id_pair<value_id_by_reference<local_id>,value_id<int*> > id_;
//};
//
//// state_service_local_client_state_accessor provides read-write access to the
//// state that a state service session has associated with the local client.
//struct state_service_local_client_state_accessor : accessor<value>
//{
//    state_service_local_client_state_accessor() {}
//    state_service_local_client_state_accessor(state_service_session* session)
//      : session_(session)
//    {}
//    id_interface const& id() const;
//    bool is_gettable() const;
//    value const& get() const;
//    bool is_settable() const;
//    void set(value const& x) const;
// private:
//    state_service_session* session_;
//    mutable id_pair<value_id_by_reference<local_id>,value_id<int*> > id_;
//};
//
//// state_service_session_state_accessor provides read-only access to the
//// session-specific state associated with the state service session.
//struct state_service_session_state_accessor
//  : accessor<state_service_session_state>
//{
//    state_service_session_state_accessor() {}
//    state_service_session_state_accessor(state_service_session* session)
//      : session_(session)
//    {}
//    id_interface const& id() const;
//    bool is_gettable() const;
//    state_service_session_state const& get() const;
//    bool is_settable() const { return false; }
//    void set(state_service_session_state const& x) const {}
// private:
//    state_service_session* session_;
//    mutable id_pair<value_id_by_reference<local_id>,value_id<int*> > id_;
//};
//
//// Get a state service session for the given mutable slot ID.
//// Note that this must be the _id stored in the mutable slot, not the external
//// ID by which it's usually referenced.
//state_service_session*
//get_state_service_session(
//    gui_context& ctx,
//    accessor<string> const& host,
//    accessor<uint16_t> const& port,
//    accessor<string> const& session_id,
//    accessor<string> const& user_id,
//    accessor<string> const& slot_id);

}

#endif
