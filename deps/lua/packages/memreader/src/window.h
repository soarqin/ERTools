#ifndef MEMREADER_WINDOW_H
#define MEMREADER_WINDOW_H

#include "memreader.h"

#define WINDOW_T MEMREADER_METATABLE(window)

typedef struct {
	HWND handle;
	DWORD processId;
	TCHAR title[512];
} window_t;

void init_window(window_t * window, HWND handle, const TCHAR* title);
int register_window(lua_State *L);
window_t* push_window(lua_State *L);

#endif