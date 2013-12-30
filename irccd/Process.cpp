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
#include "Lua/LuaFS.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaPipe.h"
#include "Lua/LuaPlugin.h"
#include "Lua/LuaServer.h"
#include "Lua/LuaSocket.h"
#include "Lua/LuaSystem.h"
#include "Lua/LuaThread.h"
#include "Lua/LuaUtil.h"

#include "Plugin.h"
#include "Process.h"

namespace irccd {

/* ---------------------------------------------------------
 * fields used for registring plugin or thread information
 * --------------------------------------------------------- */

const char *	Process::FieldInfo = "__process_info__";

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

#if !defined(NDEBUG)
	{ "debug",			luaopen_debug		},
#endif

	/*
	 * There is no function for this one, but server object is passed
	 * through almost every function, so we load it for convenience
	 */
	{ "irccd.server",		luaopen_server		},
};

const Process::Libraries Process::irccdLibs = {
	{ "irccd",			luaopen_irccd		},
	{ "irccd.logger",		luaopen_logger		},
	{ "irccd.fs",			luaopen_fs		},
	{ "irccd.parser",		luaopen_parser		},
	{ "irccd.plugin",		luaopen_plugin		},
	{ "irccd.socket",		luaopen_socket		},
	{ "irccd.socket.address",	luaopen_socket_address	},
	{ "irccd.socket.listener",	luaopen_socket_listener	},
	{ "irccd.system",		luaopen_system		},
	{ "irccd.thread",		luaopen_thread		},
	{ "irccd.thread.pipe",		luaopen_thread_pipe	},
	{ "irccd.util",			luaopen_util		}
};

Process::Ptr Process::create()
{
	return std::shared_ptr<Process>(new Process);
}

void Process::initialize(Ptr process, const Info &info)
{
	auto setField = [&] (const std::string &which, const std::string &name) {
		lua_pushlstring(*process, which.c_str(), which.length());
		lua_setfield(*process, -2, name.c_str());
	};

	auto L = static_cast<lua_State *>(*process);

	LUA_STACK_CHECKBEGIN(L);

	/* Plugin information */
	lua_createtable(L, 0, 0);

	setField(info.name, "name");
	setField(info.path, "path");
	setField(info.home, "home");
	setField(info.author, "author");
	setField(info.comment, "comment");
	setField(info.version, "version");
	setField(info.license, "license");

	lua_setfield(L, LUA_REGISTRYINDEX, FieldInfo);

	LUA_STACK_CHECKEQUALS(L);
}

Process::Info Process::info(lua_State *L)
{
	Process::Info info;

	lua_getfield(L, LUA_REGISTRYINDEX, FieldInfo);
	if (lua_type(L, -1) != LUA_TTABLE)
		luaL_error(L, "uninitialized state");

	info.name = Luae::requireField<std::string>(L, -1, "name");
	info.path = Luae::requireField<std::string>(L, -1, "path");
	info.home = Luae::requireField<std::string>(L, -1, "home");
	info.author = Luae::requireField<std::string>(L, -1, "author");
	info.comment = Luae::requireField<std::string>(L, -1, "comment");
	info.version = Luae::requireField<std::string>(L, -1, "version");
	info.license = Luae::requireField<std::string>(L, -1, "license");

	return info;
}

Process::operator lua_State *()
{
	return m_state;
}

} // !irccd
