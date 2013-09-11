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
	const char *		m_name;		//! name of library to load
	lua_CFunction		m_func;		//! C function for it
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
	: m_error(error)
	, m_which(which)
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

void Plugin::call(const string &func,
		  shared_ptr<Server> server,
		  vector<string> params)
{
	lua_State *L = m_state.get();

	lua_getglobal(L, func.c_str());
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 1);
	} else {
		int np = 0;

		if (server) {
			LuaServer::pushObject(L, server);
			++ np;
		}

		for (const string &a : params) {
			lua_pushstring(L, a.c_str());
			++ np;
		}

		if (lua_pcall(L, np, 0, 0) != LUA_OK) {
			string error = lua_tostring(L, -1);
			lua_pop(L, 1);

			throw ErrorException(m_name, error);
		}
	}
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

	m_state = LuaState(luaL_newstate());

	// Load default library as it was done by require.
	for (const Library &l : libLua)
		Luae::require(m_state.get(), l.m_name, l.m_func, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (const Library &l : libIrccd)
		Luae::preload(m_state.get(), l.m_name, l.m_func);

	// Set the plugin home
	oss << Util::configUser();
	oss << m_name;

	m_home = oss.str();

	if (luaL_dofile(m_state.get(), path.c_str()) != LUA_OK) {
		m_error = lua_tostring(m_state.get(), -1);
		lua_pop(m_state.get(), 1);

		return false;
	}

	return true;
}

void Plugin::onCommand(shared_ptr<Server> server,
		       const string &channel,
		       const string &who,
		       const string &message)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(message);

	call("onCommand", server, params);
}

void Plugin::onConnect(shared_ptr<Server> server)
{
	call("onConnect", server);
}

void Plugin::onChannelNotice(shared_ptr<Server> server,
			     const string &nick,
			     const string &target,
			     const string &notice)
{
	vector<string> params;

	params.push_back(nick);
	params.push_back(target);
	params.push_back(notice);

	call("onChannelNotice", server, params);
}

void Plugin::onInvite(shared_ptr<Server> server,
		      const string &channel,
		      const string &who)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);

	call("onInvite", server, params);
}

void Plugin::onJoin(shared_ptr<Server> server,
		    const string &channel,
		    const string &nickname)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(nickname);

	call("onJoin", server, params);
}

void Plugin::onKick(shared_ptr<Server> server,
		    const string &channel,
		    const string &who,
		    const string &kicked,
		    const string &reason)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(kicked);
	params.push_back(reason);

	call("onKick", server, params);
}

void Plugin::onMessage(shared_ptr<Server> server,
		       const string &channel,
		       const string &who,
		       const string &message)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(message);

	call("onMessage", server, params);
}

void Plugin::onMode(shared_ptr<Server> server,
		    const string &channel,
		    const string &who,
		    const string &mode,
		    const string &modeArg)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(mode);
	params.push_back(modeArg);

	call("onMode", server, params);
}

void Plugin::onNick(shared_ptr<Server> server,
		    const string &oldnick,
		    const string &newnick)
{
	vector<string> params;

	params.push_back(oldnick);
	params.push_back(newnick);

	call("onNick", server, params);
}

void Plugin::onNotice(shared_ptr<Server> server,
		      const string &nick,
		      const string &target,
		      const string &notice)
{
	vector<string> params;

	params.push_back(nick);
	params.push_back(target);
	params.push_back(notice);

	call("onNotice", server, params);
}

void Plugin::onPart(shared_ptr<Server> server,
		    const string &channel,
		    const string &who,
		    const string &reason)
{
	vector<string> params;
	
	params.push_back(channel);
	params.push_back(who);
	params.push_back(reason);

	call("onPart", server, params);
}

void Plugin::onQuery(shared_ptr<Server> server,
		     const string &who,
		     const string &message)
{
	vector<string> params;

	params.push_back(who);
	params.push_back(message);

	call("onQuery", server, params);
}

void Plugin::onReload()
{
	call("onReload");
}

void Plugin::onTopic(shared_ptr<Server> server,
		     const string &channel,
		     const string &who,
		     const string &topic)
{
	vector<string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(topic);

	call("onTopic", server, params);
}

void Plugin::onUserMode(shared_ptr<Server> server,
			const string &who,
			const string &mode)
{
	vector<string> params;

	params.push_back(who);
	params.push_back(mode);

	call("onUserMode", server, params);
}
