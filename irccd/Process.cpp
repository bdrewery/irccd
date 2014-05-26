/*
 * Process.cpp -- Lua thread or plugin process
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include <config.h>

#include "lua/LuaIrccd.h"
#include "lua/LuaLogger.h"
#include "lua/LuaFS.h"
#include "lua/LuaParser.h"
#include "lua/LuaPipe.h"
#include "lua/LuaPlugin.h"
#include "lua/LuaRule.h"
#include "lua/LuaServer.h"
#include "lua/LuaSocket.h"
#include "lua/LuaSystem.h"
#include "lua/LuaThread.h"
#include "lua/LuaUtf8.h"
#include "lua/LuaUtil.h"

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
	{ "irccd.rule",			luaopen_rule		},
	{ "irccd.socket",		luaopen_socket		},
#if defined(COMPAT_1_1)
	{ "irccd.socket.address",	luaopen_socket_address	},
#endif
	{ "irccd.socket.listener",	luaopen_socket_listener	},
	{ "irccd.system",		luaopen_system		},
	{ "irccd.thread",		luaopen_thread		},
	{ "irccd.thread.pipe",		luaopen_thread_pipe	},
	{ "irccd.utf8",			luaopen_utf8		},
	{ "irccd.util",			luaopen_util		}
};

Process::Ptr Process::create()
{
	/*
	 * Use the std::shared_ptr constructor because std::make_shared
	 * needs a default constructor which is private for Process.
	 */
	return std::shared_ptr<Process>(new Process);
}

void Process::initialize(Ptr process, const Info &info)
{
	auto L = static_cast<lua_State *>(*process);

	LUAE_STACK_CHECKBEGIN(L);

	/* Plugin information */
	LuaeTable::create(L);
	LuaeTable::set(L, -1, "name", info.name);
	LuaeTable::set(L, -1, "path", info.path);
	LuaeTable::set(L, -1, "home", info.home);
	LuaeTable::set(L, -1, "author", info.author);
	LuaeTable::set(L, -1, "comment", info.comment);
	LuaeTable::set(L, -1, "version", info.version);
	LuaeTable::set(L, -1, "license", info.license);
	Luae::setfield(L, LUA_REGISTRYINDEX, FieldInfo);

	LUAE_STACK_CHECKEQUALS(L);
}

Process::Info Process::info(lua_State *L)
{
	LUAE_STACK_CHECKBEGIN(L);
	Process::Info info;

	Luae::getfield(L, LUA_REGISTRYINDEX, FieldInfo);
	if (Luae::type(L, -1) != LUA_TTABLE)
		Luae::error(L, "uninitialized state");

	info.name = LuaeTable::require<std::string>(L, -1, "name");
	info.path = LuaeTable::require<std::string>(L, -1, "path");
	info.home = LuaeTable::require<std::string>(L, -1, "home");
	info.author = LuaeTable::require<std::string>(L, -1, "author");
	info.comment = LuaeTable::require<std::string>(L, -1, "comment");
	info.version = LuaeTable::require<std::string>(L, -1, "version");
	info.license = LuaeTable::require<std::string>(L, -1, "license");

	Luae::pop(L);
	LUAE_STACK_CHECKEQUALS(L);

	return info;
}

Process::operator lua_State *()
{
	return m_state;
}

} // !irccd
