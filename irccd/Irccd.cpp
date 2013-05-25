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

#include <unistd.h>

#include <Directory.h>
#include <Logger.h>
#include <Parser.h>
#include <SocketServerInet.h>
#include <SocketServerUnix.h>
#include <Util.h>

#include "Irccd.h"

using namespace irccd;
using namespace parser;
using namespace std;

/* {{{ Client handlers */

typedef function<void(Irccd *, const string &params)> Handler;

static void handleChannelNotice(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("CNOTICE needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->cnotice(params[1], params[2]);
	}
}

static void handleInvite(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() < 2) {
		Logger::warn("INVITE needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->invite(params[1], params[2]);
	}
}

static void handleJoin(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() < 2) {
		Logger::warn("JOIN needs at least 2 arguments");
	} else {
		string password = "";
		if (params.size() == 3)
			password = params[2];

		if ((server = irccd->findServer(params[0])) != nullptr)
			server->join(params[1], password);
	}
}

static void handleKick(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 4);

	if (params.size() < 2) {
		Logger::warn("KICK needs at least 3 arguments");
	} else {
		string reason = "";
		if (params.size() == 4)
			reason = params[3];

		if ((server = irccd->findServer(params[0])) != nullptr)
			server->kick(params[1], params[2], reason);
	}
}

static void handleMe(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("ME needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->me(params[1], params[2]);
	}
}

static void handleMessage(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("MSG needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->say(params[1], params[2]);
	}
}

static void handleMode(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 3) {
		Logger::warn("MODE needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->mode(params[1], params[2]);
	}
}

static void handleNick(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		Logger::warn("NICK needs 2 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->nick(params[1]);
	}
}

static void handleNotice(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("NOTICE needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->notice(params[1], params[2]);
	}
}

static void handlePart(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		Logger::warn("PART needs 2 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->part(params[1]);
	}
}

static void handleTopic(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("TOPIC needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->topic(params[1], params[2]);
	}
}

