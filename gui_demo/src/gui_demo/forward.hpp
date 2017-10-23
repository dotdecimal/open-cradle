#ifndef GUI_DEMO_UI_FORWARD_HPP
#define GUI_DEMO_UI_FORWARD_HPP

#include <gui_demo/common.hpp>

namespace alia {

template<class Value>
struct accessor;
template<class Value>
struct indirect_accessor;

}

namespace cradle {

struct gui_context;
struct gui_task_stack;
struct gui_structure;
struct gui_point;
struct styled_text_fragment;
typedef std::vector<styled_text_fragment> styled_text;
struct app_context;

}

namespace gui_demo {

}

#endif
