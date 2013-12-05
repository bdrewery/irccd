
#include "Lua/LuaIrccd.h"
#include "Lua/LuaLogger.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaPipe.h"
#include "Lua/LuaPlugin.h"
#include "Lua/LuaServer.h"
#include "Lua/LuaSocket.h"
#include "Lua/LuaThread.h"
#include "Lua/LuaUtil.h"

#include "Process.h"

namespace irccd {

/* ---------------------------------------------------------
 * fields used for registring plugin or thread information
 * --------------------------------------------------------- */

const char *	Process::FieldName = "__plugin_name__";
const char *	Process::FieldHome = "__plugin_home__";

/* --------------------------------------------------------
 * list of libraries to load
 * -------------------------------------------------------- */

const Process::Libraries Process::luaLibs = {
	{ "_G",				luaopen_base		},
	{ "io",				luaopen_io		},
	{ "math",			luaopen_math		},
	{ "package",			luaopen_package		},
	{ "string",			luaopen_string		},
	{ "table",			luaopen_table		},

	/*
	 * There is no function for this one, but server object is passed
	 * through almost every function, so we load it for convenience
	 */
	{ "irccd.server",		luaopen_server		},
};

const Process::Libraries Process::irccdLibs = {
	{ "irccd",			luaopen_irccd		},
	{ "irccd.logger",		luaopen_logger		},
	{ "irccd.parser",		luaopen_parser		},
	{ "irccd.plugin",		luaopen_plugin		},
	{ "irccd.socket",		luaopen_socket		},
	{ "irccd.socket.address",	luaopen_socket_address	},
	{ "irccd.socket.listener",	luaopen_socket_listener	},
	{ "irccd.thread",		luaopen_thread		},
	{ "irccd.thread.pipe",		luaopen_thread_pipe	},
	{ "irccd.util",			luaopen_util		}
};

std::string Process::getName(lua_State *L) noexcept
{
	return Luae::requireField<std::string>(L, LUA_REGISTRYINDEX, FieldName);
}

std::string Process::getHome(lua_State *L) noexcept
{
	return Luae::requireField<std::string>(L, LUA_REGISTRYINDEX, FieldHome);
}

Process::Ptr Process::create(LuaState &&state)
{
	return std::shared_ptr<Process>(new Process(std::move(state)));
}

void Process::initialize(lua_State *L,
			 const std::string &name,
			 const std::string &home)
{
	lua_pushlstring(L, name.c_str(), name.length());
	lua_setfield(L, LUA_REGISTRYINDEX, FieldName);

	lua_pushlstring(L, home.c_str(), home.length());
	lua_setfield(L, LUA_REGISTRYINDEX, FieldHome);
}

Process::Process(LuaState &&state)
	: m_state(std::move(state))
{
}

Process::operator lua_State *()
{
	puts("CALLED???");
	return m_state;
}

}
