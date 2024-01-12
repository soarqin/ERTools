#ifndef MEMREADER_UTILS_H
#define MEMREADER_UTILS_H

#include "memreader.h"

#define MEMREADER_METATABLE(t) "memreader." #t

int push_error(lua_State *L, const char* msg);
int push_last_error(lua_State *L);
const char* get_lua_string(lua_State *L, int index);

// Userdata Field Handling
int udata_field_get_int(lua_State *L, void *v);
int udata_field_set_int(lua_State *L, void *v);
int udata_field_get_string(lua_State *L, void *v);
int udata_field_set_string(lua_State *L, void *v);

typedef int(*udata_field_fn) (lua_State *L, void *v);

typedef const struct {
	const char *name;
	udata_field_fn func; // get or set function for type of member
	size_t offset; // offset of member within the struct
}  udata_field_info;

typedef udata_field_info* udata_field_reg;

void register_udata_fields(lua_State *L, udata_field_reg l);
int udata_field_proxy(lua_State *L);
int udata_field_index_handler(lua_State *L);
int udata_field_newindex_handler(lua_State *L);

/** 
Defines a register_<name>(lua_State *L) function that registers a userdata type
that has fields.

Expects the following to be defined:

luaL_Reg <name>_methods[]				// contains member methods
luaL_Reg <name>_meta[]					// contains metamethods
udata_field_info <name>_getters[]		// contains field getters
udata_field_info <name>_setters[]		// contains field setters
*/
#define UDATA_REGISTER_TYPE_WITH_FIELDS(name, mt)						\
																		\
	/* store methods in a table outside the metatable */				\
	luaL_newlib(L, name##_methods);										\
	int methods = lua_gettop(L);										\
																		\
	/* only put the metamethods in the metatable itself */				\
	luaL_newmetatable(L, mt);											\
	luaL_setfuncs(L, name##_meta, 0);									\
	int metatable = lua_gettop(L);										\
																		\
	lua_pushliteral(L, "__metatable");									\
	lua_pushvalue(L, methods);											\
	lua_rawset(L, metatable); /* metatable.__metatable = methods */		\
																		\
	lua_pushliteral(L, "__index");										\
	lua_pushvalue(L, metatable);										\
	register_udata_fields(L, name##_getters); /* put getters 
											in the metatable */			\
	lua_pushvalue(L, methods);											\
	/* push udata_field_index_handler with the 
	metatable and methods as upvalues */								\
	lua_pushcclosure(L, udata_field_index_handler, 2);					\
	lua_rawset(L, metatable); /* metatable.__index = 
				udata_field_index_handler(metatable, methods) */		\
																		\
	lua_pushliteral(L, "__newindex");									\
	lua_newtable(L); /* table for setters */							\
	register_udata_fields(L, name##_setters);							\
	/* push udata_field_newindex_handler with the 
	setters table as an upvalue */										\
	lua_pushcclosure(L, udata_field_newindex_handler, 1);				\
	lua_rawset(L, metatable); /* metatable.__newindex = 
						udata_field_newindex_handler(setters) */		\
																		\
	lua_pop(L, 2); /* pop metatable and methods table */				\
	return 0;

#endif