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

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"

#include "Lua/LuaIrccd.h"
#include "Lua/LuaLogger.h"
#include "Lua/LuaParser.h"
#include "Lua/LuaPlugin.h"
#include "Lua/LuaServer.h"
#include "Lua/LuaUtil.h"

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

/* --------------------------------------------------------
 * Deffered calls commands
 * -------------------------------------------------------- */

DeferredCall::DeferredCall()
{
}

DeferredCall::DeferredCall(DeferredType type, Server *server, int ref)
	: m_type(type)
	, m_server(server)
	, m_ref(ref)
{
}

DeferredType DeferredCall::type() const
{
	return m_type;
}

const Server * DeferredCall::server() const
{
	return m_server;
}

void DeferredCall::addParam(const vector<string> & list)
{
	m_params.push_back(list);
}

void DeferredCall::execute(Plugin &plugin)
{
	lua_rawgeti(plugin.getState(), LUA_REGISTRYINDEX, m_ref);
	int nparams = 0;

	switch (m_type) {
	case DeferredType::Names:
		lua_createtable(plugin.getState(), m_params.size(), m_params.size());

		for (size_t i = 0; i < m_params.size(); ++i) {
			lua_pushstring(plugin.getState(), m_params[i][0].c_str());
			lua_rawseti(plugin.getState(), -2, i + 1);
		}

		nparams = 1;
		break;
	default:
		break;
	}

	if (lua_pcall(plugin.getState(), nparams, 0, 0) != LUA_OK) {
		Logger::warn("plugin %s: %s",
		    plugin.getName().c_str(),
		    lua_tostring(plugin.getState(), -1));
	}

	luaL_unref(plugin.getState(), LUA_REGISTRYINDEX, m_ref);
}

bool DeferredCall::operator==(const DeferredCall &c1)
{
	return m_type == c1.m_type &&
	    m_server == c1.m_server &&
	    m_params == c1.m_params &&
	    m_ref == c1.m_ref;
}

/* --------------------------------------------------------
 * private methods and members
 * -------------------------------------------------------- */

void Plugin::callLua(const string &name, int nret, string fmt, ...)
{
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
		Logger::warn("plugin %s: %s", m_name.c_str(), lua_tostring(m_state.get(), -1));
		lua_pop(m_state.get(), 1);
	}
}

bool Plugin::loadLua(const string &path)
{
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

Plugin::Plugin(Plugin &&src)
{
	m_name = std::move(src.m_name);
	m_home = std::move(src.m_home);
	m_error = std::move(src.m_error);
	m_state = std::move(src.m_state);
	m_defcalls = std::move(src.m_defcalls);
}

Plugin & Plugin::operator=(Plugin &&src)
{
	m_name = std::move(src.m_name);
	m_home = std::move(src.m_home);
	m_error = std::move(src.m_error);
	m_state = std::move(src.m_state);
	m_defcalls = std::move(src.m_defcalls);

	return *this;
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

lua_State * Plugin::getState() const
{
	return m_state.get();
}

const string & Plugin::getError() const
{
	return m_error;
}

bool Plugin::open(const string &path)
{
	ostringstream oss;

	// Get the plugin directory, XDG config irccd + plugin name
	oss << Util::configDirectory();
	oss << m_name;

	m_home = oss.str();

	return loadLua(path);
}

void Plugin::addDeferred(DeferredCall call)
{
	m_defcalls.push_back(call);
}

bool Plugin::hasDeferred(DeferredType type, const Server *sv)
{
	for (const DeferredCall &c : m_defcalls)
		if (c.type() == type && c.server() == sv)
			return true;

	return false;
}

DeferredCall & Plugin::getDeferred(DeferredType type, const Server *sv)
{
	for (DeferredCall &c : m_defcalls)
		if (c.type() == type && c.server() == sv)
			return c;

	throw out_of_range("not found");
}

void Plugin::removeDeferred(DeferredCall &dc)
{
	m_defcalls.erase(std::remove(m_defcalls.begin(), m_defcalls.end(), dc),
	    m_defcalls.end());
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

void Plugin::onQuery(Server *server, const string &who, const string &message)
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
