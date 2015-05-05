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
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include <IrccdConfig.h>

#include <Logger.h>

#if defined(WITH_JS)
#  include "Plugin.h"
#  include "TimerEvent.h"
#endif

#include "Transport.h"
#include "TransportCommand.h"
#include "TransportService.h"
#include "Server.h"
#include "ServerEvent.h"
#include "ServerService.h"

namespace irccd {

class Irccd {
private:
	/* Main loop */
	std::atomic<bool> m_running{true};
	std::condition_variable m_condition;
	std::mutex m_mutex;

	/* Events */
	std::queue<TransportCommand> m_transportCommands;
	std::queue<ServerEvent> m_serverEvents;

	/* Optional JavaScript plugins */
#if defined(WITH_JS)
	std::map<std::string, std::shared_ptr<Plugin>> m_plugins;
	std::map<std::string, PluginConfig> m_pluginConf;
	std::deque<TimerEvent> m_timerEvents;
#endif

	/* Identities */
	std::map<std::string, ServerIdentity> m_identities;

	/* Server and Transport threads */
	ServerService m_serverService;
	TransportService m_transportService;

public:
	Irccd();
	~Irccd();

	/* ------------------------------------------------
	 * Identity management
	 * ------------------------------------------------ */

	/**
	 * Add an identity.
	 *
	 * @param identity the identity
	 * @note If the identity already exists, it is overriden
	 */
	inline void identityAdd(ServerIdentity identity) noexcept
	{
		m_identities.emplace(identity.name, std::move(identity));
	}

	/**
	 * Get an identity, if not found, the default one is used.
	 *
	 * @param name the identity name
	 * @return the identity or default one
	 */
	inline ServerIdentity identityFind(const std::string &name) const noexcept
	{
		return m_identities.count(name) > 0 ? ServerIdentity() : m_identities.at(name);
	}

	/* ------------------------------------------------
	 * Server management
	 * ------------------------------------------------ */

	/**
	 * Add a new server to the application.
	 *
	 * @param args the arguments to pass to the Server constructor
	 */
	template <typename... Args>
	inline void serverAdd(Args&&... args) noexcept
	{
		try {
			m_serverService.add(std::forward<Args>(args)...);
		} catch (const std::exception &ex) {
			Logger::warning() << "server: " << ex.what() << std::endl;
		}
	}

	/**
	 * Disconnect a server and remove it.
	 *
	 * @param name the name
	 * @throw std::exception if the server does not exist
	 */
	inline void serverDisconnect(const std::string &name)
	{
		serverFind(name)->disconnect();

		m_serverService.reload();
	}

	/**
	 * Force reconnection to a server.
	 *
	 * @param name the server name
	 * @throw std::exception if the server does not exist
	 */
	inline void serverReconnect(const std::string &name)
	{
		serverFind(name)->reconnect();

		m_serverService.reload();
	}

	/**
	 * Find a server by name.
	 *
	 * @param name the server name
	 * @return the server
	 * @throw std::exception if the server does not exist
	 */
	inline std::shared_ptr<Server> serverFind(const std::string &name) const
	{
		return m_serverService.find(name);
	}

	/**
	 * Check if a server exists.
	 *
	 * @param name the name
	 * @return true if exists
	 */
	inline bool serverHas(const std::string &name) const noexcept
	{
		return m_serverService.has(name);
	}

	/* ------------------------------------------------
	 * Plugin management
	 * ------------------------------------------------ */

#if defined(WITH_JS)
	inline bool pluginIsLoaded(const std::string &name) const noexcept
	{
		return m_plugins.count(name) > 0;
	}

	/**
	 * Find a plugin.
	 *
	 * @param name the plugin id
	 * @return the plugin
	 * @throws std::out_of_range if not found
	 */
	inline std::shared_ptr<Plugin> pluginFind(const std::string &name) const
	{
		using namespace std::string_literals;

		if (m_plugins.count(name) == 0) {
			throw std::out_of_range("plugin "s + name + " not found"s);
		}

		return m_plugins.at(name);
	}

	/**
	 * Add plugin configuration for the specified plugin.
	 *
	 * @param name
	 * @param config
	 */
	inline void pluginAddConfig(std::string name, PluginConfig config)
	{
		m_pluginConf.emplace(std::move(name), std::move(config));
	}

	/**
	 * Load a plugin by a path or a name.
	 *
	 * @param path the plugin path
	 * @throw std::exception on failures
	 */
	void pluginLoad(std::string path);

	/**
	 * Unload a plugin and remove it.
	 *
	 * @param name the plugin id
	 */
	void pluginUnload(const std::string &name);

	/**
	 * Reload a plugin by calling onReload.
	 *
	 * @param name the plugin name
	 * @throw std::exception on failures
	 */
	inline void pluginReload(const std::string &name)
	{
		pluginFind(name)->onReload();
	}
#endif

	/* ------------------------------------------------
	 * Transport management
	 * ------------------------------------------------ */

	template <typename T, typename... Args>
	inline void transportAdd(Args&&... args)
	{
		try {
			m_transportService.add<T>(std::forward<Args>(args)...);
		} catch (const std::exception &ex) {
			Logger::warning() << "transport: " << ex.what() << std::endl;
		}
	}

	/* ------------------------------------------------
	 * For threads only
	 * ------------------------------------------------ */

	/**
	 * Add a new transport event.
	 *
	 * @param args the arguments to pass to the command constructor.
	 * @note Thread-safe
	 */
	inline void transportAddCommand(TransportCommand command)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			m_transportCommands.push(std::move(command));
		}

		m_condition.notify_one();
	}

#if defined(WITH_JS)
	/**
	 * Add a timer event.
	 *
	 * @param event the timer event
	 * @note Thread-safe
	 */
	inline void timerAddEvent(TimerEvent event)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			m_timerEvents.push_back(std::move(event));
		}

		m_condition.notify_one();
	}
#endif

	inline void serverAddEvent(ServerEvent event)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			m_serverEvents.push(std::move(event));
		}

		m_condition.notify_one();
	}

	/**
	 * Start the main loop.
	 */
	void run();

	/**
	 * Request to stop, usually from a signal.
	 */
	void stop();
};

/**
 * Unique and global instance.
 *
 * Used as a pointer to be sure that it is destroyed in the main and not
 * before anything else (Logger for instance).
 */
extern Irccd *irccd;

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
