#include "process.h"
#include "address.h"
#include "module.h"

#include <psapi.h>

process_t* check_process(lua_State *L, int index)
{
	process_t* proc = (process_t*)luaL_checkudata(L, index, PROCESS_T);
	return proc;
}

process_t* push_process(lua_State *L)
{
	process_t *proc = (process_t*)lua_newuserdata(L, sizeof(process_t));
	luaL_getmetatable(L, PROCESS_T);
	lua_setmetatable(L, -2);
	return proc;
}

void init_process(process_t* process, DWORD pid, HANDLE handle)
{
	process->pid = pid;
	process->handle = handle;

	DWORD cb;
	EnumProcessModules(process->handle, &process->module, sizeof(HMODULE), &cb);

	GetModuleBaseName(process->handle, NULL, process->name, sizeof(process->name) / sizeof(TCHAR));
	GetModuleFileNameEx(process->handle, NULL, process->path, sizeof(process->path) / sizeof(TCHAR));
}

static SIZE_T process_read_common(process_t* process, LPVOID address, LPVOID buff, SIZE_T size)
{
	SIZE_T numBytesRead;
	if (!ReadProcessMemory(process->handle, address, buff, size, &numBytesRead))
		return (SIZE_T)-1;
	return numBytesRead;
}

static int process_read(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);
	SIZE_T bytes = (SIZE_T)luaL_checkinteger(L, 3);

	if (bytes <= 256)
	{
		char buff[256];
		SIZE_T numBytesRead = process_read_common(process, address, buff, bytes);
		if (numBytesRead == (SIZE_T)-1)
			return push_last_error(L);
		lua_pushlstring(L, buff, numBytesRead);
	}
	else
	{
		char *buff = malloc(bytes);
		SIZE_T numBytesRead = process_read_common(process, address, buff, bytes);
		if (numBytesRead == (SIZE_T)-1)
		{
			free(buff);
			return push_last_error(L);
		}
		lua_pushlstring(L, buff, numBytesRead);
		free(buff);
	}
	return 1;
}

static int process_readbyte(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);

	char buff = 0;
	SIZE_T numBytesRead = process_read_common(process, address, &buff, sizeof(char));
	if (numBytesRead == (SIZE_T)-1)
		return push_last_error(L);

	lua_pushinteger(L, buff);
	return 1;
}

static int process_readshort(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);

	short buff = 0;
	SIZE_T numBytesRead = process_read_common(process, address, &buff, sizeof(short));
	if (numBytesRead == (SIZE_T)-1)
		return push_last_error(L);

	lua_pushinteger(L, buff);
	return 1;
}

static int process_readint(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);

	int buff = 0;
	SIZE_T numBytesRead = process_read_common(process, address, &buff, sizeof(int));
	if (numBytesRead == (SIZE_T)-1)
		return push_last_error(L);

	lua_pushinteger(L, buff);
	return 1;
}

static int process_readlong(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = (LPVOID)memaddress_checkptr(L, 2);

	long long buff = 0;
	SIZE_T numBytesRead = process_read_common(process, address, &buff, sizeof(long long));
	if (numBytesRead == (SIZE_T)-1)
		return push_last_error(L);

	lua_pushinteger(L, buff);
	return 1;
}

static int process_read_relative(lua_State *L)
{
	process_t* process = check_process(L, 1);
	LPVOID address = process->module;
	LONG_PTR offset = memaddress_checkptr(L, 2);
	SIZE_T bytes = (SIZE_T)luaL_checkinteger(L, 3);
	address = (LPVOID)((char*)address + offset);
	lua_settop(L, 1); // pop everything but the process_t
	memaddress_t* absAddress = push_memaddress(L);
	absAddress->ptr = address;
	lua_pushinteger(L, bytes);
	return process_read(L);
}

