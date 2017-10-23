#ifndef CRADLE_EXTERNAL_LUA_HPP
#define CRADLE_EXTERNAL_LUA_HPP

extern "C" {

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

}

#ifdef _MSC_VER

#pragma comment (lib, "lua51.lib")

#endif

#endif
