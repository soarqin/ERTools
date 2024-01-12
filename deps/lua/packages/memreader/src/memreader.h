#ifndef MEMREADER_H
#define MEMREADER_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM < 502
#ifndef luaL_newlib
# define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif
# define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
#endif

#ifndef WINVER
#define WINVER 0x501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x501
#endif
#define PSAPI_VERSION 1
#include <windows.h>
#include <tchar.h>
#include <assert.h>

#define OPEN_PROCESS_FLAGS PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ
#define MAX_PROCESSES 1024
#define MAX_MODULES 1024

#include "utils.h"

#define SNAPSHOT_T MEMREADER_METATABLE(snapshot)

#endif