static void handleUserMode(Irccd *irccd, const string &cmd)
{
	Server *server;
	vector<string> params = Util::split(cmd, " \t", 1);

	if (params.size() != 2) {
		Logger::warn("UMODE needs 2 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->umode(params[1]);
	}
}

static map<string, Handler> createHandlers(void)
{
	map<string, Handler> handlers;

	handlers["CNOTICE"]	= handleChannelNotice;
	handlers["INVITE"]	= handleInvite;
	handlers["JOIN"]	= handleJoin;
	handlers["KICK"]	= handleKick;
	handlers["MODE"]	= handleMode;
	handlers["ME"]		= handleMe;
	handlers["MODE"]	= handleMode;
	handlers["MSG"]		= handleMessage;
	handlers["NOTICE"]	= handleNotice;
	handlers["NICK"]	= handleNick;
	handlers["PART"]	= handlePart;
	handlers["TOPIC"]	= handleTopic;
	handlers["UMODE"]	= handleUserMode;

	return handlers;
}

static map<string, Handler> handlers = createHandlers();

/* }}} */

/* --------------------------------------------------------
 * private methods
 * -------------------------------------------------------- */

Irccd * Irccd::m_instance = nullptr;

void Irccd::clientRead(SocketClient *client)
{
	char data[128 + 1];
	int length;

	/*
	 * First, read what is available and execute the command
	 * even if the client has disconnected.
	 */
	try {
		length = client->receive(data, sizeof (data) - 1);
		if (length > 0) {
			data[length] = '\0';
			client->addData(data);
		}
	} catch (Socket::Exception ex) {
		// do not log disconnected client, it's not really an error
		if (!ex.disconnected())
			Logger::log("%s", ex.what());

		m_clients.erase(remove(m_clients.begin(), m_clients.end(), client), m_clients.end());
		m_listener.removeClient(client);
	}

	if (client->isFinished())
		execute(client->getCommand());
}

void Irccd::execute(const string &cmd)
{
	string cmdName;
	size_t cmdDelim;

	cmdDelim = cmd.find_first_of(" \t");
	if (cmdDelim != string::npos) {
		cmdName = cmd.substr(0, cmdDelim);
		try {
			handlers.at(cmdName)(this, cmd.substr(cmdDelim + 1));
		} catch (out_of_range ex) {
			Logger::warn("invalid command %s", cmdName.c_str());
		}
	}
}

/* --------------------------------------------------------
 * Open functions, read config and servers
 * -------------------------------------------------------- */

void Irccd::openConfig(void)
{
	if (m_configPath.length() == 0)
		m_configPath = Util::configFilePath("irccd.conf");

	Parser config(m_configPath);

	if (!config.open()) {
		Logger::warn("failed to open: %s", m_configPath.c_str());
		exit(1);
	}

	// Extract parameters that are needed for the next
	if (config.hasOption("general", "plugin-path"))
		addPluginPath(config.getOption("general", "plugin-path").m_value);
	if (config.hasOption("general", "plugins")) {
		string list = config.getOption("general", "plugins").m_value;
		for (string s : Util::split(list, " \t"))
			addWantedPlugin(s);
	}

	// Modules should be opened first.
	openPlugins();

	openIdentities(config);
	openListeners(config);
	openServers(config);
}

void Irccd::openPlugins(void)
{
	// Get list of modules to load from config
	for (const string &s : m_pluginWanted) {
		Plugin *plugin;
		ostringstream oss;
		string finalPath;
		bool found = false;

		// Seek the plugin in the directories.
		for (const string &path : m_pluginDirs) {
			oss.str("");
			oss << path << "/" << s << ".lua";

			finalPath = oss.str();
			Logger::log("Checking for plugin %s", finalPath.c_str());
			if (Util::exist(finalPath)) {
				found = true;
				break;
			}
		}

		if (!found) {
			Logger::warn("Plugin %s not found", s.c_str());
			continue;
		}

		plugin = new Plugin(s);

		/*
		 * At this step, the open function will open the lua
		 * script, that script may want to call some bindings
		 * directly so we need to add it to the registered
		 * Lua plugins even if it has failed. So we remove
		 * it only on failure and expect plugins to be well
		 * coded.
		 */
		m_plugins.push_back(plugin);	// don't remove that

		if (!plugin->open(finalPath)) {
			Logger::warn("Failed to load module %s: %s",
			    s.c_str(), plugin->getError().c_str());

			m_plugins.erase(remove(m_plugins.begin(),
			    m_plugins.end(), plugin), m_plugins.end());
			delete plugin;
		}
	}
}

void Irccd::openIdentities(const parser::Parser &config)
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

			Logger::log("Found identity %s (%s, %s, \"%s\")", identity.m_name.c_str(),
			    identity.m_nickname.c_str(), identity.m_username.c_str(),
			    identity.m_realname.c_str());

			m_identities.push_back(identity);
		} catch (NotFoundException ex) {
			Logger::log("Section \"identity\" requires %s", ex.which().c_str());
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
			else if (type == "unix")
				extractUnix(s);
			else
				Logger::warn("unknown listener type `%s'", type.c_str());
		} catch (NotFoundException ex) {
			Logger::warn("Listener requires %s", ex.which().c_str());
		}
	}
}

void Irccd::extractInternet(const Section &s)
{
	SocketServerInet *inet;
	vector<string> protocols;
	string address, family;
	int port, finalFamily = 0;

	address = s.getOption<string>("address");
	family = s.getOption<string>("family");
	port = s.getOption<int>("port");

	// extract list of family
	protocols = Util::split(family, " \t");
	for (string p : protocols) {
		if (p == "ipv4")
			finalFamily |= Socket::Inet4;
		else if (p == "ipv6")
			finalFamily |= Socket::Inet6;
		else {
			Logger::warn("parameter family is one of them: ipv4, ipv6");
			Logger::warn("defaulting to ipv4");
	
			finalFamily |= Socket::Inet4;
		}
	}

	inet = new SocketServerInet(address, port, finalFamily);
	if (inet->bind() && inet->listen(16)) {
		Logger::log("Listening on port %u...", port);

		m_socketServers.push_back(inet);
		m_listener.addClient(inet);
	} else {
		Logger::warn("%s", inet->getErrorMessage().c_str());
	}
}

