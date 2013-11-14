/*
 * Irccd.h -- main irccd class
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

#ifndef _IRCCD_H_
#define _IRCCD_H_

#include <exception>
#include <mutex>
#include <sstream>

#include <Parser.h>
#include <config.h>

#include "Server.h"

#if defined(WITH_LUA)
#  include "DefCall.h"
#  include "Plugin.h"
#endif

namespace irccd {

/* --------------------------------------------------------
 * Irccd main class
 * -------------------------------------------------------- */

enum Options {
	Config		= 'c',
	Foreground	= 'f',
	Verbose		= 'v',
	PluginPath	= 'p',
	PluginWanted	= 'P'
};

#if defined(WITH_LUA)

using PluginMap		= std::unordered_map<
				lua_State *,
				Plugin::Ptr
			  >;

using PluginList	= std::vector<
				Plugin::Ptr
			  >;

using IdentityList	= std::vector<
				Server::Identity
			  >;

using ThreadMap		= std::unordered_map<
				lua_State *,
				Plugin::Ptr
			  >;

using DefCallList	= std::unordered_map<
				Server::Ptr,
				std::vector<DefCall>
			  >;

using Lock		= std::lock_guard<std::mutex>;
using RLock		= std::lock_guard<std::recursive_mutex>;

using ServerList	= std::vector<Server::Ptr>;

#endif

class Irccd {
private:
	using PluginDirs		= std::vector<std::string>;
	using PluginList		= std::vector<std::string>;
	using PluginSpecifiedMap	= std::unordered_map<std::string, bool>;

	static Irccd m_instance;			//! unique instance

	// Ignition
	bool m_running;					//! forever loop
	bool m_foreground;				//! run to foreground

	// Config
	std::string m_configPath;			//! config file path
	std::unordered_map<char, bool> m_overriden;	//! overriden parameters

	// Identities
	IdentityList m_identities;			//! user identities
	Server::Identity m_defaultIdentity;		//! default identity

	// Plugins
	PluginDirs m_pluginDirs;			//! list of plugin directories
	PluginList m_pluginWanted;			//! list of wanted modules
	PluginSpecifiedMap m_pluginSpecified;		//! list of plugin specified by paths

#if defined(WITH_LUA)
	std::recursive_mutex m_pluginLock;		//! lock to add plugin
	PluginMap m_pluginMap;				//! map of plugins loaded
	ThreadMap m_threadMap;				//! map of threads
	DefCallList m_deferred;				//! list of deferred call
#endif

	ServerList m_servers;				//! list of servers
	std::mutex m_serverLock;			//! lock for managing servers


	/* {{{ Private miscellaneous methods */
	
	Irccd();

	bool isOverriden(char c);

	/* }}} */


	/* {{{ Private plugin management */

	void loadWantedPlugins();
	bool isPluginLoaded(const std::string &name);

	/* }}} */

	/* {{{ Private open functions (configuration) */

	/*
	 * Open will call the below function in the same order they are
	 * declared, do not change that.
	 */
	void openConfig();
	void readGeneral(const Parser &config);		// [general]
	void readPlugins(const Parser &config);		// [plugins]
	void readIdentities(const Parser &config);	// [identity]

	void readListeners(const Parser &config);
	void extractInternet(const Section &s, int type);

#if !defined(_WIN32)
	void extractUnix(const Section &s, int type);
#endif

	void readServers(const Parser &config);
	void extractChannels(const Section &section, Server::Ptr server);

	/* }}} */

	/* {{{ Private server management */

	void handleConnection(const IrcEvent &event);
	void handleInvite(const IrcEvent &event);
	void handleKick(const IrcEvent &event);

#if defined(WITH_LUA)
	void callPlugin(std::shared_ptr<Plugin> p, const IrcEvent &ev);
	void callDeferred(const IrcEvent &ev);
#endif

	void removeServer(Server::Ptr sv);

	/* }}} */

public:
	/* {{{ Public constructor, destructor and miscellaneous methods */

