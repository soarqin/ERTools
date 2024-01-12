#ifndef MEMREADER_MODULE_H
#define MEMREADER_MODULE_H

#include "memreader.h"
#include <tlhelp32.h>

#define MODULE_T MEMREADER_METATABLE(module)

typedef struct {
	HMODULE handle;
	TCHAR name[MAX_MODULE_NAME32 + 1];
	TCHAR path[MAX_PATH];
	DWORD size;
} module_t;

void init_module(module_t * module, MODULEENTRY32 * me32);
int register_module(lua_State *L);
module_t* push_module(lua_State *L);

#endif