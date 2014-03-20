/*
 * Irccd.h -- main irccd class
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

#ifndef _IRCCD_H_
#define _IRCCD_H_

/**
 * @file Irccd.h
 * @brief Main irccd class
 */

#include <exception>
#include <mutex>
#include <sstream>

#include <Parser.h>
#include <config.h>

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
class Irccd {
private:
	using Wanted	= std::vector<std::string>;
	using Overriden	= std::unordered_map<char, bool>;

	static Irccd	m_instance;		//! unique instance

	// Ignition
	bool		m_running;		//! forever loop
	bool		m_foreground;		//! run to foreground

	// Config
	std::string	m_configPath;		//! config file path
	Overriden	m_overriden;		//! overriden parameters

	// Identities
	IdentityList	m_identities;		//! user identities
	Server::Identity m_defaultIdentity;	//! default identity

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
	void readPlugins(const Parser &config);		// [plugins]
	void readIdentities(const Parser &config);	// [identity]

	void readListeners(const Parser &config);
	void extractInternet(const Section &s, int type);

#if !defined(_WIN32)
	void extractUnix(const Section &s, int type);
#endif

	void readServers(const Parser &config);
	void extractChannels(const Section &section, Server::Ptr server);

public:
	~Irccd();

	/**
	 * Get the unique irccd instance.
	 *
	 * @return the irccd instance
	 */
	static Irccd &getInstance();

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

#endif // !_IRCCD_H_
