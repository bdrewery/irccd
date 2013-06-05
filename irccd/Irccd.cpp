/*
 * Irccd.cpp -- main irccd class
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

#include <algorithm>
#include <functional>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <Logger.h>
#include <Parser.h>
#include <SocketTCP.h>
#include <Util.h>

#include "Irccd.h"

using namespace irccd;
using namespace std;

/* {{{ Client handlers */

typedef function<bool(SocketTCP &, const string &params)> Handler;

static bool handleChannelNotice(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "CNOTICE needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).cnotice(params[1], params[2]);

	return true;
}

static bool handleInvite(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "INVITE needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).invite(params[1], params[2]);

	return true;
}

static bool handleJoin(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() < 2) {
		string error = "JOIN needs at least 2 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	string password = "";
	if (params.size() == 3)
		password = params[2];

	Irccd::getInstance()->findServer(params[0]).join(params[1], password);

	return true;
}

static bool handleKick(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 4);

	if (params.size() < 3) {
		string error = "KICK needs at least 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	string reason = "";
	if (params.size() == 4)
		reason = params[3];

	Irccd::getInstance()->findServer(params[0]).kick(params[1], params[2], reason);

	return true;
}

static bool handleLoad(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 1);

	if (params.size() != 1) {
		string error = "LOAD needs 1 argument\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->loadPlugin(params[0]);

	return true;
}

static bool handleMe(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "ME needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).me(params[1], params[2]);

	return true;
}

static bool handleMessage(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "MSG needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).say(params[1], params[2]);

	return true;
}

static bool handleMode(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "MODE needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).mode(params[1], params[2]);

	return true;
}

static bool handleNick(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		string error = "NICK needs 2 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).nick(params[1]);

	return true;
}

static bool handleNotice(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "NOTICE needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).notice(params[1], params[2]);

	return true;
}

static bool handlePart(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		string error = "PART needs 2 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).part(params[1]);

	return true;
}

static bool handleReload(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 1);

	if (params.size() != 1) {
		string error = "RELOAD needs 1 argument\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->reloadPlugin(params[0]);

	return true;
}

static bool handleTopic(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		string error = "TOPIC needs 3 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).topic(params[1], params[2]);

	return true;
}

static bool handleUnload(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 1);

	if (params.size() != 1) {
		string error = "UNLOAD needs 1 argument\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->unloadPlugin(params[0]);

	return true;
}

static bool handleUserMode(SocketTCP &client, const string &cmd)
{
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		string error = "UMODE needs 2 arguments\n";
		client.send(error.c_str(), error.length());

		return false;
	}

	Irccd::getInstance()->findServer(params[0]).umode(params[1]);

	return true;
}

static map<string, Handler> createHandlers(void)
{
	map<string, Handler> handlers;

	handlers["CNOTICE"]	= handleChannelNotice;
	handlers["INVITE"]	= handleInvite;
	handlers["JOIN"]	= handleJoin;
	handlers["KICK"]	= handleKick;
	handlers["LOAD"]	= handleLoad;
	handlers["MODE"]	= handleMode;
	handlers["ME"]		= handleMe;
	handlers["MODE"]	= handleMode;
	handlers["MSG"]		= handleMessage;
	handlers["NOTICE"]	= handleNotice;
	handlers["NICK"]	= handleNick;
	handlers["PART"]	= handlePart;
	handlers["RELOAD"]	= handleReload;
	handlers["TOPIC"]	= handleTopic;
	handlers["UMODE"]	= handleUserMode;
	handlers["UNLOAD"]	= handleUnload;

	return handlers;
}

static map<string, Handler> handlers = createHandlers();

/* }}} */

/* --------------------------------------------------------
 * private methods
 * -------------------------------------------------------- */

Irccd * Irccd::m_instance = nullptr;

void Irccd::clientAdd(SocketTCP &server)
{
	SocketTCP client;

	try {
		SocketTCP client = server.accept();

		// Add to clients to read data
		m_clients[client] = "";
		m_listener.add(client);
	} catch (Socket::ErrorException ex) {
		Logger::warn("listener: could not accept client: %s", ex.what());
	}
}

void Irccd::clientRead(SocketTCP &client)
{
	char data[128 + 1];
	int length;
	bool removeIt = false;

	/*
	 * First, read what is available and execute the command
	 * even if the client has disconnected.
	 */
	try {
		length = client.recv(data, sizeof (data) - 1);

		// Disconnection?
		if (length == 0) {
			removeIt = true;
		} else {
			data[length] = '\0';

			// Copy the result
			string cmd = m_clients[client] + string(data);
			m_clients[client] = cmd;

			size_t position = cmd.find_first_of('\n');
			if (position != string::npos)
				execute(client, cmd.substr(0, position));
		}
	} catch (Socket::ErrorException ex) {
		Logger::log("listener: Could not read from client %s", ex.what());
		removeIt = true;
	}

	if (removeIt) {
		m_clients.erase(client);
		m_listener.remove(client);
	}
}

