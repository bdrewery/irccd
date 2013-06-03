/*
 * Plugin.cpp -- irccd Lua plugin interface
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#include <sstream>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"

#if defined(WITH_LUA)
#  include "Lua/LuaIrccd.h"
#  include "Lua/LuaLogger.h"
#  include "Lua/LuaParser.h"
#  include "Lua/LuaPlugin.h"
#  include "Lua/LuaServer.h"
#  include "Lua/LuaUtil.h"
#endif

using namespace irccd;
using namespace std;

/* --------------------------------------------------------
 * list of libraries to load
 * -------------------------------------------------------- */

#if defined(WITH_LUA)

struct Library {
	const char *	m_name;		//! name of library to load
	lua_CFunction	m_func;		//! C function for it
};

static const Library libLua[] = {
	{ "_G",			luaopen_base	},
	{ "io",			luaopen_io	},
	{ "math",		luaopen_math	},
	{ "package",		luaopen_package	},
	{ "string",		luaopen_string	},
	{ "table",		luaopen_table	},

	/*
	 * There is no function for this one, but server object is passed
	 * through almost every function, so we load it for convenience
	 */
	{ "irccd.server",	luaopen_server	},
};

static const Library libIrccd[] = {
	{ "irccd",		luaopen_irccd	},
	{ "irccd.logger",	luaopen_logger	},
	{ "irccd.parser",	luaopen_parser	},
	{ "irccd.plugin",	luaopen_plugin	},
	{ "irccd.util",		luaopen_util	}
};
#endif

/* --------------------------------------------------------
 * private methods and members
 * -------------------------------------------------------- */

void Plugin::callLua(const string &name, int nret, string fmt, ...)
{
#if defined(WITH_LUA)
	va_list ap;
	int count = 0;

	va_start(ap, fmt);
	lua_getglobal(m_state.get(), name.c_str());
	if (lua_type(m_state.get(), -1) != LUA_TFUNCTION)
		return;

	for (size_t i = 0; i < fmt.length(); ++i) {
		switch (fmt[i]) {
		case 'S':
			LuaServer::pushObject(m_state.get(), va_arg(ap, Server *));
			++ count;
			break;
		case 'i':
			lua_pushinteger(m_state.get(), va_arg(ap, int));
			++ count;
			break;
		case 's':
			lua_pushstring(m_state.get(), va_arg(ap, const char *));
			++ count;
			break;
		default:
			break;
		}
	}

	if (lua_pcall(m_state.get(), count, nret, 0) != LUA_OK) {
		Logger::warn("error in plugin: %s", lua_tostring(m_state.get(), -1));
		lua_pop(m_state.get(), 1);
	}
#else
	(void)name;
	(void)nret;
	(void)fmt;
#endif
}

bool Plugin::loadLua(const std::string &path)
{
#if defined(WITH_LUA)
	m_state = unique_ptr<lua_State, LuaDeleter>(luaL_newstate());

	// Load default library as it was done by require.
	for (const Library &l : libLua)
		(void)luaL_requiref(m_state.get(), l.m_name, l.m_func, 1);

	// Put external modules in package.preload so user
	// will need require (modname)
	lua_getglobal(m_state.get(), "package");
	lua_getfield(m_state.get(), -1, "preload");
	for (const Library &l : libIrccd) {
		lua_pushcfunction(m_state.get(), l.m_func);
		lua_setfield(m_state.get(), -2, l.m_name);
	}
	lua_pop(m_state.get(), 2);

	if (luaL_dofile(m_state.get(), path.c_str())) {
		if (lua_type(m_state.get(), -1) == LUA_TSTRING)
			m_error = lua_tostring(m_state.get(), -1);
		lua_pop(m_state.get(), 1);
		return false;
	}

	return true;
#else
	(void)path;

	return true;
#endif
}

/* --------------------------------------------------------
 * public methods and members
 * -------------------------------------------------------- */

Plugin::Plugin()
{
}

Plugin::Plugin(const string &name)
	:m_name(name)
{
}

Plugin::~Plugin()
{
}

const string & Plugin::getName() const
{
	return m_name;
}
const string & Plugin::getHome() const
{
	return m_home;
}

#if defined(WITH_LUA)
lua_State * Plugin::getState() const
{
	return m_state.get();
}
#endif

const string & Plugin::getError() const
{
	return m_error;
}

bool Plugin::open(const std::string &path)
{
	ostringstream oss;

	// Get the plugin directory, XDG config irccd + plugin name
	oss << Util::configDirectory();
	oss << m_name;

	m_home = oss.str();

	return loadLua(path);
}

void Plugin::onCommand(Server *server, const string &channel, const string &who, const string &message)
{
	callLua("onCommand", 0, "S s s s", server, channel.c_str(), who.c_str(), message.c_str());
}

void Plugin::onConnect(Server *server)
{
	callLua("onConnect", 0, "S", server);
}

void Plugin::onChannelNotice(Server *server, const string &nick, const string &target, const string &notice)
{
	callLua("onChannelNotice", 0, "S s s s", server, nick.c_str(), target.c_str(), notice.c_str());
}

void Plugin::onInvite(Server *server, const string &channel, const string &who)
{
	callLua("onInvite", 0, "S s s", server, channel.c_str(), who.c_str());
}

void Plugin::onJoin(Server *server, const string &channel, const string &nickname)
{
	callLua("onJoin", 0, "S s s", server, channel.c_str(), nickname.c_str());
}

void Plugin::onKick(Server *server, const string &channel, const string &who, const string &kicked, const string &reason)
{
	callLua("onKick", 0, "S s s s s", server, channel.c_str(), who.c_str(), kicked.c_str(), reason.c_str());
}

void Plugin::onMessage(Server *server, const string &channel, const string &who, const string &message)
{
	callLua("onMessage", 0, "S s s s", server, channel.c_str(), who.c_str(), message.c_str());
}

void Plugin::onMode(Server *server, const string &channel, const string &who, const string &mode, const string &modeArg)
{
	callLua("onMode", 0, "S s s s s", server, channel.c_str(), who.c_str(), mode.c_str(), modeArg.c_str());
}

void Plugin::onNick(Server *server, const string &oldnick, const string &newnick)
{
	callLua("onNick", 0, "S s s", server, oldnick.c_str(), newnick.c_str());
}

void Plugin::onNotice(Server *server, const string &nick, const string &target, const string &notice)
{
	callLua("onNotice", 0, "S s s s", server, nick.c_str(), target.c_str(), notice.c_str());
}

void Plugin::onPart(Server *server, const string &channel, const string &who, const string &reason)
{
	callLua("onPart", 0, "S s s s", server, channel.c_str(), who.c_str(), reason.c_str());
}

void Plugin::onQuery(Server *server, const std::string &who, const std::string &message)
{
	callLua("onQuery", 0, "S s s", server, who.c_str(), message.c_str());
}

void Plugin::onReload()
{
	callLua("onReload", 0, "");
}

void Plugin::onTopic(Server *server, const string &channel, const string &who, const string &topic)
{
	callLua("onTopic", 0, "S s s s", server, channel.c_str(), who.c_str(), topic.c_str());
}

void Plugin::onUserMode(Server *server, const string &who, const string &mode)
{
	callLua("onUserMode", 0, "S s s", server, who.c_str(), mode.c_str());
}
