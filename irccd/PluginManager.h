/*
 * PluginManager.h -- manage plugins
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

/**
 * @file PluginManager.h
 * @brief Manage the list of plugins
 */

#include <memory>
#include <mutex>
#include <vector>

#include <common/Singleton.h>

namespace irccd {

class Plugin;

/**
 * @class PluginManager
 * @brief Manage irccd plugins
 */
class PluginManager : public Singleton<PluginManager> {
private:
	SINGLETON(PluginManager);

	using Dirs		= std::vector<std::string>;
	using List		= std::vector<std::shared_ptr<Plugin>>;
	using Mutex		= std::recursive_mutex;
	using Lock		= std::lock_guard<Mutex>;

	Mutex			m_lock;
	Dirs			m_dirs;
	List			m_plugins;

public:
	/**
	 * Check whether a plugin is loaded.
	 *
	 * @param name the name
	 * @return true if loaded
	 */
	bool isLoaded(const std::string &name);

	/**
	 * Get a list of loaded plugins.
	 *
	 * @return the list of loaded plugins
	 */
	std::vector<std::string> list();

	/**
	 * Add path for finding plugins.
	 *
	 * @param path the path
	 */
	void addPath(std::string path);

	/**
	 * Try to load a plugin.
	 *
	 * @param path the full path
	 * @param relative tell if the path is relative
	 * @throw std::runtime_error on failure
	 */
	void load(const std::string &path, bool relative = false);

	/**
	 * Unload a plugin.
	 *
	 * @param name the plugin name
	 */
	void unload(const std::string &name);

	/**
	 * Reload a plugin.
	 *
	 * @param name the plugin to reload
	 */
	void reload(const std::string &name);

	/**
	 * Find a plugin by its name.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw std::out_of_range if not found
	 */
	std::shared_ptr<Plugin> find(const std::string &name);

	/**
	 * Iterate over all plugins.
	 *
	 * @param func the function to call
	 */
	template <typename Func>
	void forAll(Func func)
	{
		Lock lk(m_lock);

		/*
		 * We use an index based loop because if the handle
		 * load a plugin, the for range will getting invalid.
		 */
		for (size_t i = 0; i < m_plugins.size(); ++i)
			func(m_plugins[i]);
	}
};

} // !irccd

#endif // _PLUGIN_MANAGER_H_
