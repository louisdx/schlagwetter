#ifndef H_LUA
#define H_LUA


#ifndef USE_LUA
#define USE_LUA 1
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
extern bool LUA_READY;

inline void luaOpen()
{
  LUA = lua_open();
  luaL_openlibs(LUA);
  luabind::open(LUA);
}

inline void luaClose()
{
  lua_close(LUA);
  LUA_READY = false;
}

inline void luaLoadScripts(const std::string & scriptdir)
{
  std::string lua_script = scriptdir + "/script.lua";

  const int lua_res = luaL_loadfile(LUA, lua_script.c_str());

  switch (lua_res)
  {
  case 0:
    std::cout << "Lua script \"" << lua_script << "\" loaded successfully." << std::endl;
    LUA_READY = true;
    lua_pcall(LUA, 0, LUA_MULTRET, 0);
    break;
  case LUA_ERRFILE:
    std::cerr << "Failed to load script file \"" << lua_script << "\" (LUA_ERRFILE). Scripting support disabled." << std::endl;
    break;
  case LUA_ERRMEM:
    std::cerr << "Failed to allocate memory for script file \"" << lua_script << "\" (LUA_ERRMEM). Scripting support disabled." << std::endl;
    break;
  case LUA_ERRSYNTAX:
    std::cerr << "Failed while loading script file \"" << lua_script << "\" (LUA_ERRSYNTAX), syntax error. Scripting support disabled." << std::endl;
    break;
  }
}


#endif


#endif
