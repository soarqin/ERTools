#include "lprefix.h"

#include <stddef.h>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"

LUALIB_API int luaopen_cjson(lua_State *l);
LUALIB_API int luaopen_lfs(lua_State * L);
LUALIB_API int luaopen_memreader(lua_State *L);

/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const luaL_Reg loadedpackages[] = {
  {"cjson", luaopen_cjson},
  {"lfs", luaopen_lfs},
  {"memreader", luaopen_memreader},
  {NULL, NULL}
};


LUALIB_API void luaL_openpkgs (lua_State *L) {
  const luaL_Reg *lib;
  /* "require" functions from 'loadedpackages' and set results to global table */
  for (lib = loadedpackages; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }
}