void Irccd::extractUnix(const Section &s)
{
	string path;
	SocketServerUnix *unix;

	path = s.requireOption<string>("path");
	unix = new SocketServerUnix(path);

	if (unix->create() && unix->bind() && unix->listen(16)) {
		Logger::log("Listening on %s...", path.c_str());

		m_socketServers.push_back(unix);
		m_listener.addClient(unix);
	} else {
		Logger::warn("%s", unix->getErrorMessage().c_str());
	}
}

void Irccd::openServers(const Parser &config)
{
	for (Section &s: config.findSections("server")) {
		Server *server = new Server();

		try {
			string name, address, commandToken, password, ident;
			int port;

			// General parameters
			if (s.hasOption("command-char"))
				server->setCommandChar(s.getOption<string>("command-char"));
			if (s.hasOption("join-invite"))
				server->setJoinInvite(s.getOption<bool>("join-invite"));
			if (s.hasOption("identity"))
				server->setIdentity(findIdentity(s.getOption<string>("identity")));

			// Get connection parameters
			name = s.requireOption<string>("name");
			address = s.requireOption<string>("address");
			port = s.requireOption<int>("port");

			if (s.hasOption("password"))
				password = s.getOption<string>("password");

			server->setConnection(name, address, port, false, password);

			// Extract channels to auto join
			extractChannels(s, server);

			m_servers.push_back(server);
		} catch (NotFoundException ex) {
			Logger::warn("Section \"server\" require %s", ex.which().c_str());
		}
	}
}

void Irccd::extractChannels(const Section &section, Server *server)
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

			server->addChannel(name, password);
		}
	}
}

/* --------------------------------------------------------
 * public methods
 * -------------------------------------------------------- */

Irccd::Irccd(void)
{
	Logger::setVerbose(false);

	// Set some defaults
	addPluginPath(MODDIR);		// see config.h.in
}

Irccd::~Irccd(void)
{
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

Plugin * Irccd::findPlugin(lua_State *state) const
{
	for (Plugin *p: m_plugins)
		if (p->getState() == state)
			return p;

	return nullptr;
}
	
vector<Server *> & Irccd::getServers(void)
{
	return m_servers;
}

vector<Plugin *> & Irccd::getPlugins(void)
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

Server * Irccd::findServer(const string &name)
{
	for (Server *s : m_servers)
		if (s->getName() == name)
			return s;

	Logger::warn("Could not find server with resource %s", name.c_str());
	return nullptr;
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

	Logger::warn("There is no identity named %s", name.c_str());
	return m_defaultIdentity;
}

int Irccd::run(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	openConfig();

	// Start all servers
	for (auto s : m_servers) {
		Logger::log("Trying to connect to %s...", s->getHost().c_str());
		s->startConnection();
	}

	// Wait for input
	for (;;) {
		// Todo: add a better thing there
		Socket *s;

		try {
			s = m_listener.select(0);
		} catch (SocketListener::EmptyException ex) {
			sleep(1);
			continue;
		}

		// Check if this is on a listening socket
		if (find(m_socketServers.begin(), m_socketServers.end(), s) != m_socketServers.end()) {
			SocketServer *server = (SocketServer *)s;
			SocketClient *clientSocket = server->accept();

			if (clientSocket == nullptr) {
				Logger::warn("Someone failed to connect %s", server->getErrorMessage().c_str());
			} else {
				m_listener.addClient(clientSocket);
				m_clients.push_back(clientSocket);
			}
		} else {
			clientRead((SocketClient *)s);
		}
	}

	return 0;
}
