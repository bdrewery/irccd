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
#include <Socket.h>
#include <SocketAddress.h>
#include <SocketListener.h>

#include <config.h>

#include "Server.h"

#if defined(WITH_LUA)
#  include "DefCall.h"
#  include "Plugin.h"
#endif

namespace irccd
{

/* --------------------------------------------------------
 * Irccd main class
 * -------------------------------------------------------- */

enum Options
{
	Config		= 'c',
	Foreground	= 'f',
	Verbose		= 'v',
	PluginPath	= 'p',
	PluginWanted	= 'P'
};

class Message
{
private:
	std::ostringstream m_data;

public:
	/**
	 * Default constructor.
	 */
	Message();

	/**
	 * Copy constructor.
	 *
	 * @param m the old message
	 */
	Message(const Message &m);

	/**
	 * Tell if the client message has finished. A client message
	 * ends with '\n'.
	 *
	 * @param msg the received data
	 * @param command the final command (if finished)
	 * @return true if finished
	 */
	bool isFinished(const std::string &msg, std::string &command);

	/**
	 * Copy assignment.
	 *
	 * @param m the message
	 * @return the message
	 */
	Message &operator=(const Message &m);
};

using ServerList	= std::vector<std::shared_ptr<Server>>;
using StreamClients	= std::map<Socket, Message>;
using DatagramClients	= std::map<SocketAddress, Message>;

#if defined(WITH_LUA)

using PluginMap		= std::unordered_map<
				lua_State *,
				std::shared_ptr<Plugin>
			  >;

using PluginList	= std::vector<
				std::shared_ptr<Plugin>
			  >;

using ThreadMap		= std::unordered_map<
				lua_State *,
				Plugin::Ptr
			  >;

using DefCallList	= std::unordered_map<
				std::shared_ptr<Server>,
				std::vector<DefCall>
			  >;

using Lock		= std::lock_guard<std::mutex>;

#endif

class Irccd
{
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

	PluginMap m_pluginMap;				//! map of plugins loaded
	ThreadMap m_threadMap;				//! map of threads
	
	DefCallList m_deferred;				//! list of deferred call
#endif

	ServerList m_servers;				//! list of servers

	// Socket clients and listeners
	std::vector<Socket> m_socketServers;		//! socket servers
	SocketListener m_listener;			//! socket listener

	StreamClients m_streamClients;			//! tcp based clients
	DatagramClients m_dgramClients;			//! udp based "clients"

	// Identities
	std::vector<Server::Identity> m_identities;	//! user identities
	Server::Identity m_defaultIdentity;		//! default identity

	/*
	 * These functions are used for TCP clients.
	 */
	void clientAdd(Socket &client);
	void clientRead(Socket &client);

	/*
	 * This function is used for UDP "clients".
	 */
	void peerRead(Socket &s);

	/**
	 * Execute a command from a client, if the client
	 * is a UDP, the info is discarded.
	 *
	 * @param cmd the command
	 * @param s the socket to read
	 * @param info the info (UDP only)
	 */
	void execute(const std::string &cmd,
		     Socket &s,
		     const SocketAddress &info = SocketAddress());

	/**
	 * Send a response to the client.
	 *
	 * @param message the message to send
	 * @param s the socket
	 * @param info the address
	 */
	void notifySocket(const std::string &message,
			  Socket &s,
			  const SocketAddress &info);

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
	void extractInternet(const Section &s, int type);

#if !defined(_WIN32)
	void extractUnix(const Section &s, int type);
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

	/**
	 * Register the thread to one plugin so that irccd.plugin
	 * think that L state is of plugin.
	 *
	 * @param L the new Lua state
	 * @param plugin for which plugin
	 */
	void registerThread(lua_State *L, std::shared_ptr<Plugin> plugin);

	/**
	 * Remove the attach thread from that Lua state
	 *
	 * @param L the Lua state from which thread
	 */
	void unregisterThread(lua_State *L);
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
