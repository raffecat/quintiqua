// Lua includes and binding utils.

#ifndef PLAYER_LUA_BIND
#define PLAYER_LUA_BIND

#include "lua.h"

int report (lua_State *L, int status);
void* checklightuserdata (lua_State *L, int narg, const char* tname);
HWND checkhwnd (lua_State *L, int narg);

#endif // PLAYER_LUA_BIND
