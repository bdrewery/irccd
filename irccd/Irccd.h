/*
 * Irccd.h -- main irccd class
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

#ifndef _IRCCD_H_
#define _IRCCD_H_

#include <exception>
#include <map>
#include <mutex>
#include <sstream>

#include <Parser.h>
#include <SocketListener.h>
#include <SocketTCP.h>

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

typedef std::vector<std::shared_ptr<Server>> ServerList;

#if defined(WITH_LUA)
  typedef std::vector<std::shared_ptr<Plugin>> PluginList;
  typedef std::map<std::shared_ptr<Server>, std::vector<DefCall>> DefCallList;
#endif

namespace options {

enum {
	Config		= 'c',
	Foreground	= 'f',
	Verbose		= 'v',
	PluginPath	= 'p',
	PluginWanted	= 'P'
};

} // !options

class Irccd {
private:
	static Irccd *m_instance;			//! unique instance

	// Ignition
	bool m_running;					//! forever loop
	bool m_foreground;				//! run to foreground

	// Config
	std::string m_configPath;			//! config file path
	std::unordered_map<char, bool> m_overriden;	//! overriden parameters

	// Plugins
	std::vector<std::string> m_pluginDirs;		//! list of plugin directories
	std::vector<std::string> m_pluginWanted;	//! list of wanted modules

#if defined(WITH_LUA)
	std::mutex m_pluginLock;			//! lock to add plugin

	PluginList m_plugins;				//! list of plugins loaded
	DefCallList m_deferred;				//! list of deferred call
#endif

	ServerList m_servers;				//! list of servers

	// Socket clients and listeners
	std::vector<SocketTCP> m_socketServers;		//! socket servers
	std::map<SocketTCP, std::string> m_clients;	//! socket clients
	SocketListener m_listener;			//! socket listener

	// Identities
	std::vector<Server::Identity> m_identities;	//! user identities
	Server::Identity m_defaultIdentity;		//! default identity

	void clientAdd(SocketTCP &client);
	void clientRead(SocketTCP &client);
	void execute(SocketTCP &client, const std::string &cmd);
	bool isPluginLoaded(const std::string &name);
	bool isOverriden(char c);

	/**
	 * Open will call the below function in the same order they are
	 * declared, do not change that.
	 */
	void openConfig();

	// [general]
	void openPlugins();

	// [identity]
	void openIdentities(const Parser &config);

	// [listener]
	void openListeners(const Parser &config);
	void extractInternet(const Section &s);

#if !defined(_WIN32)
	void extractUnix(const Section &s);
#endif

	// [server]
	void openServers(const Parser &config);
	void extractChannels(const Section &section, std::shared_ptr<Server> server);

	// Avoid constructor.
	Irccd();

public:
	~Irccd();

	/**
	 * Get the unique irccd instance.
	 *
	 * @return the irccd instance
	 */
	static Irccd * getInstance();

	/**
	 * Tells irccd that a parameter from command line has been set.
	 *
	 * @param c the option
	 */
	void override(char c);

	/**
	 * Add a plugin path to find other plugins.
	 *
	 * @param path the directory path
	 */
	void addPluginPath(const std::string &path);

	/**
	 * Add a wanted plugin, irccd will concatenate ".lua"
	 * to the plugin name and try to find it in the
	 * plugin directories.
	 *
	 * @param name the plugin name
	 */
	void addWantedPlugin(const std::string &name);

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
	std::shared_ptr<Plugin> findPlugin(lua_State *state);

	/**
	 * Find a plugin by it's name. It is used by irccdctl
	 * to load, unload and reload on command.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw out_of_range when not found
	 */
	std::shared_ptr<Plugin> findPlugin(const std::string &name);

	/**
	 * Get plugins.
	 *
	 * @return a list of plugins
	 */
	PluginList & getPlugins();

	/**
	 * Get the plugin lock, used to load / unload module.
	 *
	 * @return the mutex lock
	 */
	std::mutex & getPluginLock();

	/**
	 * Add a deferred call for a specific server.
	 *
	 * @param sever the server that request it
	 * @param call the plugin to call
	 */
	void addDeferred(std::shared_ptr<Server> server, DefCall call);
#endif

	/**
	 * Get the servers list
	 *
	 * @return the list of servers
	 */
	ServerList & getServers();

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
	 * Find a server by its resource name.
	 *
	 * @param name the server's resource name
	 * @return a server
	 */
	std::shared_ptr<Server> & findServer(const std::string &name);

	/**
	 * Find an identity.
	 *
	 * @param name the identity's resource name.
	 * @return an identity or the default one
	 */
	const Server::Identity & findIdentity(const std::string &name);

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

	/* ------------------------------------------------
	 * IRC Handlers
	 * ------------------------------------------------ */

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

	/**
	 * The handleConnection will auto join servers
	 * defined in the server of that event.
	 *
	 * @param event the event
	 */
	void handleConnection(const IrcEvent &event);

	/**
	 * The handleInvite will auto join the server if the
	 * user want it.
	 *
	 * @param event the event
	 */
	void handleInvite(const IrcEvent &event);

	/**
	 * The kick function just remove the channel if I'm the one who was
	 * kicked.
	 *
	 * @param event the event
	 */
	void handleKick(const IrcEvent &event);

#if defined(WITH_LUA)
	/**
	 * Call the plugin function depending on the event.
	 *
	 * @param p the current plugin
	 * @param ev the event
	 */
	void callPlugin(std::shared_ptr<Plugin> p, const IrcEvent &ev);

	/**
	 * Call a deferred function.
	 *
	 * @param ev the event
	 */
	void callDeferred(const IrcEvent &ev);
#endif
};

#if defined(WITH_LUA)

template<class T>
static T toType(lua_State *L, int idx)
{
	return reinterpret_cast<T>(lua_touserdata(L, idx));
}

template<class T>
static T toType(lua_State *L, int idx, const char *tname)
{
	return reinterpret_cast<T>(luaL_checkudata(L, idx, tname));
}

#endif

} // !irccd

#endif // !_IRCCD_H_
