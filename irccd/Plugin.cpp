/*
 * Plugin.cpp -- irccd Lua plugin interface
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
#include "Lua/LuaPipe.h"
#include "Lua/LuaPlugin.h"
#include "Lua/LuaServer.h"
#include "Lua/LuaSocket.h"
#include "Lua/LuaThread.h"
#include "Lua/LuaUtil.h"

namespace irccd
{

/* --------------------------------------------------------
 * list of libraries to load
 * -------------------------------------------------------- */

const Plugin::Libraries Plugin::luaLibs = {
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

const Plugin::Libraries Plugin::irccdLibs = {
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

/* --------------------------------------------------------
 * Plugin exception
 * -------------------------------------------------------- */

Plugin::ErrorException::ErrorException()
{
}

Plugin::ErrorException::ErrorException(const std::string &which, const std::string &error)
	: m_error(error)
	, m_which(which)
{
}

std::string Plugin::ErrorException::which() const
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

void Plugin::call(const std::string &func,
		  Server::Ptr server,
		  std::vector<std::string> params)
{
	lua_State *L = m_state;

	lua_getglobal(L, func.c_str());
	if (lua_type(L, -1) != LUA_TFUNCTION)
		lua_pop(L, 1);
	else
	{
		int np = 0;

		if (server)
		{
			LuaServer::pushObject(L, server);
			++ np;
		}

		for (const std::string &a : params)
		{
			lua_pushstring(L, a.c_str());
			++ np;
		}

		if (lua_pcall(L, np, 0, 0) != LUA_OK)
		{
			std::string error = lua_tostring(L, -1);
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

Plugin::Plugin(const std::string &name,
	       const std::string &path)
	: m_name(name)
	, m_path(path)
{
	m_state = std::move(LuaState(luaL_newstate()));
}

const std::string &Plugin::getName() const
{
	return m_name;
}

const std::string &Plugin::getHome() const
{
	return m_home;
}

LuaState &Plugin::getState()
{
	return m_state;
}

const std::string & Plugin::getError() const
{
	return m_error;
}

bool Plugin::open()
{
	// Load default library as it was done by require.
	for (const Library &l : luaLibs)
		Luae::require(m_state, l.m_name, l.m_func, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (const Library &l : irccdLibs)
		Luae::preload(m_state, l.m_name, l.m_func);

	// Find the home directory for the plugin
	m_home = Util::findPluginHome(m_name);

	if (luaL_dofile(m_state, m_path.c_str()) != LUA_OK)
	{
		m_error = lua_tostring(m_state, -1);
		lua_pop(m_state, 1);

		return false;
	}

	// Do a initial load
	try
	{
		onLoad();
	}
	catch (ErrorException ex)
	{
		m_error = ex.what();
		return false;
	}

	return true;
}

void Plugin::addThread(Thread::Ptr th)
{
	m_threads.push_back(th);
}

Plugin::ThreadList &Plugin::getThreads()
{
	return m_threads;
}

void Plugin::onCommand(Server::Ptr server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(message);

	call("onCommand", server, params);
}

void Plugin::onConnect(Server::Ptr server)
{
	call("onConnect", server);
}

void Plugin::onChannelNotice(Server::Ptr server,
			     const std::string &nick,
			     const std::string &target,
			     const std::string &notice)
{
	std::vector<std::string> params;

	params.push_back(nick);
	params.push_back(target);
	params.push_back(notice);

	call("onChannelNotice", server, params);
}

void Plugin::onInvite(Server::Ptr server,
		      const std::string &channel,
		      const std::string &who)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);

	call("onInvite", server, params);
}

void Plugin::onJoin(Server::Ptr server,
		    const std::string &channel,
		    const std::string &nickname)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(nickname);

	call("onJoin", server, params);
}

void Plugin::onKick(Server::Ptr server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &kicked,
		    const std::string &reason)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(kicked);
	params.push_back(reason);

	call("onKick", server, params);
}

void Plugin::onLoad()
{
	call("onLoad");
}

void Plugin::onMessage(Server::Ptr server,
		       const std::string &channel,
		       const std::string &who,
		       const std::string &message)

{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(message);

	call("onMessage", server, params);
}

void Plugin::onMe(Server::Ptr server,
		  const std::string &channel,
		  const std::string &who,
		  const std::string &message)

{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(message);

	call("onMe", server, params);
}

void Plugin::onMode(Server::Ptr server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &mode,
		    const std::string &modeArg)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(mode);
	params.push_back(modeArg);

	call("onMode", server, params);
}

void Plugin::onNick(Server::Ptr server,
		    const std::string &oldnick,
		    const std::string &newnick)
{
	std::vector<std::string> params;

	params.push_back(oldnick);
	params.push_back(newnick);

	call("onNick", server, params);
}

void Plugin::onNotice(Server::Ptr server,
		      const std::string &nick,
		      const std::string &target,
		      const std::string &notice)
{
	std::vector<std::string> params;

	params.push_back(nick);
	params.push_back(target);
	params.push_back(notice);

	call("onNotice", server, params);
}

void Plugin::onPart(Server::Ptr server,
		    const std::string &channel,
		    const std::string &who,
		    const std::string &reason)
{
	std::vector<std::string> params;
	
	params.push_back(channel);
	params.push_back(who);
	params.push_back(reason);

	call("onPart", server, params);
}

void Plugin::onQuery(Server::Ptr server,
		     const std::string &who,
		     const std::string &message)
{
	std::vector<std::string> params;

	params.push_back(who);
	params.push_back(message);

	call("onQuery", server, params);
}

void Plugin::onReload()
{
	call("onReload");
}

void Plugin::onTopic(Server::Ptr server,
		     const std::string &channel,
		     const std::string &who,
		     const std::string &topic)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);
	params.push_back(topic);

	call("onTopic", server, params);
}

void Plugin::onUserMode(Server::Ptr server,
			const std::string &who,
			const std::string &mode)
{
	std::vector<std::string> params;

	params.push_back(who);
	params.push_back(mode);

	call("onUserMode", server, params);
}

void Plugin::onUnload()
{
	call("onUnload");
}

} // !irccd
