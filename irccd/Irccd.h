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

#include <condition_variable>
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
#  include "Plugin.h"
#endif

namespace irccd {

/* --------------------------------------------------------
 * IRC Events, used by Server
 * -------------------------------------------------------- */

enum class IrcEventType {
	Connection
};

typedef std::vector<std::string> IrcEventParams;

struct IrcEvent {
	IrcEventType m_type;				//! event type
	IrcEventParams m_params;			//! parameters

	std::shared_ptr<Server> m_server;		//! on which server

	IrcEvent(IrcEventType type, IrcEventParams params,
		 std::shared_ptr<Server> server);

	~IrcEvent();
};

/* --------------------------------------------------------
 * User Events, used by Listener
 * -------------------------------------------------------- */

/* --------------------------------------------------------
 * Irccd main class
 * -------------------------------------------------------- */

typedef std::vector<std::shared_ptr<Server>> ServerList;

class Irccd {
private:
	static Irccd *m_instance;			//! unique instance

	// Config
	std::string m_configPath;			//! config file path

	// Plugins
	std::vector<std::string> m_pluginDirs;		//! list of plugin directories
	std::vector<std::string> m_pluginWanted;	//! list of wanted modules

#if defined(WITH_LUA)
	std::vector<Plugin> m_plugins;			//! list of plugins loaded
	std::mutex m_pluginLock;			//! lock to add plugin
#endif

	// Irc & User event queues
	std::vector<IrcEvent> m_ircEvents;		//! IRC events

	std::mutex m_queueLock;				//! event queue lock
	std::condition_variable m_queueCond;		//! queue waiting condition

	ServerList m_servers;				//! list of servers

	// Socket clients and listeners
	std::vector<SocketTCP> m_socketServers;		//! socket servers
	std::map<SocketTCP, std::string> m_clients;	//! socket clients
	SocketListener m_listener;			//! socket listener

	// Identities
	std::vector<Identity> m_identities;		//! user identities
	Identity m_defaultIdentity;			//! default identity

	void clientAdd(SocketTCP &client);
	void clientRead(SocketTCP &client);
	void execute(SocketTCP &client, const std::string &cmd);
	bool isPluginLoaded(const std::string &name);

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
	void extractChannels(const Section &section, Server &server);

	Irccd();
public:
	~Irccd();

	/**
	 * Get the unique irccd instance.
	 *
	 * @return the irccd instance
	 */
	static Irccd * getInstance(void);

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
	Plugin & findPlugin(lua_State *state);

	/**
	 * Find a plugin by it's name. It is used by irccdctl
	 * to load, unload and reload on command.
	 *
	 * @param name the plugin name
	 * @return the plugin
	 * @throw out_of_range when not found
	 */
	Plugin & findPlugin(const std::string &name);

	/**
	 * Get plugins.
	 *
	 * @return a list of plugins
	 */
	std::vector<Plugin> & getPlugins();

	/**
	 * Get the plugin lock, used to load / unload module.
	 *
	 * @return the mutex lock
	 */
	std::mutex & getPluginLock();
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
	 * Set the verbosity.
	 *
	 * @param verbose true means more verbose
	 */
	void setVerbosity(bool verbose);

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
	const Identity & findIdentity(const std::string &name);

	/**
	 * Run the application.
	 *
	 * @return the error code
	 */
	int run();

	/* --- EXPERIMENTAL --- */
	void pushIrcEvent(IrcEvent &event);
};

} // !irccd

#endif // !_IRCCD_H_