void Irccd::execute(SocketTCP &client, const string &cmd)
{
	string cmdName;
	size_t cmdDelim;

	cmdDelim = cmd.find_first_of(" \t");
	if (cmdDelim != string::npos) {
		cmdName = cmd.substr(0, cmdDelim);
		if (handlers.find(cmdName) == handlers.end())
			Logger::warn("listener: invalid command %s", cmdName.c_str());
		else {
			try {
				bool correct = handlers[cmdName](client, cmd.substr(cmdDelim + 1));
				if (correct)
					client.send("OK\n", 3);
			} catch (out_of_range ex) {
				ostringstream oss;
				string error;

				oss << ex.what() << "\n";
				error = oss.str();

				client.send(error.c_str(), error.length());
			} catch (Socket::ErrorException ex) {
				Logger::warn("listener: failed to send: %s", ex.what());
			}
		}
	}
}

bool Irccd::isPluginLoaded(const string &name)
{
#if defined(WITH_LUA)
	bool ret = true;

	try {
		(void)findPlugin(name);
	} catch (out_of_range ex) {
		ret = false;
	}

	return ret;
#else
	(void)name;

	return false;
#endif
}

/* --------------------------------------------------------
 * Open functions, read config and servers
 * -------------------------------------------------------- */

/*
 * Order is:
 * 1. Option -c passed to the command line
 * 2. User defined, usually ~/.config/irccd/irccd.conf
 * 3. Default cmake configured path, usually /usr/local/etc/irccd.conf
 */
void Irccd::openConfig(void)
{
	// Set some defaults
	addPluginPath(MODDIR);		// see config.h.in

	// Length empty means use user or default
	if (m_configPath.length() == 0) {
		// 2. User defined
		m_configPath = Util::configFilePath("irccd.conf");

		// 3. Not found, fallback to default path
		if (!Util::exist(m_configPath))
			m_configPath = DEFAULT_IRCCD_CONFIG;
	}

	Parser config(m_configPath);

	if (!config.open()) {
		Logger::warn("irccd: failed to open: %s", m_configPath.c_str());
		Logger::warn("irccd: no configuration could be found, exiting");
		exit(1);
	}

	Logger::log("irccd: using configuration %s", m_configPath.c_str());

	Section general = config.getSection("general");

	// Extract parameters that are needed for the next
	if (general.hasOption("plugin-path"))
		addPluginPath(general.getOption<string>("plugin-path"));
	if (general.hasOption("plugins")) {
		string list = general.getOption<string>("plugins");
		for (string s : Util::split(list, " \t"))
			addWantedPlugin(s);
	}

	// Modules should be opened first.
	openPlugins();

	openIdentities(config);
	openListeners(config);
	openServers(config);
}

