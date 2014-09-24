/*
 * PluginManager.cpp -- manage plugins
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include <common/Logger.h>
#include <common/Util.h>

#include "Plugin.h"
#include "PluginManager.h"

namespace irccd {

bool PluginManager::isLoaded(const std::string &name)
{
	Lock lk(m_lock);
	bool ret = true;

	try {
		(void)find(name);
	} catch (std::out_of_range ex) {
		ret = false;
	}

	return ret;
}

std::vector<std::string> PluginManager::list()
{
	Lock lk(m_lock);
	std::vector<std::string> list;

	for (auto &p : m_plugins)
		list.push_back(p->getName());

	return list;
}

void PluginManager::addPath(std::string path)
{
	Lock lk(m_lock);

	m_dirs.push_back(std::move(path));
}

void PluginManager::load(const std::string &name, bool relative)
{
	Lock lk(m_lock);

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
		for (const auto &p : m_dirs) {
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

	auto p = std::make_shared<Plugin>(realname, realpath);
	m_plugins.push_back(p);

	try {
		p->open();
	} catch (const Plugin::ErrorException &ex) {
		m_plugins.erase(std::remove(m_plugins.begin(), m_plugins.end(), p), m_plugins.end());

		throw std::runtime_error("failed to load " + ex.which() + ": " + ex.error());
	}
}

void PluginManager::unload(const std::string &name)
{
	Lock lk(m_lock);

	try {
		auto i = find(name);

		try {
			i->onUnload();
		} catch (const Plugin::ErrorException &ex) {
			Logger::warn("irccd: error while unloading %s: %s",
			    name.c_str(), ex.what());
		}

		m_plugins.erase(std::remove(m_plugins.begin(), m_plugins.end(), i), m_plugins.end());
	} catch (const std::exception &) {
		Logger::warn("irccd: there is no plugin %s loaded", name.c_str());
	}
}

void PluginManager::reload(const std::string &name)
{
	Lock lk(m_lock);

	try {
		auto i = find(name);

		try {
			i->onReload();
		} catch (const Plugin::ErrorException &ex) {
			Logger::warn("plugin %s: %s", ex.which().c_str(), ex.error().c_str());
		}
	} catch (const std::out_of_range &ex) {
		Logger::warn("irccd: %s", ex.what());
	}
}

std::shared_ptr<Plugin> PluginManager::find(const std::string &name)
{
	Lock lk(m_lock);
	std::ostringstream oss;

	auto i = std::find_if(m_plugins.begin(), m_plugins.end(),
	    [&] (auto &p) -> bool {
		return p->getName() == name;
	    }
	);

	if (i == m_plugins.end()) {
		oss << "plugin " << name << " not found";

		throw std::out_of_range(oss.str());
	}

	return *i;
}

#if 0

void PluginManager::collectGarbage()
{
	Lock lk(pluginLock);

	for (auto p : plugins)
		lua_gc(p->getState(), LUA_GCCOLLECT, 0);
}

#endif

} // !irccd
