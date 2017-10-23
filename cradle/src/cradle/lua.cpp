#include <cradle/lua.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cradle/external/lua.hpp>

namespace cradle {

struct untyped_boxed_data
{
    virtual ~untyped_boxed_data() {}
};

template<class Data>
struct boxed_data : untyped_boxed_data
{
    Data data;
};

static int free_boxed_data(lua_State* L)
{
    if (lua_gettop(L) != 1)
        luaL_error(L, "internal error: free_boxed_data() called incorrectly");
    untyped_boxed_data** ptr = (untyped_boxed_data**)lua_touserdata(L, 1);
    if (!ptr)
        luaL_error(L, "internal error: free_boxed_data() called incorrectly");
    delete *ptr;
    return 0;
}

template<class Data>
static void push_boxed_data(lua_State* L, Data const& data)
{
    boxed_data<Data>* boxed = new boxed_data<Data>;
    *(untyped_boxed_data**)lua_newuserdata(L, sizeof(untyped_boxed_data*)) =
        boxed;
    boxed->data = data;

    // Add a metatable so the data gets freed when lua gc's it.
    lua_newtable(L);
    lua_pushcfunction(L, &free_boxed_data);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
}

template<class Data>
static Data const& get_boxed_data(lua_State* L, int index)
{
    untyped_boxed_data** ptr = (untyped_boxed_data**)lua_touserdata(L, index);
    if (!ptr)
        throw exception("invalid userdata");
    boxed_data<Data>* typed = dynamic_cast<boxed_data<Data>*>(*ptr);
    if (!typed)
        throw exception("invalid userdata");
    return typed->data;
}

// Determine if the table at the given index in the Lua context L represents a
// list. If it does, set *length to the length of the list and return true.
// Otherwise, return false.
static bool lua_table_is_list(int* length, lua_State* L, int index)
{
    // Iterate through the key/value pairs in the table.
    lua_pushnil(L); // first key
    int max_index = 0, n_keys = 0;
    while (lua_next(L, index))
    {
        // Now the key is at index -2 and the value is at index -1.
        // Check if the key is a number. If not, then this isn't a list.
        if (!lua_isnumber(L, -2))
        {
            // Pop the value and key.
            lua_pop(L, 2);
            return false;
        }
        double n = lua_tonumber(L, -2);
        // Check if n is a positive integer. If not, then this isn't a list.
        int i = int(n);
        if (double(i) != n || i < 1)
        {
            // Pop the value and key.
            lua_pop(L, 2);
            return false;
        }
        max_index = i;
        ++n_keys;
        // Pop the value.
        // The key is replaced by the next call to lua_next().
        lua_pop(L, 1);
    }
    // If this is a list, there should be no gaps in the indices.
    if (max_index != n_keys)
        return false;
    *length = n_keys;
    return true;
}

static void get_from_lua(value* v, lua_State* L, int index)
{
    switch (lua_type(L, index))
    {
     case LUA_TBOOLEAN:
        set(*v, lua_toboolean(L, index) != 0 ? true : false);
        break;

     case LUA_TNUMBER:
        set(*v, lua_tonumber(L, index));
        break;

     case LUA_TSTRING:
        set(*v, string(lua_tostring(L, index)));
        break;

     case LUA_TUSERDATA:
        set(*v, get_boxed_data<blob>(L, index));
        break;

     case LUA_TTABLE:
      {
        // If index is relative, convert it to absolute.
        // This is necessary because traversing the table requires pushing
        // elements onto the stack.
        if (index < 0)
            index = lua_gettop(L) + 1 + index;

        int list_length;
        if (lua_table_is_list(&list_length, L, index))
        {
            // Iterate through the key/value pairs in the table.
            lua_pushnil(L); // first key
            value_list list(list_length);
            while (lua_next(L, index))
            {
                // Now the key is at index -2 and the value is at index -1.
                int i = int(lua_tonumber(L, -2));
                try
                {
                    get_from_lua(&list[i - 1], L, -1);
                }
                catch (exception& e)
                {
                    e.add_context("at index " + to_string(i));
                    throw;
                }
                // Pop the value.
                // The key is replaced by the next call to lua_next().
                lua_pop(L, 1);
            }
            set(*v, list);
        }
        else
        {
            // Iterate through the key/value pairs in the table.
            lua_pushnil(L); // first key
            value_map map;
            while (lua_next(L, index))
            {
                // Copy the key so that the string checks don't modify it.
                lua_pushvalue(L, -2);
                // Now the key copy is at index -1 and the value is at -2.
                if (!lua_isstring(L, -1))
                    throw exception("record key must be string");
                string key(lua_tostring(L, -1));
                try
                {
                    get_from_lua(&map[key], L, -2);
                }
                catch (exception& e)
                {
                    e.add_context("in field " + key);
                    throw;
                }
                // Pop the value and the copy of the key.
                // The original key is replaced by the next call to lua_next().
                lua_pop(L, 2);
            }
            set(*v, map);
        }
        break;
      }

     default:
        set(*v, nil);
    }
}

static void push_to_lua(lua_State* L, value const& v)
{
    switch (v.type())
    {
     case value_type::NIL:
        lua_pushnil(L);
        break;
     case value_type::BOOLEAN:
        lua_pushboolean(L, cast<bool>(v));
        break;
     case value_type::NUMBER:
        lua_pushnumber(L, cast<number>(v));
        break;
     case value_type::STRING:
        lua_pushstring(L, cast<string>(v).c_str());
        break;
     case value_type::BLOB:
        push_boxed_data(L, cast<blob>(v));
        break;
     case value_type::LIST:
      {
        value_list const& list = cast<value_list>(v);
        int n_entries = boost::numeric_cast<int>(list.size());
        lua_createtable(L, n_entries, 0);
        for (int i = 0; i != n_entries; ++i)
        {
            push_to_lua(L, list[i]);
            lua_rawseti(L, -2, i + 1);
        }
        break;
      }
     case value_type::RECORD:
      {
        value_map const& map = cast<value_map>(v);
        int n_entries = boost::numeric_cast<int>(map.size());
        lua_createtable(L, 0, n_entries);
        for (value_map::const_iterator i = map.begin(); i != map.end(); ++i)
        {
            push_to_lua(L, i->second);
            lua_setfield(L, -2, i->first.c_str());
        }
        break;
      }
    }
}

static int function_invoker(lua_State* L)
{
    int n_args = lua_gettop(L);
    value_list values(n_args);
    for (int i = 0; i != n_args; ++i)
        get_from_lua(&values[i], L, i + 1);
    api_function_ptr const& f =
        get_boxed_data<api_function_ptr>(L, lua_upvalueindex(1));
    null_check_in check_in;
    null_progress_reporter reporter;
    push_to_lua(L, f->execute(check_in, reporter, values));
    return 1;
}

void register_lua_functions(lua_State* L, string const& package_name,
    api_function_list const& functions)
{
    // This duplicates what luaL_register does.
    // We can't use luaL_register itself because we register closures.
    luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
    lua_getfield(L, -1, package_name.c_str());
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        if (luaL_findtable(L, LUA_GLOBALSINDEX, package_name.c_str(), 1)
            != NULL)
        {
            luaL_error(L, "name conflict for module " LUA_QS,
                package_name.c_str());
        }
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, package_name.c_str());
    }
    lua_remove(L, -2);
    // At this point, the only value on the stack is the table that we want
    // to fill with functions.

    for (auto const& i : functions)
    {
        push_boxed_data(L, i);
        lua_pushcclosure(L, function_invoker, 1);
        string name = i->info.name + "_" + i->info.version;
        lua_setfield(L, -2, name.c_str());
        if (!i->info.is_legacy)
        {
            push_boxed_data(L, i);
            lua_pushcclosure(L, function_invoker, 1);
            string name = i->info.name;
            lua_setfield(L, -2, name.c_str());
        }
    }

    lua_pop(L, 1);
}

void register_lua_api(lua_State* L, string const& package_name,
    api_info const& api)
{
    for (auto const& i : api.modules)
        register_lua_functions(L, package_name, i.second.functions);
}

}
