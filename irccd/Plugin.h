/*
 * Plugin.h -- irccd Lua plugin interface
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

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>

#include "Luae.h"
#include "Process.h"
#include "Server.h"

namespace irccd {

class IrcEvent;

/**
 * @class Plugin
 * @brief Lua plugin
 *
 * A plugin is identified by name and can be loaded and unloaded
 * at runtime.
 */
class Plugin {
public:
	class ErrorException : public std::exception {
	private:
		std::string m_error;
		std::string m_which;

	public:
		ErrorException() = default;

		ErrorException(const std::string &which, const std::string &error);

		/**
		 * Tells which plugin name has failed.
		 *
		 * @return the plugin name
		 */
		std::string which() const;

		/**
		 * Get the error.
		 *
		 * @return the error
		 */
		std::string error() const;

		/**
		 * @copydoc std::exception::what
		 */
		virtual const char * what() const throw();
	};

	using Ptr		= std::shared_ptr<Plugin>;
	using Dirs		= std::vector<std::string>;
	using List		= std::vector<Plugin::Ptr>;
	using Mutex		= std::recursive_mutex;
	using Lock		= std::lock_guard<Mutex>;
	using MapFunc		= std::function<void (Ptr &ptr)>;

private:
	static Mutex		pluginLock;	//! lock for managing plugins
	static Dirs		pluginDirs;	//! list of plugin directories
	static List		plugins;	//! map of plugins loaded

	Process::Ptr		m_process;	//! lua_State
	Process::Info		m_info;		//! plugin information

	static void callPlugin(Plugin::Ptr p, const IrcEvent &ev);

	std::string getGlobal(const std::string &name);

	void callFunction(const std::string &func,
			  Server::Ptr server = Server::Ptr(),
			  std::vector<std::string> params = std::vector<std::string>());

	void callFunctionNum(const std::string &func,
			  Server::Ptr server,
			  int np = 0);

public:
	/**
	 * Check whether a plugin is loaded.
	 *
	 * @param name the name
	 * @return true if loaded
	 */
	static bool isLoaded(const std::string &name);

	/**
	 * Get a list of loaded plugins.
	 *
	 * @return the list of loaded plugins
	 */
	static std::vector<std::string> list();

	/**
	 * Add path for finding plugins.
	 *
	 * @param path the path
	 */
	static void addPath(const std::string &path);

	/**
	 * Try to load a plugin.
	 *
	 * @param path the full path
	 * @param relative tell if the path is relative
	 * @throw std::runtime_error on failure
	 */
	static void load(const std::string &path, bool relative = false);

	/**
	 * Unload a plugin.
	 *
	 * @param name the plugin name
	 */
	static void unload(const std::string &name);

	/**
	 * Reload a plugin.
	 *
	 * @param name the plugin to reload
	 */
	static void reload(const std::string &name);

	/**
	 * Find a plugin by its name.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw std::out_of_range if not found
	 */
	static Ptr find(const std::string &name);

	/**
	 * Iterate over all plugins.
	 *
	 * @param func the function to call
	 */
	static void forAll(MapFunc func);

	/**
	 * Default constructor. (Forbidden)
	 */
	Plugin() = delete;

	/**
	 * Correct constructor.
	 *
	 * @param name the plugin name
	 * @param path the path to the plugin
	 */
	Plugin(const std::string &name,
	       const std::string &path);

	/**
	 * Get the plugin name.
	 *
	 * @return the name
	 */
	const std::string &getName() const;

	/**
	 * Get the plugin home directory.
	 *
	 * @return the directory
	 */
	const std::string &getHome() const;

	/**
	 * Find the plugin's home. It first tries to load user's one
	 * and system after.
	 */
	void setHome();

	/**
	 * Get the plugin Lua state.
	 *
	 * @return the Lua state
	 */
	lua_State *getState();

	/**
	 * Open the plugin.
	 *
	 * @throw ErrorException on error
	 */
	void open();
};

} // !irccd

#endif // !_PLUGIN_H_
