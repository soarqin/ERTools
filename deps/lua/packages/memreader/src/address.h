#ifndef MEMREADER_ADDRESS_H
#define MEMREADER_ADDRESS_H

#include "memreader.h"

#define MEMORY_ADDRESS_T MEMREADER_METATABLE(address)

typedef struct {
	LPVOID ptr;
} memaddress_t;

memaddress_t* check_memaddress(lua_State *L, int index);
memaddress_t* push_memaddress(lua_State *L);
int udata_field_get_memaddress(lua_State *L, void *v);

LONG_PTR memaddress_checkptr(lua_State *L, int index);

int register_memaddress(lua_State *L);

#endif