/*
 * Irccd.h -- main irccd class
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

#ifndef _IRCCD_H_
#define _IRCCD_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include <IrccdConfig.h>

#include <Logger.h>
#include <Socket.h>
#include <SocketAddress.h>

#include "Plugin.h"
#include "Server.h"
#include "TransportServer.h"

namespace irccd {

using Event = std::function<void ()>;
using Events = std::vector<Event>;

/**
 * @class ServerEvent
 * @brief Structure that owns several informations about an IRC event
 *
 * This structure is used to dispatch the IRC event to the plugins and the transports.
 */
class ServerEvent {
public:
	std::string server;
	std::string origin;
	std::string target;
	std::string json;
	std::function<std::string (Plugin &)> name;
	std::function<void (Plugin &)> exec;
};

/**
 * @enum ServerMessageType
 * @brief Describe which type of message has been received
 *
 * On channels and queries, you may have a special command or a standard message depending on the
 * beginning of the message.
 *
 * Example: `!reminder help' may invoke the command event if a plugin reminder exists.
 */
enum class ServerMessageType {
	Command,		//!< special command
	Message			//!< standard message
};

/**
 * @brief Combine the type of message and its content
 */
using ServerMessagePair = std::pair<std::string, ServerMessageType>;

/**
 * @brief Table of servers
 */
using Servers = std::unordered_map<std::string, std::shared_ptr<Server>>;

template <typename T>
using LookupTable = std::unordered_map<SocketAbstract::Handle, std::shared_ptr<T>>;

/**
 * @class Irccd
 * @brief Irccd main instance
 *
 * This class is used as the main application event loop, it stores servers, plugins and transports.
 *
 * In a general manner, no code in irccd is thread-safe because irccd is mono-threaded except the JavaScript timer
 * API.
 *
 * If you plan to add more threads to irccd, then the simpliest and safest way to execute thread-safe code is to
 * register an event using Irccd::addEvent function which will be called during the event loop dispatching.
 *
 * Thus, except noticed as thread-safe, no function is assumed to be.
 */
class Irccd {
private:
	/* Main loop */
	static std::atomic<bool> m_running;

	/* Mutex for addEvent */
	std::mutex m_mutex;

	/* IPC */
	SocketTcp<address::Ipv4> m_socketServer;
	SocketTcp<address::Ipv4> m_socketClient;

	/* Event loop */
	Events m_events;

	/* Servers */
	Servers m_servers;

	/* Optional JavaScript plugins */
#if defined(WITH_JS)
	std::unordered_map<std::string, std::shared_ptr<Plugin>> m_plugins;
	std::unordered_map<std::string, PluginConfig> m_pluginConf;
#endif

	/* Identities */
	std::unordered_map<std::string, ServerIdentity> m_identities;

	/* Lookup tables */
	LookupTable<TransportClientAbstract> m_lookupTransportClients;
	LookupTable<TransportServerAbstract> m_lookupTransportServers;

	/* Server slots */
	void handleServerOnChannelNotice(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string notice);
	void handleServerOnConnect(std::shared_ptr<Server> server);
	void handleServerOnInvite(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target);
	void handleServerOnJoin(std::shared_ptr<Server> server, std::string origin, std::string channel);
	void handleServerOnKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason);
	void handleServerOnMessage(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message);
	void handleServerOnMe(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message);
	void handleServerOnMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg);
	void handleServerOnNick(std::shared_ptr<Server> server, std::string origin, std::string nickname);
	void handleServerOnNotice(std::shared_ptr<Server> server, std::string origin, std::string message);
	void handleServerOnPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason);
	void handleServerOnQuery(std::shared_ptr<Server> server, std::string origin, std::string message);
	void handleServerOnTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic);
	void handleServerOnUserMode(std::shared_ptr<Server> server, std::string origin, std::string mode);

	/* Transport slots */
	void handleTransportChannelNotice(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string message);
	void handleTransportConnect();
	void handleTransportDisconnect(std::shared_ptr<TransportClientAbstract> tc, std::string server);
	void handleTransportInvite(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string target, std::string channel);
	void handleTransportJoin(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string password);
	void handleTransportKick(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string target, std::string channel, std::string reason);
	void handleTransportMe(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string message);
	void handleTransportMessage(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string message);
	void handleTransportMode(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string mode);
	void handleTransportNick(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string nickname);
	void handleTransportNotice(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string target, std::string message);
	void handleTransportPart(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string reason);
	void handleTransportReconnect(std::shared_ptr<TransportClientAbstract> tc, std::string server);
	void handleTransportReload(std::shared_ptr<TransportClientAbstract> tc, std::string plugin);
	void handleTransportTopic(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string channel, std::string topic);
	void handleTransportUnload(std::shared_ptr<TransportClientAbstract> tc, std::string plugin);
	void handleTransportUserMode(std::shared_ptr<TransportClientAbstract> tc, std::string server, std::string mode);

	/* Timer slots */
