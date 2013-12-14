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

#include "DefCall.h"
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

namespace irccd {

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

Plugin::ErrorException::ErrorException(const std::string &which,
				       const std::string &error)
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

bool Plugin::isPluginLoaded(const std::string &name)
{
	bool ret = true;

	try {
		(void)find(name);
	} catch (std::out_of_range ex) {
		ret = false;
	}

	return ret;
}

void Plugin::callPlugin(Plugin::Ptr p, const IrcEvent &ev)
{
	switch (ev.m_type) {
	case IrcEventType::Connection:
		p->onConnect(ev.m_server);
		break;
	case IrcEventType::ChannelNotice:
		p->onChannelNotice(ev.m_server, ev.m_params[0], ev.m_params[1],
		    ev.m_params[2]);
		break;
	case IrcEventType::Invite:
		p->onInvite(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Join:
		p->onJoin(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Kick:
		p->onKick(ev.m_server, ev.m_params[0], ev.m_params[1],
		    ev.m_params[2], ev.m_params[3]);
		break;
	case IrcEventType::Message:
	{
		std::string cc = ev.m_server->getOptions().m_commandChar;
		std::string sp = cc + p->getName();
		std::string msg = ev.m_params[2];

		// handle special commands "!<plugin> command"
		if (cc.length() > 0 && msg.compare(0, sp.length(), sp) == 0) {
			std::string plugin = msg.substr(
			    cc.length(), sp.length() - cc.length());

			if (plugin == p->getName()) {
				p->onCommand(ev.m_server,
						ev.m_params[0],
						ev.m_params[1],
						msg.substr(sp.length())
				);
			}
		} else
			p->onMessage(ev.m_server, ev.m_params[0], ev.m_params[1],
			    ev.m_params[2]);
	}
		break;
	case IrcEventType::Me:
		p->onMe(ev.m_server, ev.m_params[1], ev.m_params[0], ev.m_params[2]);
		break;
	case IrcEventType::Mode:
		p->onMode(ev.m_server, ev.m_params[0], ev.m_params[1],
		    ev.m_params[2], ev.m_params[3]);
		break;
	case IrcEventType::Nick:
		p->onNick(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Notice:
		p->onNotice(ev.m_server, ev.m_params[0], ev.m_params[1],
		    ev.m_params[2]);
		break;
	case IrcEventType::Part:
		p->onPart(ev.m_server, ev.m_params[0], ev.m_params[1],
		    ev.m_params[2]);
		break;
	case IrcEventType::Query:
		p->onQuery(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Topic:
		p->onTopic(ev.m_server, ev.m_params[0], ev.m_params[1],
		     ev.m_params[2]);
		break;
	case IrcEventType::UserMode:
		p->onUserMode(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	default:
		break;
	}
}

void Plugin::callDeferred(const IrcEvent &ev)
{
	// Check if we have deferred call for this server
	if (deferred.find(ev.m_server) == deferred.end())
		return;

	std::vector<DefCall>::iterator it = deferred[ev.m_server].begin();

	for (; it != deferred[ev.m_server].end(); ) {
		bool deleteIt = true;

		if (ev.m_type == it->type()) {
			switch (it->type()) {
			case IrcEventType::Names:
				it->onNames(ev.m_params);
				break;
			case IrcEventType::Whois:
				it->onWhois(ev.m_params);
				break;
			default:
				deleteIt = false;
				break;
			}
		} else
			deleteIt = false;

		/*
		 * If found and executed, break the loop if not, we will
		 * call multiple times for the same event.
		 */
		if (deleteIt) {
			it = deferred[ev.m_server].erase(it);
			break;
		} else
			++it;
	}
}

void Plugin::callFunction(const std::string &func,
			  Server::Ptr server,
			  std::vector<std::string> params)
{
	lua_State *L = m_state;

	lua_getglobal(L, func.c_str());
	if (lua_type(L, -1) != LUA_TFUNCTION)
		lua_pop(L, 1);
	else {
		int np = 0;

		if (server) {
			Luae::pushShared<Server>(L, server, ServerType);
			++ np;
		}

		for (const std::string &a : params) {
			lua_pushstring(L, a.c_str());
			++ np;
		}

		if (lua_pcall(L, np, 0, 0) != LUA_OK) {
			std::string error = lua_tostring(L, -1);
			lua_pop(L, 1);

			throw ErrorException(m_name, error);
		}
	}
}

/* --------------------------------------------------------
 * public methods and members
 * -------------------------------------------------------- */

Plugin::Mutex		Plugin::pluginLock;
Plugin::Dirs		Plugin::pluginDirs;
Plugin::Map		Plugin::pluginMap;
Plugin::ThreadMap	Plugin::threadMap;
Plugin::DefCallList	Plugin::deferred;

void Plugin::addPath(const std::string &path)
{
	Lock lk(pluginLock);

	pluginDirs.push_back(path);
}

void Plugin::load(const std::string &name, bool relative)
{
	Lock lk(pluginLock);

	std::ostringstream oss;
	std::string realname, realpath;
	bool found = false;

	if (isPluginLoaded(name))
		return;

	/*
	 * If the plugin has been specified by path using foo = /path then
	 * it should contains the .lua extension, otherwise we search
	 * for it.
	 */
	if (relative) {
		Logger::log("irccd: checking for plugin %s", name.c_str());
		found = Util::exist(name);

		/*
		 * Compute the name by removing .lua extension and optional
		 * directory.
		 */
		realpath = name;
		realname = Util::baseName(realpath);

		auto pos = realname.find(".lua");
		if (pos != std::string::npos)
			realname.erase(pos);
	} else {
		realname = name;

		// Seek the plugin in the directories.
		for (auto p : pluginDirs) {
			oss.str("");
			oss << p;

			// Add a / or \\ only if needed
			if (p.length() > 0 && p[p.length() - 1] != Util::DIR_SEP)
				oss << Util::DIR_SEP;

			oss << name << ".lua";

			realpath = oss.str();
			Logger::log("irccd: checking for plugin %s", realpath.c_str());

			if (Util::exist(realpath)) {
				found = true;
				break;
			}
		}
	}

	if (!found)
		Logger::warn("irccd: plugin %s not found", realname.c_str());
	else {
		/*
		 * At this step, the open function will open the lua
		 * script, that script may want to call some bindings
		 * directly so we need to add it to the registered
		 * Lua plugins even if it has failed. So we remove
		 * it only on failure and expect plugins to be well
		 * coded.
		 */
		 
		Plugin::Ptr p = std::make_shared<Plugin>(realname, realpath);
		pluginMap[p->getState()] = p;

		if (!p->open()) {
			Logger::warn("irccd: failed to load plugin %s: %s",
			    realname.c_str(), p->getError().c_str());

			pluginMap.erase(p->getState());
		}
	}
}

void Plugin::unload(const std::string &name)
{
	Lock lk(pluginLock);

	try {
		auto i = find(name);

		try {
			i->onUnload();
		} catch (Plugin::ErrorException ex) {
			Logger::warn("irccd: error while unloading %s: %s",
			    name.c_str(), ex.what());
		}

		pluginMap.erase(i->getState());
	} catch (std::out_of_range) {
		Logger::warn("irccd: there is no plugin %s loaded", name.c_str());
	}
}

void Plugin::reload(const std::string &name)
{
	Lock lk(pluginLock);

	try {
		find(name)->onReload();
	} catch (std::out_of_range ex) {
		Logger::warn("irccd: %s", ex.what());
	}
}

Plugin::Ptr Plugin::find(lua_State *state)
{
	Lock lk(pluginLock);

	for (auto plugin : pluginMap) {
		/*
		 * Test if the current plugin is the one with that
		 * Lua state.
		 */
		if (plugin.first == state)
			return plugin.second;
	}

	for (auto plugin : threadMap) {
		if (plugin.first == state)
			return plugin.second;
	}

	throw std::out_of_range("plugin not found");
}

Plugin::Ptr Plugin::find(const std::string &name)
{
	using Type = std::pair<lua_State *, Plugin::Ptr>;

	Lock lk(pluginLock);
	std::ostringstream oss;

	auto i = std::find_if(pluginMap.begin(), pluginMap.end(),
	    [&] (Type p) -> bool {
		return p.second->getName() == name;
	    }
	);

	if (i == pluginMap.end()) {
		oss << "plugin " << name << " not found";

		throw std::out_of_range(oss.str());
	}

	return (*i).second;
}

void Plugin::forAll(MapFunc func)
{
	Lock lk(pluginLock);

	for (auto p : pluginMap)
		func(p.second);
}

void Plugin::defer(Server::Ptr server, DefCall call)
{
	Lock lk(pluginLock);

	deferred[server].push_back(call);
}

void Plugin::registerThread(lua_State *L,
			    Plugin::Ptr plugin,
			    Thread::Ptr thrd)
{
	Lock lk(pluginLock);

	threadMap[L] = plugin;
	plugin->m_threads.push_back(thrd);
}

void Plugin::unregisterThread(lua_State *L)
{
	Lock lk(pluginLock);

	threadMap.erase(L);
}

void Plugin::handleIrcEvent(const IrcEvent &ev)
{
	Lock lk(pluginLock);

	/*
	 * This is the handle of deferred calls, they are not handled in the
	 * next callPlugin block.
	 */
	try {
		if (ev.m_type == IrcEventType::Names ||
		    ev.m_type == IrcEventType::Whois)
			callDeferred(ev);
	} catch (Plugin::ErrorException ex) {
		Logger::warn("plugin %s: %s", ex.which().c_str(), ex.what());
	}

	for (auto p : pluginMap) {
		/*
		 * Ignore Lua threads that share the same Plugin
		 * object.
		 */
		if (threadMap.find(p.first) != threadMap.end())
			continue;

		try {
			callPlugin(p.second, ev);
		} catch (Plugin::ErrorException ex) {
			Logger::warn("plugin %s: %s",
			    p.second->getName().c_str(), ex.what());
		}
	}
}

Plugin::Plugin(const std::string &name,
	       const std::string &path)
	: m_name(name)
	, m_path(path)
{
	m_state = std::move(LuaState(luaL_newstate()));

	Luae::initRegistry(m_state);
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

const std::string &Plugin::getError() const
{
	return m_error;
}

bool Plugin::open()
{
	// Load default library as it was done by require.
	for (auto l : luaLibs)
		Luae::require(m_state, l.first, l.second, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (auto l : irccdLibs)
		Luae::preload(m_state, l.first, l.second);

	// Find the home directory for the plugin
	m_home = Util::findPluginHome(m_name);

	if (luaL_dofile(m_state, m_path.c_str()) != LUA_OK) {
		m_error = lua_tostring(m_state, -1);
		lua_pop(m_state, 1);

		return false;
	}

	// Do a initial load
	try {
		onLoad();
	} catch (ErrorException ex) {
		m_error = ex.what();
		return false;
	}

	return true;
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

	callFunction("onCommand", server, params);
}

void Plugin::onConnect(Server::Ptr server)
{
	callFunction("onConnect", server);
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

	callFunction("onChannelNotice", server, params);
}

void Plugin::onInvite(Server::Ptr server,
		      const std::string &channel,
		      const std::string &who)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(who);

	callFunction("onInvite", server, params);
}

void Plugin::onJoin(Server::Ptr server,
		    const std::string &channel,
		    const std::string &nickname)
{
	std::vector<std::string> params;

	params.push_back(channel);
	params.push_back(nickname);

	callFunction("onJoin", server, params);
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

	callFunction("onKick", server, params);
}

void Plugin::onLoad()
{
	callFunction("onLoad");
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

	callFunction("onMessage", server, params);
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

	callFunction("onMe", server, params);
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

	callFunction("onMode", server, params);
}

void Plugin::onNick(Server::Ptr server,
		    const std::string &oldnick,
		    const std::string &newnick)
{
	std::vector<std::string> params;

	params.push_back(oldnick);
	params.push_back(newnick);

	callFunction("onNick", server, params);
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

	callFunction("onNotice", server, params);
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

	callFunction("onPart", server, params);
}

void Plugin::onQuery(Server::Ptr server,
		     const std::string &who,
		     const std::string &message)
{
	std::vector<std::string> params;

	params.push_back(who);
	params.push_back(message);

	callFunction("onQuery", server, params);
}

void Plugin::onReload()
{
	callFunction("onReload");
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

	callFunction("onTopic", server, params);
}

void Plugin::onUserMode(Server::Ptr server,
			const std::string &who,
			const std::string &mode)
{
	std::vector<std::string> params;

	params.push_back(who);
	params.push_back(mode);

	callFunction("onUserMode", server, params);
}

void Plugin::onUnload()
{
	callFunction("onUnload");
}

} // !irccd
