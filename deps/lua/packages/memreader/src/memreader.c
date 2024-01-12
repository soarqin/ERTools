#include "memreader.h"
#include "process.h"
#include "address.h"
#include "module.h"
#include "window.h"

#include <psapi.h>
#include <tlhelp32.h>

static int memreader_debug_privilege(lua_State *L)
{
	BOOL state = lua_toboolean(L, 1);
	HANDLE hToken = NULL;
	LUID luid;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		return push_last_error(L);
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
		return push_last_error(L);

	TOKEN_PRIVILEGES tokenPriv;
	tokenPriv.PrivilegeCount = 1;
	tokenPriv.Privileges[0].Luid = luid;
	tokenPriv.Privileges[0].Attributes = state ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tokenPriv, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
		return push_last_error(L);

	lua_pushboolean(L, TRUE);
	return 1;
}

static int memreader_process_iterator(lua_State *L)
{
	HANDLE handle = *(HANDLE*)lua_touserdata(L, lua_upvalueindex(1));
	static PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	BOOL success;
	if (lua_isnil(L, 2)) // control variable is nil on first iteration
		success = Process32First(handle, &pe32);
	else
		success = Process32Next(handle, &pe32);

	if (!success)
		return 0;

	lua_pushinteger(L, pe32.th32ProcessID);
	lua_pushstring(L, pe32.szExeFile);
	return 2;
}

static int memreader_snapshot_gc(lua_State *L)
{
	HANDLE handle = *(HANDLE*)lua_touserdata(L, 1);
	if (handle) CloseHandle(handle);
	return 0;
}

static int register_snapshot(lua_State *L)
{
	luaL_newmetatable(L, SNAPSHOT_T);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, memreader_snapshot_gc);
	lua_settable(L, -3);
	lua_pop(L, 1);
	return 0;
}

static int memreader_processes(lua_State *L)
{
	HANDLE* handlePtr = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE*));
	luaL_getmetatable(L, SNAPSHOT_T);
	lua_setmetatable(L, -2);

	*handlePtr = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (*handlePtr == INVALID_HANDLE_VALUE)
		return push_last_error(L);

	// memreader_process_iterator's upvalue is the HANDLE* userdata
	lua_pushcclosure(L, memreader_process_iterator, 1);
	return 1;
}

static int memreader_open_process(lua_State *L)
{
	DWORD processId = (int)luaL_checkinteger(L, 1);

	if (processId <= 0)
		return push_error(L, "invalid process id");

	HANDLE processHandle = OpenProcess(OPEN_PROCESS_FLAGS, 0, processId);

	if (!processHandle)
		return push_last_error(L);

	process_t *process = push_process(L);
	init_process(process, processId, processHandle);

	return 1;
}

static int memreader_find_window(lua_State *L)
{
	const char* windowName = luaL_checkstring(L, 1);
	HWND windowHandle = FindWindow(NULL, windowName);
	if (!windowHandle)
		return push_last_error(L);

	window_t* window = push_window(L);
	init_window(window, windowHandle, windowName);
	return 1;
}

static const luaL_Reg memreader_funcs[] = {
	{ "openprocess", memreader_open_process },
	{ "debugprivilege", memreader_debug_privilege },
	{ "processes", memreader_processes },
	{ "findwindow", memreader_find_window },
	{ NULL, NULL }
};

LUALIB_API int luaopen_memreader(lua_State *L)
{
	luaL_newlib(L, memreader_funcs);

	register_process(L);
	register_memaddress(L);
	register_module(L);
	register_window(L);
	register_snapshot(L);

	return 1;
}
