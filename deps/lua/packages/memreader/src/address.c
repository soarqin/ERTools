#include "address.h"

memaddress_t* check_memaddress(lua_State *L, int index)
{
	memaddress_t* addr = (memaddress_t*)luaL_checkudata(L, index, MEMORY_ADDRESS_T);
	return addr;
}

memaddress_t* push_memaddress(lua_State *L)
{
	memaddress_t *addr = (memaddress_t*)lua_newuserdata(L, sizeof(memaddress_t));
	luaL_getmetatable(L, MEMORY_ADDRESS_T);
	lua_setmetatable(L, -2);
	return addr;
}

// for field access
int udata_field_get_memaddress(lua_State *L, void *v)
{
	memaddress_t *memaddress = push_memaddress(L);
	memaddress->ptr = *(LPVOID*)v;
	return 1;
}

LONG_PTR memaddress_checkptr(lua_State *L, int index)
{
	int t = lua_type(L, index);
	if (t == LUA_TNUMBER)
		return (LONG_PTR)lua_tointeger(L, index);
	else if (t == LUA_TUSERDATA)
	{
		memaddress_t* addr = check_memaddress(L, index);
		return (LONG_PTR)(addr->ptr);
	}
	return luaL_error(L, "attempt to perform arithmetic on %s and %s (expected number or second %s)",
		MEMORY_ADDRESS_T, luaL_typename(L, 2), MEMORY_ADDRESS_T);
}

static int memaddress_tostring(lua_State *L)
{
	memaddress_t* addr = check_memaddress(L, 1);
	lua_pushfstring(L, "%p", addr->ptr);
	return 1;
}

static int memaddress_add(lua_State *L)
{
	LONG_PTR a = memaddress_checkptr(L, 1);
	LONG_PTR b = memaddress_checkptr(L, 2);
	memaddress_t* result = push_memaddress(L);
	result->ptr = (LPVOID)(a + b);
	return 1;
}

static int memaddress_sub(lua_State *L)
{
	LONG_PTR a = memaddress_checkptr(L, 1);
	LONG_PTR b = memaddress_checkptr(L, 2);
	memaddress_t* result = push_memaddress(L);
	result->ptr = (LPVOID)(a - b);
	return 1;
}

static const luaL_Reg memaddress_meta[] = {
	{ "__tostring", memaddress_tostring },
	{ "__add", memaddress_add },
	{ "__sub", memaddress_sub },
	{ NULL, NULL }
};

int register_memaddress(lua_State *L)
{
	luaL_newmetatable(L, MEMORY_ADDRESS_T);
	luaL_setfuncs(L, memaddress_meta, 0);
	lua_pop(L, 1);
	return 0;
}