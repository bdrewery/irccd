/*
 * Process.cpp -- Lua thread or plugin process
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "Lua/LuaIrccd.h"
#include "Lua/LuaLogger.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaPipe.h"
#include "Lua/LuaPlugin.h"
#include "Lua/LuaServer.h"
#include "Lua/LuaSocket.h"
#include "Lua/LuaThread.h"
#include "Lua/LuaUtil.h"

#include "Plugin.h"
#include "Process.h"

namespace irccd {

/* ---------------------------------------------------------
 * fields used for registring plugin or thread information
 * --------------------------------------------------------- */

const char *	Process::FieldName = "__process_name__";
const char *	Process::FieldHome = "__process_home__";

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

Process::Ptr Process::create()
{
	return std::shared_ptr<Process>(new Process);
}

void Process::initialize(Process::Ptr process,
			 const std::string &name,
			 const std::string &home)
{
	lua_pushlstring(*process, name.c_str(), name.length());
	lua_setfield(*process, LUA_REGISTRYINDEX, FieldName);

	lua_pushlstring(*process, home.c_str(), home.length());
	lua_setfield(*process, LUA_REGISTRYINDEX, FieldHome);
}

std::string Process::getName(lua_State *L)
{
	return Luae::requireField<std::string>(L, LUA_REGISTRYINDEX, FieldName);
}

std::string Process::getHome(lua_State *L)
{
	return Luae::requireField<std::string>(L, LUA_REGISTRYINDEX, FieldHome);
}

Process::operator lua_State *()
{
	return m_state;
}

} // !irccd
