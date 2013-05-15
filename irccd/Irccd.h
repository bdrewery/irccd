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

#include <sstream>

#include <Parser.h>
#include <SocketServer.h>
#include <SocketListener.h>

#include <config.h>

#include "Server.h"
#include "Plugin.h"

namespace irccd {

class Irccd {
private:
	static Irccd *m_instance;			/*! unique instance */

	// Plugins
	std::vector<Plugin *> m_plugins;		/*! list of plugins */

	// Config
	std::string m_configPath;			/*! config file path */

	std::vector<Server *> m_servers;		/*! list of servers */

	// Socket clients and listeners
	std::vector<SocketServer *> m_socketServers;	/*! socket servers */
	std::vector<SocketClient *> m_clients;		/*! socket clients */
	SocketListener m_listener;			/*! socket listener */

	// Identities
	std::vector<Identity> m_identities;		/*! user identities */
	Identity m_defaultIdentity;			/*! default identity */

	// Some paths
	std::string m_modulePath;			/*! plugin directory */

	void execute(const std::string &cmd);
	void clientRead(SocketClient *client);

	/* [listener] */

	void extractInternet(const parser::Section &s);
	void extractUnix(const parser::Section &s);
	void readListeners(const parser::Parser &config);

	/* [server] */

	void extractChannels(const parser::Section &section, Server *server);
	void readServers(const parser::Parser &config);

	/* [identity] */
	void readIdentities(const parser::Parser &config);

	void readConfig(void);
	void openConfig(void);

	void openModules(void);

	Irccd(void);
public:
	~Irccd(void);

	/**
	 * Get the unique irccd instance.
	 *
	 * @return the irccd instance
	 */
	static Irccd * getInstance(void);

	/**
	 * Find a plugin by it's associated Lua State, so it can
	 * be retrieved by every Lua bindings.
	 *
	 * @param state the Lua state
	 * @return the plugin
	 */
	Plugin * findPlugin(lua_State *state) const;

	/**
	 * Set the plugin directory where to find .lua files
	 *
	 * @param path the directory path
	 */
	void setModulePath(const std::string &path);

	/**
	 * Get the servers list
	 *
	 * @return the list of servers
	 */
	std::vector<Server *> & getServers(void);

	/**
	 * Get plugins.
	 *
	 * @return a list of plugins
	 */
	std::vector<Plugin *> & getPlugins(void);

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
	Server * findServer(const std::string &name);

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
	int run(int argc, char **argv);
};

} // !irccd

#endif // !_IRCCD_H_
