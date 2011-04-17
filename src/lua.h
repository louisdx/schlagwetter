#ifndef H_LUA
#define H_LUA


#ifndef USE_LUA
#define USE_LUA 0
#endif

#if USE_LUA > 0

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <luabind/luabind.hpp>

extern lua_State * LUA;

#endif


#endif
