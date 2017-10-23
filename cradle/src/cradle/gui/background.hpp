#ifndef CRADLE_GUI_BACKGROUND_HPP
#define CRADLE_GUI_BACKGROUND_HPP

#include <cradle/gui/common.hpp>
#include <cradle/background/api.hpp>
#include <cradle/disk_cache.hpp>

namespace cradle {

template<class Value>
struct background_accessor : accessor<Value>
{
    virtual background_data_ptr<Value> const&
    get_background_data_ptr() const = 0;
};

// deflicker(ctx, x, delay) returns an accessor to the value of x where changes
// in the value of x are hidden (by preserving the old value) until a new
// value emerges or delay ms has passed. This ensures that the accessor stays
// gettable throughout that period, and can thus eliminate any flickering that
// might otherwise occur.
struct untyped_deflickered_accessor_data;
struct deflickering_data
{
    bool is_current;
    untyped_deflickered_accessor_data* children;
};
unsigned const default_deflicker_delay = 100;
struct deflickerer : noncopyable
{
    deflickerer() {}
    deflickerer(gui_context& ctx, unsigned delay = default_deflicker_delay)
    { begin(ctx, delay); }
    ~deflickerer() { end(); }
    void begin(gui_context& ctx, unsigned delay = default_deflicker_delay);
    void end();
    gui_context* ctx_;
    deflickering_data* data_;
    unsigned delay_;
    bool valid_so_far_;
    bool change_detected_;
};
struct untyped_deflickered_accessor_data
{
    virtual void clear() = 0;
    virtual void copy_input() = 0;
    untyped_deflickered_accessor_data* next;
};
template<class Value>
struct deflickered_accessor_data : untyped_deflickered_accessor_data
{
    alia::owned_id input_id, output_id;
    immutable<Value> input_value, output_value;

    void clear()
    {
        output_id.store(no_id);
        reset(output_value);
    }
    void copy_input()
    {
        output_id.store(input_id.get());
        output_value = input_value;
    }
};
template<class Value>
struct deflickered_accessor : accessor<Value>
{
    deflickered_accessor() {}
    deflickered_accessor(deflickered_accessor_data<Value>* data)
      : data_(data)
    {}
    alia::id_interface const& id() const { return data_->output_id.get(); }
    Value const& get() const { return cradle::get(data_->output_value); }
    bool is_gettable() const { return is_initialized(data_->output_value); }
    bool is_settable() const { return false; }
    void set(Value const& value) const {}
 private:
    deflickered_accessor_data<Value>* data_;
};
template<class Value>
immutable<Value>
as_immutable(accessor<Value> const& x)
{
    assert(is_gettable(x));
    background_accessor<Value> const* ba =
        dynamic_cast<background_accessor<Value> const*>(&x);
    return ba ?
        cast_immutable<Value>(ba->get_background_data_ptr().untyped().data()) :
        make_immutable(get(x));
}
template<class Value>
deflickered_accessor<Value>
deflicker(
    gui_context& ctx, deflickerer& deflickerer,
    accessor<Value> const& x)
{
    deflickered_accessor_data<Value>* own_data;
    if (get_cached_data(ctx, &own_data))
        own_data->output_id.store(no_id);

    deflickering_data* shared_data = deflickerer.data_;

    if (is_refresh_pass(ctx))
    {
        own_data->next = shared_data->children;
        shared_data->children = own_data;

        if (!own_data->input_id.matches(x.id()))
        {
            deflickerer.change_detected_ = true;
            reset(own_data->input_value);
            own_data->input_id.store(x.id());
        }
        if (is_gettable(x))
        {
            if (!is_initialized(own_data->input_value))
                own_data->input_value = as_immutable(x);
        }
        else
            deflickerer.valid_so_far_ = false;
    }

    return deflickered_accessor<Value>(own_data);
}
template<class Value>
deflickered_accessor<Value>
deflicker(gui_context& ctx, accessor<Value> const& x,
    unsigned delay = default_deflicker_delay)
{
    deflickerer deflickerer(ctx, delay);
    return deflicker(ctx, deflickerer, x);
}

// MUTABLE VALUE ACCESSORS

template<class Value>
struct gui_mutable_value_data
{
    // the combination of the IDs of anything that affects the view of the
    // mutable value
    owned_id captured_id;
    // our view of the value
    immutable<Value> value;
    id_change_minimization_data<Value> id_change_minimization;
};

template<class Value>
struct gui_mutable_value_accessor : accessor<Value>
{
    gui_mutable_value_accessor() {}
    gui_mutable_value_accessor(gui_mutable_value_data<Value>& data)
      : data_(&data)
    {}
    id_interface const& id() const
    {
        auto const& captured_id = data_->captured_id;
        return captured_id.is_initialized() ? captured_id.get() : no_id;
    }
    Value const& get() const { return cradle::get(data_->value); }
    bool is_gettable() const { return is_initialized(data_->value); }
    bool is_settable() const { return false; }
    void set(Value const& value) const {}
 private:
    gui_mutable_value_data<Value>* data_;
};

template<class Value, class EntityId>
id_change_minimization_accessor<gui_mutable_value_accessor<Value> >
gui_mutable_entity_value(gui_context& ctx, accessor<EntityId> const& entity_id,
    boost::function<void(EntityId const& entity_id)> const& dispatch_job,
    // If data is supplied, it will be gotten from the UI context, so you
    // should either consistently supply it or not supply it for a particular
    // use of this function.
    gui_mutable_value_data<Value>* data = 0)
{
    if (!data)
        get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        if (is_gettable(entity_id))
        {
            auto& bg = get_background_system(ctx);
            auto view_id =
                combine_ids(
                    get_mutable_cache_update_id(bg),
                    ref(&entity_id.id()));
            if (!data->captured_id.matches(view_id))
            {
                // Either the cache has updated or we're looking at a
                // different entity, so update our view.
                auto new_value =
                    get_cached_mutable_value(
                        bg,
                        make_id(get(entity_id)),
                        [&]()
                        { dispatch_job(get(entity_id)); });
                if (is_initialized(new_value))
                    data->value = cast_immutable<Value>(new_value);
                else
                    reset(data->value);
                data->captured_id.store(view_id);
            }
        }
        else
        {
            // No entity ID, so just clear our view of the value.
            data->captured_id.clear();
            reset(data->value);
        }
    }
    return
        minimize_id_changes(ctx, &data->id_change_minimization,
            gui_mutable_value_accessor<Value>(*data));
}

// GUI REPORTS OF BACKGROUND SYSTEM

// Get the reports of permanent failures from the background system and issue
// them as notifications.
void issue_permanent_failure_notifications(gui_context& ctx);

// Display an overlay showing the status of the background execution system.
void do_background_status_report(gui_context& ctx);

// Show a report on the contents of the memory cache.
void do_memory_cache_report(gui_context& ctx);

}

#endif
