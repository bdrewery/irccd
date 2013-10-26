#ifndef _LUA_THREAD_H_
#define _LUA_THREAD_H_

#include <lua.hpp>

namespace irccd
{

int luaopen_thread(lua_State *L);

} // !irccd

#endif // !_LUA_THREAD_H_
