#ifndef CRADLE_GUI_DISPLAYS_DISPLAY_HPP
#define CRADLE_GUI_DISPLAYS_DISPLAY_HPP

#include <cradle/gui/common.hpp>
#include <boost/function.hpp>

#include <cradle/gui/displays/types.hpp>

namespace cradle {

struct embedded_canvas;

api(enum internal)
enum class display_layout_type
{
    MAIN_PLUS_ROW,
    MAIN_PLUS_SMALL_COLUMN,
    TWO_ROWS,
    TWO_COLUMNS,
    SQUARES,
    COLUMN_PLUS_MAIN,
    MAIN_PLUS_COLUMN

};

api(struct internal)
struct display_view_instance
{
    string instance_id;
    string type_id;
};

typedef std::vector<display_view_instance> display_view_instance_list;

// TODO: Separate this out into composition state and definition.
api(struct internal)
struct display_view_composition
{
    string id;
    string label;
    display_view_instance_list views;
    display_layout_type layout;
};

typedef std::vector<display_view_composition> display_view_composition_list;

struct display_view_provider_interface
{
    // Get the number of view types provided by this provider.
    virtual unsigned get_count() = 0;

    // Get the ID of the nth view type provided by this provider.
    virtual string const&
    get_type_id(unsigned type_index) = 0;

    // Get the label for a particular type of view.
    virtual string const&
    get_type_label(string const& type_id) = 0;

    // Get the label of an instantiated view.
    virtual indirect_accessor<string>
    get_view_label(gui_context& ctx, string const& type_id,
        string const& instance_id) = 0;

    // Do the main content of a view.
    // is_preview indicates whether or not this call is being used to preview
    // the view.
    virtual void
    do_view_content(gui_context& ctx, string const& type_id,
        string const& instance_id, bool is_preview) = 0;
};

template<class DisplayContext>
struct display_view_interface
{
    virtual string const& get_type_id() const = 0;

    // Get the label for this type of view.
    virtual string const&
    get_type_label(DisplayContext const& display_ctx) = 0;

    // Get the label of an instantiatied view.
    virtual indirect_accessor<string>
    get_view_label(gui_context& ctx, DisplayContext const& display_ctx,
        string const& instance_id) = 0;

    // Do the main content of a view.
    virtual void
    do_view_content(gui_context& ctx, DisplayContext const& display_ctx,
        string const& instance_id, bool is_preview) = 0;

    // used to construct a linked-list of views in a provider
    display_view_interface* next_view;
};

template<class DisplayContext>
struct display_view_provider : display_view_provider_interface
{
    display_view_provider(DisplayContext* display_ctx)
      : display_ctx(display_ctx), head(0), tail(0)
    {}
    void add_view(display_view_interface<DisplayContext>* view)
    {
        view->next_view = 0;
        if (tail)
            tail->next_view = view;
        else
            head = view;
        tail = view;
    }

    unsigned get_count()
    {
        unsigned count = 0;
        for (auto* view = head; view; view = view->next_view)
            ++count;
        return count;
    }

    string const&
    get_type_id(unsigned index)
    {
        auto* view = head;
        for (unsigned i = 0; i != index && view; ++i)
            view = view->next_view;
        if (!view)
            throw exception("invalid view index");
        return view->get_type_id();
    }

    display_view_interface<DisplayContext>&
    find_view(string const& type_id)
    {
        for (auto* view = head; view; view = view->next_view)
        {
            if (view->get_type_id() == type_id)
                return *view;
        }
        throw exception("unsupported view type");
    }

    string const&
    get_type_label(string const& type_id)
    {
        return this->find_view(type_id).get_type_label(*display_ctx);
    }

    indirect_accessor<string>
    get_view_label(gui_context& ctx, string const& type_id,
        string const& instance_id)
    {
        return this->find_view(type_id).
            get_view_label(ctx, *display_ctx, instance_id);
    }

    void do_view_content(gui_context& ctx, string const& type_id,
        string const& instance_id, bool is_preview)
    {
        this->find_view(type_id).
            do_view_content(ctx, *display_ctx, instance_id, is_preview);
    }

    DisplayContext* display_ctx;
    display_view_interface<DisplayContext>* head;
    display_view_interface<DisplayContext>* tail;
};

void do_display(gui_context& ctx,
    display_view_provider_interface& provider,
    accessor<display_view_composition_list> const& compositions,
    accessor<display_state> const& state,
    accessor<float> const& controls_width,
    boost::function<void (
        gui_context& ctx, accessor<display_state> const& state,
        accordion& accordion)> const& do_controls);

// A macro for defining simple views with static labels.
#define CRADLE_DEFINE_SIMPLE_VIEW(view_type, DisplayContext, \
    type_id, label, content_implementation) \
    struct view_type : display_view_interface<DisplayContext> \
    { \
        view_type() {} \
        string const& get_type_id() const \
        { \
            static string the_type_id = type_id; \
            return the_type_id; \
        } \
        string const& \
        get_type_label(DisplayContext const& display_ctx) \
        { \
            static string const type_label = label; \
            return type_label; \
        } \
        indirect_accessor<string> \
        get_view_label(gui_context& ctx, DisplayContext const& display_ctx, \
            string const& instance_id) \
        { \
            return make_indirect(ctx, text(label)); \
        } \
        void \
        do_view_content(gui_context& ctx, DisplayContext const& display_ctx, \
            string const& instance_id, bool is_preview) \
        { \
            content_implementation \
        } \
    };

}

#endif
