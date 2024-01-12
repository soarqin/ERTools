#include "window.h"

window_t* push_window(lua_State *L)
{
	window_t *window = (window_t*)lua_newuserdata(L, sizeof(window_t));
	luaL_getmetatable(L, WINDOW_T);
	lua_setmetatable(L, -2);
	return window;
}

void init_window(window_t * window, HWND handle, const TCHAR* title)
{
	window->handle = handle;
	GetWindowThreadProcessId(handle, &window->processId);
	if (!title)
		GetWindowText(handle, window->title, sizeof(window->title) / sizeof(TCHAR));
	else
		strncpy_s(window->title, sizeof(window->title), title, sizeof(window->title) / sizeof(TCHAR));
}

static const luaL_Reg window_meta[] = {
	{ NULL, NULL }
};
static const luaL_Reg window_methods[] = {
	{ NULL, NULL }
};
static udata_field_info window_getters[] = {
	{ "title", udata_field_get_string, offsetof(window_t, title) },
	{ "pid", udata_field_get_int, offsetof(window_t, processId) },
	{ NULL, NULL }
};
static udata_field_info window_setters[] = {
	{ NULL, NULL }
};

int register_window(lua_State *L)
{
	UDATA_REGISTER_TYPE_WITH_FIELDS(window, WINDOW_T)
}