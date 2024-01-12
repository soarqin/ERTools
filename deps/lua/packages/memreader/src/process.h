#ifndef MEMREADER_PROCESS_H
#define MEMREADER_PROCESS_H

#include "memreader.h"

#define PROCESS_T MEMREADER_METATABLE(process)

typedef struct {
	DWORD pid;
	HANDLE handle;
	HMODULE module;
	TCHAR name[MAX_PATH];
	TCHAR path[MAX_PATH];
} process_t;

process_t* check_process(lua_State *L, int index);
process_t* push_process(lua_State *L);
void init_process(process_t* process, DWORD pid, HANDLE handle);

int register_process(lua_State *L);

#endif