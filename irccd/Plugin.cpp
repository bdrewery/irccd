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

#include "event/IrcEventLoad.h"
#include "event/IrcEventReload.h"
#include "event/IrcEventUnload.h"

#include "lua/LuaServer.h"

#include "Irccd.h"
#include "IrcEvent.h"
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
			IrcEventUnload().action(*i->m_process);
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
		auto i = find(name);

		IrcEventReload().action(*i->m_process);
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

	/*
	 * We use an index based loop because if the handle
	 * load a plugin, the for range will getting invalid.
	 */
	for (size_t i = 0; i < plugins.size(); ++i) {
		try {
			ev.action(plugins[i]->getState());
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
	IrcEventLoad().action(L);
}

} // !irccd