void Irccd::loadPlugin(const string &name)
{
#if defined(WITH_LUA)
	ostringstream oss;
	string finalPath;
	bool found = false;

	if (isPluginLoaded(name))
		return;

	// Seek the plugin in the directories.
	for (const string &path : m_pluginDirs) {
		oss.str("");
		oss << path << "/" << name << ".lua";

		finalPath = oss.str();
		Logger::log("irccd: checking for plugin %s", finalPath.c_str());
		if (Util::exist(finalPath)) {
			found = true;
			break;
		}
	}

	if (!found) {
		Logger::warn("irccd: plugin %s not found", name.c_str());
	} else {
		/*
		 * At this step, the open function will open the lua
		 * script, that script may want to call some bindings
		 * directly so we need to add it to the registered
		 * Lua plugins even if it has failed. So we remove
		 * it only on failure and expect plugins to be well
		 * coded.
		 */
		
		m_pluginLock.lock();
		m_plugins.push_back(Plugin(name));		// don't remove that

		Plugin & plugin = m_plugins.back();
		m_pluginLock.unlock();

		if (!plugin.open(finalPath)) {
			Logger::warn("irccd: failed to load module %s: %s",
			    name.c_str(), plugin.getError().c_str());

			m_pluginLock.lock();
			m_plugins.pop_back();
			m_pluginLock.unlock();
		}
	}
#else
	Logger::warn("irccd: can't load plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::unloadPlugin(const string &name)
{
#if defined(WITH_LUA)
	vector<Plugin>::iterator i;	

	m_pluginLock.lock();
	i = find_if(m_plugins.begin(), m_plugins.end(), [&] (Plugin &p) -> bool {
		return p.getName() == name;
	});

	if (i == m_plugins.end())
		Logger::warn("irccd: there is no module %s loaded", name.c_str());
	else
		m_plugins.erase(i);

	m_pluginLock.unlock();
#else
	Logger::warn("irccd: can't unload plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::reloadPlugin(const string &name)
{
#if defined(WITH_LUA)
	try {
		findPlugin(name).onReload();
	} catch (out_of_range ex) {
		Logger::warn("irccd: %s", ex.what());
	}
#else
	Logger::warn("irccd: can't reload plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::openPlugins(void)
{
	// Get list of modules to load from config
	for (const string &s : m_pluginWanted) {
		loadPlugin(s);
	}
}

void Irccd::openIdentities(const Parser &config)
{
	for (Section &s: config.findSections("identity")) {
		Identity identity;

		try {
			identity.m_name = s.requireOption<string>("name");

			if (s.hasOption("nickname"))
				identity.m_nickname = s.getOption<string>("nickname");
			if (s.hasOption("username"))
				identity.m_username = s.getOption<string>("username");
			if (s.hasOption("realname"))
				identity.m_realname = s.getOption<string>("realname");
			if (s.hasOption("version"))
				identity.m_ctcpversion = s.getOption<string>("version");

			Logger::log("identity: found identity %s (%s, %s, \"%s\")", identity.m_name.c_str(),
			    identity.m_nickname.c_str(), identity.m_username.c_str(),
			    identity.m_realname.c_str());

			m_identities.push_back(identity);
		} catch (NotFoundException ex) {
			Logger::log("identity: missing parameter %s", ex.which().c_str());
		}
	};
}

void Irccd::openListeners(const Parser &config)
{
	for (Section &s : config.findSections("listener")) {
		try {
			string type;

			type = s.requireOption<string>("type");

			if (type == "internet")
				extractInternet(s);
			else if (type == "unix") {
#if !defined(_WIN32)
				extractUnix(s);
#else
				Logger::warn("listener: unix sockets are not supported on Windows");
#endif
			} else
				Logger::warn("listener: unknown listener type `%s'", type.c_str());
		} catch (NotFoundException ex) {
			Logger::warn("listener: missing parameter %s", ex.which().c_str());
		}
	}
}

void Irccd::extractInternet(const Section &s)
{
	SocketTCP inet;
	vector<string> protocols;
	string address, family;
	int port;
	bool ipv4 = false, ipv6 = false;

	address = s.getOption<string>("address");
	family = s.getOption<string>("family");
	port = s.getOption<int>("port");

	// extract list of family
	protocols = Util::split(family, " \t");

	for (string p : protocols) {
		if (p == "ipv4")
			ipv4 = true;
		else if (p == "ipv6")
			ipv6 = true;
		else {
			Logger::warn("listener: parameter family is one of them: ipv4, ipv6");
			Logger::warn("listener: defaulting to ipv4");
	
			ipv4 = true;
			ipv6 = false;
		}
	}

	try {
		int reuse = 1;

		inet.create((ipv6) ? AF_INET6 : AF_INET);
		inet.set(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));
		if (ipv6) {
			int mode = !ipv4;
			inet.set(IPPROTO_IPV6, IPV6_V6ONLY, &mode, sizeof (mode));
		}

		inet.bind(BindPointIP(address, port, (ipv6) ? AF_INET6 : AF_INET));
		inet.listen(64);

		// On success add to listener and servers
		m_socketServers.push_back(inet);
		m_listener.add(inet);

		Logger::log("listener: listening for clients on port %d...", port);
	} catch (Socket::ErrorException ex) {
		Logger::warn("listener: internet socket error: %s", ex.what());
	}
}

#if !defined(_WIN32)
void Irccd::extractUnix(const Section &s)
{
	string path;
	SocketTCP unix;

	path = s.requireOption<string>("path");

	// First remove the dust
	if (Util::exist(path) && remove(path.c_str()) < 0)
		Logger::warn("listener: error removing %s: %s", path.c_str(), strerror(errno));
		else {

		try {
			unix.create(AF_UNIX);
			unix.bind(UnixPoint(path));
			unix.listen(64);

			Logger::log("listener: listening for clients on %s...", path.c_str());
		} catch (Socket::ErrorException ex) {
			Logger::warn("listener: unix socket error: %s", ex.what());
		}
	}
}
#endif

void Irccd::openServers(const Parser &config)
{
	for (Section &s: config.findSections("server")) {
		Server server;

		try {
			string name, address, commandToken, password, ident;
			int port;

			// General parameters
			if (s.hasOption("command-char"))
				server.setCommandChar(s.getOption<string>("command-char"));
			if (s.hasOption("join-invite"))
				server.setJoinInvite(s.getOption<bool>("join-invite"));
			if (s.hasOption("identity"))
				server.setIdentity(findIdentity(s.getOption<string>("identity")));

			// Get connection parameters
			name = s.requireOption<string>("name");
			address = s.requireOption<string>("address");
			port = s.requireOption<int>("port");

			if (s.hasOption("password"))
				password = s.getOption<string>("password");

			server.setConnection(name, address, port, false, password);

			// Extract channels to auto join
			extractChannels(s, server);

			m_servers.push_back(std::move(server));
		} catch (NotFoundException ex) {
			Logger::warn("server: missing parameter %s", ex.which().c_str());
		}
	}
}

void Irccd::extractChannels(const Section &section, Server &server)
{
	vector<string> channels;
	string list, name, password;
	size_t colon;

	if (section.hasOption("channels")) {
		list = section.getOption<string>("channels");
		channels = Util::split(list, " \t");

		for (string s : channels) {
			// detect an optional channel password
			colon = s.find_first_of(':');
			if (colon != string::npos) {
				name = s.substr(0, colon);
				password = s.substr(colon + 1);
			} else {
				name = s;
				password = "";
			}

			server.addChannel(name, password);
		}
	}
}

/* --------------------------------------------------------
 * public methods
 * -------------------------------------------------------- */

Irccd::Irccd(void)
{
	Socket::init();

	Logger::setVerbose(false);
}

Irccd::~Irccd(void)
{
	Socket::finish();
}

Irccd * Irccd::getInstance(void)
{
	if (m_instance == nullptr)
		m_instance = new Irccd();

	return m_instance;
}

void Irccd::addPluginPath(const string &path)
{
	m_pluginDirs.push_back(path);
}

void Irccd::addWantedPlugin(const string &name)
{
	m_pluginWanted.push_back(name);
}

#if defined(WITH_LUA)

Plugin & Irccd::findPlugin(lua_State *state)
{
	m_pluginLock.lock();
	for (Plugin &p : m_plugins)
		if (p.getState() == state) {
			m_pluginLock.unlock();
			return p;
		}

	m_pluginLock.unlock();

	// This one should not happen
	throw out_of_range("plugin not found");
}

Plugin & Irccd::findPlugin(const string &name)
{
	ostringstream oss;

	m_pluginLock.lock();
	for (Plugin &p : m_plugins)
		if (p.getName() == name) {
			m_pluginLock.unlock();
			return p;
		}

	m_pluginLock.unlock();

	oss << "plugin " << name << " not found";

	throw out_of_range(oss.str());
}

#endif
	
vector<Server> & Irccd::getServers(void)
{
	return m_servers;
}

vector<Plugin> & Irccd::getPlugins(void)
{
	return m_plugins;
}

void Irccd::setConfigPath(const string &path)
{
	m_configPath = path;
}

void Irccd::setVerbosity(bool verbose)
{
	Logger::setVerbose(verbose);
}

Server & Irccd::findServer(const string &name)
{
	for (Server &s : m_servers)
		if (s.getName() == name)
			return s;

	throw out_of_range("could not find server with resource " + name);
}

const Identity & Irccd::findIdentity(const string &name)
{
	/*
	 * When name is length 0 that mean user hasn't defined an identity
	 * because it's optional, we don't write an empty message error.
	 */

	if (name.length() == 0)
		return m_defaultIdentity;

	for (const Identity &i : m_identities)
		if (i.m_name == name)
			return i;

	Logger::warn("identity: %s not found", name.c_str());

	return m_defaultIdentity;
}

int Irccd::run(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openConfig();

	// Start all servers
	for (Server &s : m_servers) {
		Logger::log("server %s: trying to connect to %s...",
		    s.getName().c_str(), s.getHost().c_str());
		s.startConnection();
	}

	// Wait for input
	for (;;) {
		// Nothing to do, just do a sleep to avoid high process usage,
		// the IRC servers thread do everything else.
		if (m_listener.size() == 0) {
			Util::usleep(500);
			continue;
		}

		try {
			SocketTCP &s = (SocketTCP &)m_listener.select(0);

			// Check if this is on a listening socket
			if (find(m_socketServers.begin(), m_socketServers.end(), s) != m_socketServers.end()) {
				clientAdd(s);
			} else {
				clientRead(s);
			}
		} catch (Socket::ErrorException ex) {
			Logger::warn("listener: client error: %s", ex.what());
		}
	}

	return 0;
}
