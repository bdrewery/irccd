#ifndef _LUA_PIPE_H_
#define _LUA_PIPE_H_

#include <lua.hpp>

namespace irccd {

int luaopen_thread_pipe(lua_State *L);

} // !irccd

#endif // !_LUA_PIPE_H_
