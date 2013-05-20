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

#include <libgen.h>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"
#include "LuaParser.h"
#include "LuaPlugin.h"
#include "LuaServer.h"
#include "LuaUtil.h"

using namespace irccd;
using namespace std;

/* --------------------------------------------------------
 * list of libraries to load
 * -------------------------------------------------------- */

struct Library {
	const char *	m_name;		//! name of library to load
	lua_CFunction	m_func;		//! C function for it
};

static const Library libLua[] = {
	{ "_G",		luaopen_base	},
	{ "io",		luaopen_io	},
	{ "math",	luaopen_math	},
	{ "package",	luaopen_package	},
	{ "string",	luaopen_string	},
	{ "table",	luaopen_table	},
	{ "server",	luaopen_server	},
};

static const Library libIrccd[] = {
	{ "parser",	luaopen_parser	},
	{ "plugin",	luaopen_plugin	},
	{ "util",	luaopen_util	}
};

/* --------------------------------------------------------
 * private methods and members
 * -------------------------------------------------------- */

void Plugin::callLua(const string &name, int nret, string fmt, ...)
{
	va_list ap;
	int count = 0;

	va_start(ap, fmt);
	lua_getglobal(m_state, name.c_str());
	if (lua_type(m_state, -1) != LUA_TFUNCTION)
		return;

	for (size_t i = 0; i < fmt.length(); ++i) {
		switch (fmt[i]) {
		case 'S':
			LuaServer::pushObject(m_state, va_arg(ap, Server *));
			++ count;
			break;
		case 'i':
			lua_pushinteger(m_state, va_arg(ap, int));
			++ count;
			break;
		case 's':
			lua_pushstring(m_state, va_arg(ap, const char *));
			++ count;
			break;
		default:
			break;
		}
	}

	if (lua_pcall(m_state, count, nret, 0) != LUA_OK) {
		Logger::warn("error in plugin: %s", lua_tostring(m_state, -1));
		lua_pop(m_state, 1);
	}
}

bool Plugin::loadLua(const std::string &path)
{
	m_state = luaL_newstate();

	// Load default library as it was done by require.
	for (const Library &l : libLua)
		(void)luaL_requiref(m_state, l.m_name, l.m_func, 1);

	// Put external modules in package.preload so user
	// will need require (modname)
	lua_getglobal(m_state, "package");
	lua_getfield(m_state, -1, "preload");
	for (const Library &l : libIrccd) {
		lua_pushcfunction(m_state, l.m_func);
		lua_setfield(m_state, -2, l.m_name);
	}
	lua_pop(m_state, 2);

	if (luaL_dofile(m_state, path.c_str())) {
		if (lua_type(m_state, -1) == LUA_TSTRING)
			m_error = lua_tostring(m_state, -1);
		lua_pop(m_state, 1);
		return false;
	}

	return true;
}

/* --------------------------------------------------------
 * public methods and members
 * -------------------------------------------------------- */

Plugin::Plugin(void)
{
}

Plugin::~Plugin(void)
{
	lua_close(m_state);
}

const string & Plugin::getName(void) const
{
	return m_name;
}
const string & Plugin::getHome(void) const
{
	return m_home;
}

lua_State * Plugin::getState(void) const
{
	return m_state;
}

const string & Plugin::getError(void) const
{
	return m_error;
}

bool Plugin::open(const std::string &path)
{
	ostringstream oss;

	// Get plugin name and extract .lua from it
	m_name = basename(path.c_str());
	size_t pos;
	if ((pos = m_name.find(".lua")) != string::npos)
		m_name = m_name.substr(0, pos);

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

void Plugin::onInvite(Server *server, const std::string &channel, const std::string &who)
{
	callLua("onInvite", 0, "S s s", server, channel.c_str(), who.c_str());
}

void Plugin::onJoin(Server *server, const string &channel, const string &nickname)
{
	callLua("onJoin", 0, "S s s", server, channel.c_str(), nickname.c_str());
}

void Plugin::onMessage(Server *server, const string &channel, const string &who, const string &message)
{
	callLua("onMessage", 0, "S s s s", server, channel.c_str(), who.c_str(), message.c_str());
}

void Plugin::onNick(Server *server, const string &oldnick, const string &newnick)
{
	callLua("onNick", 0, "S s s", server, oldnick.c_str(), newnick.c_str());
}

void Plugin::onPart(Server *server, const string &channel, const string &who, const string reason)
{
	callLua("onPart", 0, "S s s s", server, channel.c_str(), who.c_str(), reason.c_str());
}
