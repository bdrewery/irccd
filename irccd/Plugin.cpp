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

#include "Lua/LuaServer.h"

#include "Irccd.h"
#include "Plugin.h"

namespace irccd {

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

std::string Plugin::ErrorException::error() const
{
	return m_error;
}

const char * Plugin::ErrorException::what() const throw()
{
	return m_error.c_str();
}

/* --------------------------------------------------------
 * private methods and members
 * -------------------------------------------------------- */

std::string Plugin::getGlobal(const std::string &name)
{
	std::string result;

	lua_getglobal(*m_process, name.c_str());
	if (lua_type(*m_process, -1) == LUA_TSTRING)
		result = lua_tostring(*m_process, -1);
	lua_pop(*m_process, 1);

	return result;
}

void Plugin::callFunction(const std::string &func,
			  Server::Ptr server,
			  std::vector<std::string> params)
{
	lua_State *L = *m_process;

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
			auto error = lua_tostring(L, -1);
			lua_pop(L, 1);

			throw Plugin::ErrorException(m_info.name, error);
		}
	}
}

void Plugin::callFunctionNum(const std::string &func, Server::Ptr server, int np)
{
	lua_State *L = *m_process;

	lua_getglobal(L, func.c_str());
	if (lua_type(L, -1) != LUA_TFUNCTION)
		lua_pop(L, 1);
	else {
		// Move the function
		lua_insert(L, 1);

		// Push a server an optional arguments
		if (server) {
			Luae::pushShared<Server>(L, server, ServerType);
			lua_insert(L, lua_gettop(L) - np++);
		}

		if (lua_pcall(L, np, 0, 0) != LUA_OK) {
			std::string error = lua_tostring(L, -1);
			lua_pop(L, 1);

			throw Plugin::ErrorException(m_info.name, error);
		}
	}
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
	case IrcEventType::Names:
		p->onNames(ev.m_server, ev.m_params);
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
	case IrcEventType::Whois:
	{
		Server::WhoisInfo info;

		info.nick = ev.m_params[0];
		info.user = ev.m_params[1];
		info.host = ev.m_params[2];
		info.realname = ev.m_params[3];

		for (size_t i = 4; i < ev.m_params.size(); ++i)
			info.channels.push_back(ev.m_params[i]);

		p->onWhois(ev.m_server, info);
	}
		break;
	default:
		break;
	}
}

/* --------------------------------------------------------
 * public static methods and members
 * -------------------------------------------------------- */

Plugin::Mutex		Plugin::pluginLock;
Plugin::Dirs		Plugin::pluginDirs;
Plugin::List		Plugin::plugins;

bool Plugin::isLoaded(const std::string &name)
{
	Lock lk(pluginLock);
	bool ret = true;

	try {
		(void)find(name);
	} catch (std::out_of_range ex) {
		ret = false;
	}

	return ret;
}