#if defined(WITH_JS)
	void handleTimerSignal(std::shared_ptr<Plugin>, std::shared_ptr<Timer>);
	void handleTimerEnd(std::shared_ptr<Plugin>, std::shared_ptr<Timer>);
#endif

	/* Private helpers */
#if defined(WITH_JS)
	ServerMessagePair parseMessage(std::string message, Server &server, Plugin &plugin);
#endif
	void dispatch();
	void process(fd_set &setinput, fd_set &setoutput);
	void exec();

	/* Private event helpers */
	void addTransportEvent(std::shared_ptr<TransportClientAbstract> tc, Event ev) noexcept;
	void addServerEvent(ServerEvent) noexcept;

public:
	/**
	 * Constructor that instanciate IPC.
	 */
	Irccd();

	/* ------------------------------------------------
	 * Event loop
	 * ------------------------------------------------ */

	/**
	 * Add an event to the queue. This will immediately signals the event loop to interrupt itself to dispatch
	 * the pending events.
	 *
	 * @param ev the event
	 * @note Thread-safe
	 */
	void addEvent(Event ev) noexcept;

	/* ------------------------------------------------
	 * Identity management
	 * ------------------------------------------------ */

	/**
	 * Add an identity.
	 *
	 * @param identity the identity
	 * @note If the identity already exists, it is overriden
	 */
	inline void addIdentity(ServerIdentity identity) noexcept
	{
		m_identities.emplace(identity.name, std::move(identity));
	}

	/**
	 * Get an identity, if not found, the default one is used.
	 *
	 * @param name the identity name
	 * @return the identity or default one
	 */
	inline ServerIdentity findIdentity(const std::string &name) const noexcept
	{
		auto it = m_identities.find(name);

		return it == m_identities.end() ? ServerIdentity{} : it->second;
	}

	/* ------------------------------------------------
	 * Server management
	 * ------------------------------------------------ */

	/**
	 * Add a new server to the application.
	 *
	 * @param sv the server
	 */
	void addServer(std::shared_ptr<Server> sv) noexcept;

	/**
	 * Find a server by name.
	 *
	 * @param name the server name
	 * @return the server
	 * @throw std::exception if the server does not exist
	 */
	inline std::shared_ptr<Server> findServer(const std::string &name) const
	{
		return m_servers.at(name);
	}

	/**
	 * Check if a server exists.
	 *
	 * @param name the name
	 * @return true if exists
	 */
	inline bool containsServer(const std::string &name) const noexcept
	{
		return m_servers.count(name) != 0;
	}

	/* ------------------------------------------------
	 * Transport management
	 * ------------------------------------------------ */

	/**
	 * Add a transport server.
	 *
	 * @param ts the transport server
	 */
	void addTransport(std::shared_ptr<TransportServerAbstract> ts);

	/* ------------------------------------------------
	 * Plugin management
	 * ------------------------------------------------ */

	/**
	 * Find a plugin.
	 *
	 * @param name the plugin id
	 * @return the plugin
	 * @throws std::out_of_range if not found
	 */
	std::shared_ptr<Plugin> findPlugin(const std::string &name) const;

	/**
	 * Add plugin configuration for the specified plugin.
	 *
	 * @param name
	 * @param config
	 */
	inline void addPluginConfig(std::string name, PluginConfig config)
	{
		m_pluginConf.emplace(std::move(name), std::move(config));
	}

	/**
	 * Load a plugin by a path or a name.
	 *
	 * @param path the plugin path
	 * @throw std::exception on failures
	 */
	void loadPlugin(std::string path);