	~Irccd();

	/**
	 * Get the unique irccd instance.
	 *
	 * @return the irccd instance
	 */
	static Irccd &getInstance();

	/**
	 * Tells irccd that a parameter from command line has been set.
	 *
	 * @param c the option
	 */
	void override(char c);

	/**
	 * Set the config path to open.
	 *
	 * @param path the config file path
	 */
	void setConfigPath(const std::string &path);

	/**
	 * Tells if we should run to foreground or not.
	 *
	 * @param mode the mode
	 */
	void setForeground(bool mode);

	/**
	 * Find an identity.
	 *
	 * @param name the identity's resource name.
	 * @return an identity or the default one
	 */
	const Server::Identity &findIdentity(const std::string &name);

	/* }}} */

	/* {{{ Public Plugin management */

	/**
	 * Add a plugin path to find other plugins.
	 *
	 * @param path the directory path
	 */
	void addPluginPath(const std::string &path);

	/**
	 * Request for loading the plugin. If specified is set to true
	 * then the name is the full path to the plugin otherwise, it is
	 * searched.
	 *
	 * @param name the plugin name or path
	 * @param bool is specified by path?
	 */
	void addWantedPlugin(const std::string &name, bool specified = false);

	/**
	 * Load a plugin externally, used for irccdctl.
	 *
	 * @param name the plugin name to load
	 */
	void loadPlugin(const std::string &name);

	/**
	 * Unload a plugin.
	 *
	 * @param name the plugin name to unload.
	 */
	void unloadPlugin(const std::string &name);

	/**
	 * Reload a plugin, it does not close but calls a specific
	 * Lua function defined in the file.
	 *
	 * @param name the plugin name
	 */
	void reloadPlugin(const std::string &name);

#if defined(WITH_LUA)
	/**
	 * Find a plugin by it's associated Lua State, so it can
	 * be retrieved by every Lua bindings.
	 *
	 * @param state the Lua state
	 * @return the plugin
	 * @throw out_of_range when not found
	 */
	Plugin::Ptr findPlugin(lua_State *state);

	/**
	 * Find a plugin by it's name. It is used by irccdctl
	 * to load, unload and reload on command.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw out_of_range when not found
	 */
	Plugin::Ptr findPlugin(const std::string &name);

	/**
	 * Add a deferred call for a specific server.
	 *
	 * @param sever the server that request it
	 * @param call the plugin to call
	 */
	void addDeferred(Server::Ptr server, DefCall call);

	/**
	 * Register the thread to one plugin so that irccd.plugin
	 * think that L state is of plugin.
	 *
	 * @param L the new Lua state
	 * @param plugin for which plugin
	 */
	void registerThread(lua_State *L, Plugin::Ptr plugin);

	/**
	 * Remove the attach thread from that Lua state
	 *
	 * @param L the Lua state from which thread
	 */
	void unregisterThread(lua_State *L);
#endif

	/* }}} */

	/* {{{ Public Server management */

	void connectServer(const Server::Info &info,
			   const Server::Identity &identity,
			   const Server::Options &options);

	/**
	 * Get the servers list
	 *
	 * @return the list of servers
	 */
	ServerList &getServers();

	/**
	 * Find a server by its resource name.
	 *
	 * @param name the server's resource name
	 * @return a server
	 * @throw std::out_of_range if not found
	 */
	Server::Ptr findServer(const std::string &name);

	/**
	 * Global IRC event handler, process the event type and call
	 * every Lua plugin if Lua is compiled in.
	 *
	 * For some event, also call handleConnection
	 * and handleInvite to do specific things.
	 *
	 * @param event the event
	 */
	void handleIrcEvent(const IrcEvent &event);

	/* }}} */

	/* {{{ Irccd management */

	/**
	 * Run the application.
	 *
	 * @return the error code
	 */
	int run();

	/**
	 * Stop all threads and everything else.
	 */
	void stop();

	/* }}} */
};

} // !irccd

#endif // !_IRCCD_H_
