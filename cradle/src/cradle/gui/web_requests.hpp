#ifndef CRADLE_GUI_WEB_REQUESTS_HPP
#define CRADLE_GUI_WEB_REQUESTS_HPP

#include <cradle/background/api.hpp>
#include <cradle/gui/background.hpp>
#include <cradle/io/web_io.hpp>

#include <boost/function.hpp>

namespace cradle {

struct gui_web_request_data
{
    untyped_background_data_ptr ptr;
    local_identity abbreviated_identity;
};

// Update a UI web request. This should be called on refresh passes.
// If this returns true, something has changed and the request's result should
// be inspected.
bool update_gui_web_request(
    gui_context& ctx, gui_web_request_data& data,
    accessor<web_request> const& request,
    dynamic_type_interface const& result_interface);

// Same as above, but written generically so that it can be used for requests
// that require custom background jobs (i.e., those that are more than just a
// single HTTP request).
// When the update requires actually creating the background job for handling
// the request, create_background_job is called.
bool update_generic_gui_web_request(
    gui_context& ctx, gui_web_request_data& data,
    untyped_accessor_base const& request,
    boost::function<void()> const& create_background_job);

template<class Value>
struct typed_gui_web_request_data
{
    gui_web_request_data untyped;
    Value const* result;
    typed_gui_web_request_data() : result(0) {}
};
template<class Value>
struct gui_web_request_accessor : accessor<Value>
{
    gui_web_request_accessor() {}
    gui_web_request_accessor(typed_gui_web_request_data<Value>& data)
      : data_(&data)
    {}
    id_interface const& id() const
    {
        if (data_->untyped.ptr.is_initialized())
        {
            id_ = get_id(data_->untyped.abbreviated_identity);
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
    typed_gui_web_request_data<Value>* data_;
    mutable value_id_by_reference<local_id> id_;
};

// Issue a web request through the GUI.
// Once the request completes, the returned accessor will be gettable and will
// yield the result of the request (parsed as JSON and converted to the
// specified value type).
// This is NOT tied into the mutable caching system, so it's really only meant
// for special cases like POST'ing ISS data.
template<class Value>
gui_web_request_accessor<Value>
gui_web_request(gui_context& ctx, accessor<web_request> const& request)
{
    typed_gui_web_request_data<Value>* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        static dynamic_type_implementation<Value> result_interface;
        if (update_gui_web_request(ctx, data->untyped, request,
                result_interface))
        {
            if (data->untyped.ptr.is_ready())
            {
                cast_immutable_value(&data->result,
                    data->untyped.ptr.data().ptr.get());
            }
            else
                data->result = 0;
        }
    }
    return gui_web_request_accessor<Value>(*data);
}

// untyped helper for gui_get_request, below
id_change_minimization_accessor<gui_mutable_value_accessor<value> >
untyped_gui_get_request(gui_context& ctx, accessor<string> const& url,
    accessor<std::vector<string> > const& headers);

// Issue a GET request through the GUI.
// Once the request completes, the returned accessor will be gettable and will
// yield the result of the request (parsed as JSON and converted to the
// specified value type).
// This uses the mutable caching system, so if the same request is made through
// multiple points in the UI, they'll share the same result. The entity ID type
// is get_request_entity_id, below.
template<class Value>
auto
gui_get_request(gui_context& ctx, accessor<string> const& url,
    accessor<std::vector<string> > const& headers)
{
    return
        apply_value_type<Value>(ctx,
            untyped_gui_get_request(ctx, url, headers));
}

api(struct internal)
struct get_request_entity_id
{
    string url;
    std::vector<string> headers;
};

// Get the API URL of the Thinknode account that we're currently using.
indirect_accessor<string>
get_api_url(gui_context& ctx, app_context& app_ctx);

}

#endif
