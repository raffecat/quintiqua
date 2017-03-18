// xlua.c: extensions to lua

#include "xlua.h"

static char* tag_typeinfo = "tag_typeinfo";

int xlua_traceback (lua_State *L)
{
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

void* xlua_optusertype(lua_State* L, int ud, xlua_type* type)
{
	if (lua_isnoneornil(L, ud)) return NULL;
	else return lua_touserdata(L, ud);
}

// create a tag table in the lua registry
int xlua_newtagtable (lua_State *L, xlua_type* type) {
	lua_pushlightuserdata(L, type);
	lua_rawget(L, LUA_REGISTRYINDEX);  /* get registry[tag] */
	if (!lua_isnil(L, -1))  /* name already in use? */
		return 0;  /* leave previous value on top, but return 0 */
	lua_pop(L, 1);
	lua_newtable(L);  /* create table */
	lua_pushlightuserdata(L, type);
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);  /* registry[tag] = table */
	return 1; /* created flag */
}

int luax_newusertype(lua_State* L, xlua_type* type, lua_CFunction gcfunc) {
	xlua_newtagtable(L, type); // push t = reg[tag] = {}
	lua_pushlightuserdata(L, tag_typeinfo); // push k = typeinfo
	lua_pushlightuserdata(L, type); // push v = tag
	lua_rawset(L, -3); // t[k] = v; pop v,k
	lua_pushcfunction(L, gcfunc); // push f = gcfunc
	lua_setfield(L, -2, "__gc");  // t.__gc = f; pop f
	lua_pop(L, 1); // pop t
	return 0;
}

int xlua_usertypetostring (lua_State *L) {
	void* p = lua_touserdata(L, 1);
	xlua_type* type = *(xlua_type**) p;
	lua_pushfstring(L, "%s<%p>", type->name, p);
	return 1;
}

void* xlua_newinstance(lua_State* L, size_t size, xlua_type* type)
{
	void* p = lua_newuserdata(L, size); // push u
	lua_pushlightuserdata(L, type); // push k = tag
	lua_rawget(L, LUA_REGISTRYINDEX); // pop k; push m = reg[k]
	if (!lua_istable(L, -1)) luaL_error(L, "bad type");
	lua_setmetatable(L, -2); // meta(u) = m; pop m
	return p; // NB. u remains on the stack
}

/*
// apply a metatable that specifies weak keys
int xlua_makeweak(lua_State* L) {
	lua_createtable(L, 0, 1); // mt
	lua_pushlstring(L, "k", 1); // weak keys
	lua_setfield(L, -2, "__mode");
	lua_setmetatable(L, -2); // setmetatable(arg1, mt)
	return 1; // return arg1
}

// create a tag table with weak keys
int xlua_newreftable(lua_State* L, void* tag) {
	int created = xlua_newtagtable(L, tag);
	if (created) xlua_makeweak(L);
	return created; // created flag
}
*/
