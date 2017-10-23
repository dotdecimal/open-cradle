#ifndef CRADLE_SCRIPTING_HPP
#define CRADLE_SCRIPTING_HPP

#include <cradle/api.hpp>
#include <cradle/external/lua.hpp>

namespace cradle {

void register_lua_functions(lua_State* L, string const& package_name,
    api_function_list const& functions);

void register_lua_api(lua_State* L, string const& package_name,
    api_info const& api);

}

#endif