static int process_version(lua_State *L)
{
	process_t* process = check_process(L, 1);

	const char* exe = process->path;

	DWORD len = GetFileVersionInfoSize(exe, NULL);
	if (len == 0)
		return push_last_error(L);

	BYTE* pVersionResource = malloc(len);
	if (!GetFileVersionInfo(exe, 0, len, pVersionResource))
	{
		free(pVersionResource);
		return push_last_error(L);
	}

	UINT uLen;
	VS_FIXEDFILEINFO* ptFixedFileInfo;
	if (!VerQueryValue(pVersionResource, "\\", (LPVOID*)&ptFixedFileInfo, &uLen))
	{
		free(pVersionResource);
		return push_error(L, "unable to get version");
	}

	if (uLen == 0)
	{
		free(pVersionResource);
		return push_error(L, "unable to get version");
	}

	lua_createtable(L, 0, 2);
	lua_createtable(L, 0, 4);
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwFileVersionMS));
	lua_setfield(L, -2, "major");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwFileVersionMS));
	lua_setfield(L, -2, "minor");
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwFileVersionLS));
	lua_setfield(L, -2, "build");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwFileVersionLS));
	lua_setfield(L, -2, "revision");
	lua_setfield(L, -2, "file");

	lua_createtable(L, 0, 4);
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwProductVersionMS));
	lua_setfield(L, -2, "major");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwProductVersionMS));
	lua_setfield(L, -2, "minor");
	lua_pushinteger(L, HIWORD(ptFixedFileInfo->dwProductVersionLS));
	lua_setfield(L, -2, "build");
	lua_pushinteger(L, LOWORD(ptFixedFileInfo->dwProductVersionLS));
	lua_setfield(L, -2, "revision");
	lua_setfield(L, -2, "product");

	free(pVersionResource);

	return 1;
}

static int process_modules_iterator(lua_State *L)
{
	process_t* process = check_process(L, 1);
	HANDLE handle = *(HANDLE*)lua_touserdata(L, lua_upvalueindex(1));
	static MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);

	BOOL success;
	if (lua_isnil(L, 2)) // control variable is nil on first iteration
		success = Module32First(handle, &me32);
	else
		success = Module32Next(handle, &me32);

	if (!success)
		return 0;

	module_t* module = push_module(L);
	init_module(module, &me32);
	return 1;
}

static int process_modules(lua_State *L)
{
	process_t* process = check_process(L, 1);
	HANDLE* handlePtr = (HANDLE*)lua_newuserdata(L, sizeof(HANDLE*));
	luaL_getmetatable(L, SNAPSHOT_T);
	lua_setmetatable(L, -2);

	// From the WinAPI docs:
	// "If the function fails with ERROR_BAD_LENGTH when called with TH32CS_SNAPMODULE or TH32CS_SNAPMODULE32, 
	// call the function again until it succeeds."
	do
	{
		*handlePtr = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process->pid);
	}
	while (*handlePtr == INVALID_HANDLE_VALUE && GetLastError() == ERROR_BAD_LENGTH);

	if (*handlePtr == INVALID_HANDLE_VALUE)
		return push_last_error(L);

	// process_modules_iterator's upvalue is the HANDLE* userdata
	lua_pushcclosure(L, process_modules_iterator, 1);
	// push process_t to make it the invariant state
	lua_pushvalue(L, 1);
	return 2;
}

static int process_exit_code(lua_State *L)
{
	process_t* process = check_process(L, 1);
	DWORD exitCode;
	if (!GetExitCodeProcess(process->handle, &exitCode))
		return push_last_error(L);

	if (exitCode == STILL_ACTIVE)
		return 0;

	lua_pushinteger(L, exitCode);
	return 1;
}

static int process_gc(lua_State *L)
{
	process_t* process = check_process(L, 1);
	CloseHandle(process->handle);
	return 0;
}

static const luaL_Reg process_meta[] = {
	{ "__gc", process_gc },
	{ NULL, NULL }
};
static const luaL_Reg process_methods[] = {
	{ "version", process_version },
	{ "read", process_read },
	{ "readbyte", process_readbyte },
	{ "readshort", process_readshort },
	{ "readint", process_readint },
	{ "readlong", process_readlong },
	{ "readrelative", process_read_relative },
	{ "modules", process_modules },
	{ "exitcode", process_exit_code },
	{ NULL, NULL }
};
static udata_field_info process_getters[] = {
	{ "pid", udata_field_get_int, offsetof(process_t, pid) },
	{ "name", udata_field_get_string, offsetof(process_t, name) },
	{ "path", udata_field_get_string, offsetof(process_t, path) },
	{ "base", udata_field_get_memaddress, offsetof(process_t, module) },
	{ NULL, NULL }
};
static udata_field_info process_setters[] = {
	{ NULL, NULL }
};

int register_process(lua_State *L)
{
	UDATA_REGISTER_TYPE_WITH_FIELDS(process, PROCESS_T)
}