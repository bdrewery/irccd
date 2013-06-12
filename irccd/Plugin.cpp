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

DeferredCall::DeferredCall(DeferredType type, shared_ptr<Server> server, int ref)
	: m_type(type)
	, m_server(server)
	, m_ref(ref)
{
}

DeferredType DeferredCall::type() const
{
	return m_type;
}

std::shared_ptr<Server> DeferredCall::server() const
{
	return m_server;
}

void DeferredCall::addParam(const vector<string> & list)
{
	m_params.push_back(list);
}

void DeferredCall::execute(Plugin &plugin)
{
	(void)plugin;
#if 0
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
#endif
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

bool Plugin::loadLua(const string &path)
{
	m_state.openState();

	// Load default library as it was done by require.
	for (const Library &l : libLua)
		m_state.require(l.m_name, l.m_func, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (const Library &l : libIrccd)
		m_state.preload(l.m_name, l.m_func);

	if (!m_state.dofile(path)) {
		m_error = m_state.getError();
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

LuaState & Plugin::getState()
{
	return m_state;
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

#if 0
void Plugin::addDeferred(DeferredCall call)
{
	m_defcalls.push_back(call);
}

bool Plugin::hasDeferred(DeferredType type, shared_ptr<)
{
	for (const DeferredCall &c : m_defcalls)
		if (c.type() == type && c.server() == sv)
			return true;

	return false;

	return false;
}

DeferredCall & Plugin::getDeferred(DeferredType type, const Server *sv)
{
	for (DeferredCall &c : m_defcalls)
		if (c.type() == type && c.server() == sv)
			return c;

	throw out_of_range("not found");
}
#endif

void Plugin::removeDeferred(DeferredCall &dc)
{
	m_defcalls.erase(std::remove(m_defcalls.begin(), m_defcalls.end(), dc),
	    m_defcalls.end());
}

void Plugin::onCommand(shared_ptr<Server> server, const string &channel, const string &who, const string &message)
{
	if (m_state.getglobal("onCommand") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(message);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onConnect(shared_ptr<Server> server)
{
	if (m_state.getglobal("onConnect") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	
	if (!m_state.pcall(1, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onChannelNotice(shared_ptr<Server> server, const string &nick, const string &target, const string &notice)
{
	if (m_state.getglobal("onChannelNotice") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(nick);
	m_state.push(target);
	m_state.push(notice);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onInvite(shared_ptr<Server> server, const string &channel, const string &who)
{
	if (m_state.getglobal("onInvite") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onJoin(shared_ptr<Server> server, const string &channel, const string &nickname)
{
	if (m_state.getglobal("onJoin") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(nickname);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onKick(shared_ptr<Server> server, const string &channel, const string &who, const string &kicked, const string &reason)
{
	if (m_state.getglobal("onKick") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(kicked);
	m_state.push(reason);

	if (!m_state.pcall(5, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onMessage(shared_ptr<Server> server, const string &channel, const string &who, const string &message)
{
	if (m_state.getglobal("onMessage") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(message);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onMode(shared_ptr<Server> server, const string &channel, const string &who, const string &mode, const string &modeArg)
{
	if (m_state.getglobal("onMode") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(mode);
	m_state.push(modeArg);

	if (!m_state.pcall(5, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onNick(shared_ptr<Server> server, const string &oldnick, const string &newnick)
{
	if (m_state.getglobal("onNick") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(oldnick);
	m_state.push(newnick);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onNotice(shared_ptr<Server> server, const string &nick, const string &target, const string &notice)
{
	if (m_state.getglobal("onNotice") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(nick);
	m_state.push(target);
	m_state.push(notice);

	if (!m_state.pcall(4, 0))
		Logger::warn("plugin %s: %s", m_name.c_str(), m_state.getError().c_str());
}

void Plugin::onPart(shared_ptr<Server> server, const string &channel, const string &who, const string &reason)
{
	if (m_state.getglobal("onPart") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(reason);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onQuery(shared_ptr<Server> server, const string &who, const string &message)
{
	if (m_state.getglobal("onQuery") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(who);
	m_state.push(message);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onReload()
{
	if (m_state.getglobal("onReload") != LUA_TFUNCTION)
		return;

	if (!m_state.pcall(0, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onTopic(shared_ptr<Server> server, const string &channel, const string &who, const string &topic)
{
	if (m_state.getglobal("onTopic") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(topic);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_state.getError());
}

void Plugin::onUserMode(shared_ptr<Server> server, const string &who, const string &mode)
{
	if (m_state.getglobal("onUserMode") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(who);
	m_state.push(mode);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_state.getError());
}
