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
 * Plugin exception
 * -------------------------------------------------------- */

Plugin::ErrorException::ErrorException()
{
}

Plugin::ErrorException::ErrorException(const string &which, const string &error)
	: m_which(which)
	, m_error(error)
{
}

Plugin::ErrorException::~ErrorException()
{
}

string Plugin::ErrorException::which() const
{
	return m_which;
}

const char * Plugin::ErrorException::what() const throw()
{
	return m_error.c_str();
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
}

Plugin & Plugin::operator=(Plugin &&src)
{
	m_name = std::move(src.m_name);
	m_home = std::move(src.m_home);
	m_error = std::move(src.m_error);
	m_state = std::move(src.m_state);

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

void Plugin::onCommand(shared_ptr<Server> server, const string &channel, const string &who, const string &message)
{
	if (m_state.getglobal("onCommand") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);
	m_state.push(message);

	if (!m_state.pcall(4, 0))
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onConnect(shared_ptr<Server> server)
{
	if (m_state.getglobal("onConnect") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	
	if (!m_state.pcall(1, 0))
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onInvite(shared_ptr<Server> server, const string &channel, const string &who)
{
	if (m_state.getglobal("onInvite") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(who);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onJoin(shared_ptr<Server> server, const string &channel, const string &nickname)
{
	if (m_state.getglobal("onJoin") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(channel);
	m_state.push(nickname);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onNick(shared_ptr<Server> server, const string &oldnick, const string &newnick)
{
	if (m_state.getglobal("onNick") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(oldnick);
	m_state.push(newnick);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onQuery(shared_ptr<Server> server, const string &who, const string &message)
{
	if (m_state.getglobal("onQuery") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(who);
	m_state.push(message);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onReload()
{
	if (m_state.getglobal("onReload") != LUA_TFUNCTION)
		return;

	if (!m_state.pcall(0, 0))
		throw ErrorException(m_name, m_state.getError());
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
		throw ErrorException(m_name, m_state.getError());
}

void Plugin::onUserMode(shared_ptr<Server> server, const string &who, const string &mode)
{
	if (m_state.getglobal("onUserMode") != LUA_TFUNCTION)
		return;

	LuaServer::pushObject(m_state, server);
	m_state.push(who);
	m_state.push(mode);

	if (!m_state.pcall(3, 0))
		throw ErrorException(m_name, m_state.getError());
}
