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
	void extractInternet(const Section &s, int type);

#if !defined(_WIN32)
	void extractUnix(const Section &s, int type);
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

#endif // !_IRCCD_H_