std::vector<std::string> Plugin::list()
{
	Lock lk(pluginLock);
	std::vector<std::string> list;

	for (auto p : plugins)
		list.push_back(p->m_info.name);

	return list;
}

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

	if (isLoaded(name))
		throw std::runtime_error("plugin " + name + " already loaded");

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
		throw std::runtime_error("plugin " + realname + " not found");

	/*
	 * At this step, the open function will open the lua
	 * script, that script may want to call some bindings
	 * directly so we need to add it to the registered
	 * Lua plugins even if it has failed. So we remove
	 * it only on failure and expect plugins to be well
	 * coded.
	 */

	Plugin::Ptr p = std::make_shared<Plugin>(realname, realpath);
	plugins.push_back(p);

	try {
		p->open();
	} catch (ErrorException ex) {
		plugins.erase(
			std::remove(plugins.begin(), plugins.end(), p),
			plugins.end()
		);

		throw std::runtime_error("failed to load " + ex.which() + ": " + ex.error());
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

		plugins.erase(
			std::remove(plugins.begin(), plugins.end(), i),
			plugins.end()
		);
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

Plugin::Ptr Plugin::find(const std::string &name)
{
	Lock lk(pluginLock);
	std::ostringstream oss;

	auto i = std::find_if(plugins.begin(), plugins.end(),
	    [&] (Plugin::Ptr p) -> bool {
		return p->getName() == name;
	    }
	);

	if (i == plugins.end()) {
		oss << "plugin " << name << " not found";

		throw std::out_of_range(oss.str());
	}

	return *i;
}

void Plugin::handleIrcEvent(const IrcEvent &ev)
{
	Lock lk(pluginLock);

	for (size_t i = 0; i < plugins.size(); ++i) {
		try {
			callPlugin(plugins[i], ev);
		} catch (Plugin::ErrorException ex) {
			Logger::warn("plugin: %s", ex.what());
		}
	}
}

/* --------------------------------------------------------
 * public methods
 * -------------------------------------------------------- */

Plugin::Plugin(const std::string &name,
	       const std::string &path)
{
	m_info.name = name;
	m_info.path = path;

	m_process = Process::create();
}

const std::string &Plugin::getName() const
{
	return m_info.name;
}

const std::string &Plugin::getHome() const
{
	return m_info.home;
}

lua_State *Plugin::getState()
{
	return static_cast<lua_State *>(*m_process);
}

void Plugin::open()
{
	lua_State *L = static_cast<lua_State *>(*m_process);

	// Load default library as it was done by require.
	for (auto l : Process::luaLibs)
		Luae::require(L, l.first, l.second, true);

	// Put external modules in package.preload so user
	// will need require (modname)
	for (auto l : Process::irccdLibs)
		Luae::preload(L, l.first, l.second);

	if (luaL_dofile(L, m_info.path.c_str()) != LUA_OK) {
		auto error = lua_tostring(L, -1);
		lua_pop(L, 1);

		throw ErrorException(m_info.name, error);
	}

	// Find the home directory for the plugin
	m_info.home = Util::findPluginHome(m_info.name);

	// Extract global information
	m_info.author	= getGlobal("AUTHOR");
	m_info.comment	= getGlobal("COMMENT");
	m_info.version	= getGlobal("VERSION");
	m_info.license	= getGlobal("LICENSE");

	// Initialize the plugin name and its data
	Process::initialize(m_process, m_info);

	// Do a initial load
	onLoad();
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

void Plugin::onNames(Server::Ptr server,
		     const std::vector<std::string> &names)
{
	// 1. Push the channel name
	lua_pushlstring(*m_process, names[0].c_str(), names[0].length());

	// 2. Push the users
	lua_createtable(*m_process, 0, 0);

	for (size_t i = 1; i < names.size(); ++i) {
		lua_pushlstring(*m_process, names[i].c_str(), names[i].length());
		lua_rawseti(*m_process, -2, i);
	}

	callFunctionNum("onNames", server, 2);
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

void Plugin::onUnload()
{
	callFunction("onUnload");
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

void Plugin::onWhois(Server::Ptr server,
		     const Server::WhoisInfo &info)
{
	lua_createtable(*m_process, 0, 0);

	lua_pushlstring(*m_process, info.nick.c_str(), info.nick.length());
	lua_setfield(*m_process, -2, "nickname");

	lua_pushlstring(*m_process, info.user.c_str(), info.user.length());
	lua_setfield(*m_process, -2, "user");

	lua_pushlstring(*m_process, info.host.c_str(), info.host.length());
	lua_setfield(*m_process, -2, "host");

	lua_pushlstring(*m_process, info.realname.c_str(), info.realname.length());
	lua_setfield(*m_process, -2, "realname");

	if (info.channels.size() >= 4) {
		lua_createtable(*m_process, 0, 0);

		for (size_t i = 4; i < info.channels.size(); ++i) {
			lua_pushstring(*m_process, info.channels[i].c_str());
			lua_rawseti(*m_process, -2, i - 3);
		}

		lua_setfield(*m_process, -2, "channels");
	}

	callFunctionNum("onWhois", server, 1);
}

} // !irccd
