#ifndef CRADLE_GUI_REQUESTS_HPP
#define CRADLE_GUI_REQUESTS_HPP

#include <cradle/background/requests.hpp>
#include <cradle/gui/common.hpp>

namespace cradle {

struct gui_request_data
{
    id_change_minimization_data<untyped_request> input_id;
    background_request_ptr ptr;
    local_identity output_id;
};

template<class Value>
struct typed_gui_request_data
{
    gui_request_data untyped;
    Value const* result;
    typed_gui_request_data() : result(0) {}
};

template<class Value>
struct gui_request_accessor : accessor<Value>
{
    gui_request_accessor() {}
    gui_request_accessor(typed_gui_request_data<Value>& data)
      : data_(&data)
    {}
    background_request_ptr& request_ptr() const { return data_->untyped.ptr; }
    id_interface const& id() const
    {
        if (data_->untyped.ptr.is_initialized())
        {
            id_ = get_id(data_->untyped.output_id);
            return id_;
        }
        else
            return no_id;
    }
    Value const& get() const { return *data_->result; }
    bool is_gettable() const { return data_->result != 0; }
    bool is_settable() const { return false; }
    void set(Value const& value) const {}
 private:
    typed_gui_request_data<Value>* data_;
    mutable value_id_by_reference<local_id> id_;
};

// Update a background request. This should be called on refresh passes.
// If this returns true, something has changed and the request's result should
// be inspected.
bool update_gui_request(
    gui_context& ctx, gui_request_data& data,
    accessor<framework_context> const& framework_context,
    accessor<untyped_request> const& request);

// gui_request performs a request in the background threads and returns an
// accessor to the result.
template<class Value>
gui_request_accessor<Value>
gui_request(gui_context& ctx,
    accessor<framework_context> const& framework_context,
    accessor<request<Value> > const& request)
{
    typed_gui_request_data<Value>* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        auto untyped_request = _field(request, untyped);
        if (update_gui_request(ctx, data->untyped, framework_context,
                untyped_request))
        {
            if (data->untyped.ptr.is_resolved())
            {
                assert(is_initialized(data->untyped.ptr.result()));
                cast_immutable_value(&data->result,
                    get_value_pointer(data->untyped.ptr.result()));
            }
            else
                data->result = 0;
        }
    }
    return gui_request_accessor<Value>(*data);
}

// This form of gui_request uses the default context associated with the GUI's
// background execution system.
template<class Value>
gui_request_accessor<Value>
gui_request(gui_context& ctx,
    accessor<request<Value> > const& request)
{
    return gui_request(ctx, get_framework_context(ctx), request);
}

// untyped helper for gui_request_objectified_form, below.
indirect_accessor<untyped_request>
gui_untyped_request_objectified_form(
    gui_context& ctx,
    accessor<framework_context> const& framework_context,
    accessor<untyped_request> const& request);

// gui_request_objectified_form yields the objectified form of a request.
template<class Value>
indirect_accessor<request<Value> >
gui_request_objectified_form(
    gui_context& ctx,
    accessor<framework_context> const& framework_context,
    accessor<request<Value> > const& request)
{
    return
        make_indirect(ctx,
            gui_apply(ctx,
                [ ](untyped_request const& untyped)
                {
                    cradle::request<Value> typed;
                    typed.untyped = untyped;
                    return typed;
                },
                gui_untyped_request_objectified_form(ctx, framework_context,
                    _field(request, untyped))));
}

// This form of gui_request_objectified_form uses the default context
// associated with the GUI's background execution system.
template<class Value>
indirect_accessor<request<Value> >
gui_request_objectified_form(
    gui_context& ctx,
    accessor<request<Value> > const& request)
{
    return
        gui_requestgui_request_objectified_form(ctx,
            get_framework_context(ctx),
            request);
}

// THINKNODE "LET" REQUESTS - This section deals with submitting generated
// Thinknode requests directly (outside the normal request system)
// (At some point, this should be merged.)

// This is used as the entity ID for objectified results, below.
api(struct internal)
struct objectified_result_entity_id
{
    calculation_request request;
    bool is_explicit;
};

// untyped helper for below
indirect_accessor<optional<let_calculation_submission_info> >
gui_untyped_thinknode_calculation_request(
    gui_context& ctx,
    accessor<augmented_calculation_request> const& request,
    accessor<bool> const& is_explicit);

// gui_thinknode_request_objectified_result takes a Thinknode request and
// yields the objectified form of the generated calculation's result.
//
// If is_explicit is false, the request will be made as a dry run.
//
template<class Value>
auto
gui_thinknode_request_objectified_result(
    gui_context& ctx,
    accessor<augmented_calculation_request> const& request,
    accessor<bool> const& is_explicit)
{
    // We're going to get back an optional calculation ID, so we need to
    // translate that into an optional request for the result object.
    auto construct_result_request =
        [ ](optional<let_calculation_submission_info> const& info)
            -> optional<cradle::request<Value> >
        {
            return
                info
              ? some(
                    make_typed_request<Value>(
                        request_type::OBJECT,
                        get(info).main_calc_id))
              : none;
        };
    return
        unwrap_optional(
            gui_apply(ctx,
                construct_result_request,
                gui_untyped_thinknode_calculation_request(
                    ctx, request, is_explicit)));
}

// Do the UI for controlling an expensive calculation in Thinknode that should
// only be run if the user explicitly requests it.
//
// 'trigger' is used as state to determine if the user has explicitly requested
// calculation of the given request. Its value can be considered opaque by the
// caller.
//
void
do_explicit_calculation_ui(
    gui_context& ctx,
    accessor<string> const& trigger,
    accessor<augmented_calculation_request> const& request,
    accessor<optional<string> > const& completed_message);

// Determine if the given request has been triggered for explicit calculation
// by the user.
// 'trigger' is the same trigger used in do_explicit_calculation_ui, above.
indirect_accessor<bool>
gui_request_is_triggered(
    gui_context& ctx,
    accessor<string> const& trigger,
    accessor<augmented_calculation_request> const& request);

// This summarizes the overall status of a high-level calulation, taking into
// account the full calculation tree and the contents of the calculation queue.
api(union internal)
union calc_status_summary
{
    // The calculation has completed.
    nil_type completed;
    // The calculation has failed.
    // (This could at some point include failure information.)
    nil_type failed;
    // Some part of the calculation is currently running.
    nil_type running;
    // The calculation is queued, and this is the position of the frontmost
    // part of it.
    size_t queued;
    // The calculation has been canceled by the user.
    nil_type canceled;
};

}

#endif