#if 0

	/**
	 * Unload a plugin and remove it.
	 *
	 * @param name the plugin id
	 */
	void unloadPlugin(const std::string &name);

	/**
	 * Reload a plugin by calling onReload.
	 *
	 * @param name the plugin name
	 * @throw std::exception on failures
	 */
	inline void reloadPlugin(const std::string &name)
	{
		pluginFind(name)->onReload();
	}
#endif

	/**
	 * Start the main loop.
	 */
	void run();

	/**
	 * Request to stop, usually from a signal.
	 */
	static void stop();
};

} // !irccd

#if 0

/**
 * @file Irccd.h
 * @brief Main irccd class
 */

#include <exception>
#include <mutex>
#include <sstream>
#include <unordered_set>

#include <Parser.h>
#include <Singleton.h>
#include <Util.h>
#include <IrccdConfig.h>

#include "Server.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

/**
 * @brief The irccd namespace
 */
namespace irccd {

/* --------------------------------------------------------
 * Irccd main class
 * -------------------------------------------------------- */

/**
 * @enum Options
 * @brief Options
 */
enum Options {
	Config		= 'c',
	Foreground	= 'f',
	Verbose		= 'v',
	PluginPath	= 'p',
	PluginWanted	= 'P'
};

/**
 * List of identities.
 */
using IdentityList	= std::vector<Server::Identity>;

/**
 * @class Irccd
 * @brief The main irccd class
 */
class Irccd : public Singleton<Irccd> {
private:
	friend class Singleton<Irccd>;
	friend std::unique_ptr<Irccd> std::make_unique<Irccd>();

	using Wanted	= std::vector<std::string>;
	using Overriden	= std::unordered_map<char, bool>;

	// Ignition
	bool		m_running { true };
	bool		m_foreground { false };

	// Config
	std::string	m_configPath;		//! config file path
	Overriden	m_overriden;		//! overriden parameters

	// Identities
	IdentityList	m_identities;		//! user identities
	Server::Identity m_defaultIdentity;	//! default identity

#if !defined(_WIN32)
	uid_t		m_uid = 0;		//! uid to run
	gid_t		m_gid = 0;		//! gid to run
#endif

	/*
	 * Plugin specified by commands line that should be
	 * loaded after initialization.
	 */
#if defined(WITH_LUA)
	Wanted m_wantedPlugins;
#endif

	Irccd();

	bool isOverriden(char c);

	/*
	 * Open will call the below function in the same order they are
	 * declared, do not change that.
	 */
	void openConfig();
	void readGeneral(const Parser &config);		// [general]
	int parse(const Section &section, const char *name, bool isgid);
	std::string idname(bool isgid);

	void readPlugins(const Parser &config);		// [plugins]
	void readIdentities(const Parser &config);	// [identity]

	/* ------------------------------------------------
	 * [rule]
	 * ------------------------------------------------ */

	void readRules(const Parser &config);

	std::unordered_set<std::string> getList(const Section &s, const std::string &name) const
	{
		std::unordered_set<std::string> result;

		if (s.hasOption(name)) {
			for (const auto &i : Util::split(s.getOption<std::string>(name), " \t")) {
				result.insert(i);
			}
		}

		return result;
	}

	/* ------------------------------------------------
	 * [listener]
	 * ------------------------------------------------ */

	void readListeners(const Parser &config);
	void extractInternet(const Section &s);

#if !defined(_WIN32)
	void extractUnix(const Section &s);
#endif

	/* ------------------------------------------------
	 * [server]
	 * ------------------------------------------------ */

	void readServers(const Parser &config);
	void extractChannels(const Section &section, std::shared_ptr<Server> &server);

public:
	~Irccd();

	/**
	 * Initialize common settings.
	 */
	void initialize();

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
	 * Only for command line, defer a plugin to be load after the
	 * configuration file load.
	 *
	 * @param name the name
	 */
	void deferPlugin(const std::string &name);

	/**
	 * Find an identity.
	 *
	 * @param name the identity's resource name.
	 * @return an identity or the default one
	 */
	const Server::Identity &findIdentity(const std::string &name);

	/**
	 * Run the application.
	 *
	 * @return the error code
	 */
	int run();

	/**
	 * Check if the server is still running.
	 *
	 * @return the status
	 */
	bool isRunning() const;

	/**
	 * Tells irccd to stop.
	 */
	void shutdown();

	/**
	 * Stop all threads and everything else.
	 */
	void stop();
};

} // !irccd

#endif

#endif // !_IRCCD_H_
