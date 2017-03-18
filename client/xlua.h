// xlua.h: extensions to lua

#ifndef XLUA_H
#define XLUA_H

#include "lua.h"
#include "lauxlib.h"

int xlua_traceback (lua_State *L);

typedef struct xlua_type {
	const char* name;
	struct xlua_type* base;
} xlua_type;

// cast the userdata at stack index 'ud' to a user type,
// returning the userdata pointer on success.
void* xlua_tousertype(lua_State* L, int ud, xlua_type* type);

// cast the userdata at stack index 'ud' to a user type,
// returning the userdata pointer or NULL if nil/missing.
void* xlua_optusertype(lua_State* L, int ud, xlua_type* type);

// create a tag table in the lua registry.
int xlua_newtagtable (lua_State *L, xlua_type* type);

// register a new user type metatable in the lua registry.
int luax_newusertype(lua_State* L, xlua_type* type, lua_CFunction gcfunc);

// implements __tostring for user types.
int xlua_usertypetostring (lua_State *L);

// create a userdata of the specified user type, pushes the
// new userdata on the stack and returns its pointer.
void* xlua_newinstance(lua_State* L, size_t size, xlua_type* type);

#endif // XLUA_H
