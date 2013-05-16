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

typedef function<void(Irccd *, const std::string &params)> Handler;

static void handleJoin(Irccd *irccd, const string &cmd)
{
	Server *server;
	std::vector<string> params = Util::split(cmd, " \t", 3);

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
	std::vector<string> params = Util::split(cmd, " \t", 4);

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
	std::vector<string> params = Util::split(cmd, " \t", 3);

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
	std::vector<string> params = Util::split(cmd, " \t", 3);

	if (params.size() != 3) {
		Logger::warn("MSG needs 3 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->say(params[1], params[2]);
	}
}

static void handleNick(Irccd *irccd, const string &cmd)
{
	Server *server;
	std::vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		Logger::warn("NICK needs 2 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->nick(params[1]);
	}
}

static void handlePart(Irccd *irccd, const string &cmd)
{
	Server *server;
	std::vector<string> params = Util::split(cmd, " \t", 2);

	if (params.size() != 2) {
		Logger::warn("PART needs 2 arguments");
	} else {
		if ((server = irccd->findServer(params[0])) != nullptr)
			server->part(params[1]);
	}
}

static map<string, Handler> createHandlers(void)
{
	map<string, Handler> handlers;

	handlers["JOIN"] = handleJoin;
	handlers["KICK"] = handleKick;
	handlers["ME"] = handleMe;
	handlers["MSG"] = handleMessage;
	handlers["NICK"] = handleNick;
	handlers["PART"] = handlePart;

	return handlers;
}

static map<string, Handler> handlers = createHandlers();

/* }}} */

/* --------------------------------------------------------
 * private methods
 * -------------------------------------------------------- */

Irccd * Irccd::m_instance = nullptr;

void Irccd::execute(const std::string &cmd)
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

		m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), client), m_clients.end());
		m_listener.removeClient(client);
	}

	if (client->isFinished())
		execute(client->getCommand());
}

void Irccd::extractInternet(const Section &s)
{
	SocketServerInet *inet;
	vector<string> protocols;
	string address, family;
	int port, finalFamily = SocketServerInet::Inet4;

	s.getOption<string>("address", address, true);
	s.getOption<string>("family", family, true);
	s.getOption<int>("port", port, true);

	// extract list of family
	protocols = Util::split(family, " \t");
	for (string p : protocols) {
		if (p == "ipv4")
			finalFamily |= SocketServerInet::Inet4;
		else if (p == "ipv6")
			finalFamily |= SocketServerInet::Inet6;
		else {
			Logger::warn("parameter family is one of them: ipv4, ipv6");
			Logger::warn("defaulting to ipv4");
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

	s.getOption<string>("path", path, true);

	unix = new SocketServerUnix(path);

	if (unix->create() && unix->bind() && unix->listen(16)) {
		Logger::log("Listening on %s...", path.c_str());

		m_socketServers.push_back(unix);
		m_listener.addClient(unix);
	} else {
		Logger::warn("%s", unix->getErrorMessage().c_str());
	}
}

void Irccd::readListeners(const Parser &config)
{
	for (Section &s : config.findSections("listener")) {
		try {
			string type;

			s.getOption<string>("type", type);

			if (type == "internet")
				extractInternet(s);
			else if (type == "unix")
				extractUnix(s);
			else
				Logger::warn("unknown listener type `%s'", type.c_str());
		} catch (NotFoundException ex) {
			Logger::warn("missing required parameter %s", ex.which().c_str());
		}
	}
}

void Irccd::extractChannels(const Section &section, Server *server)
{
	vector<string> channels;
	string list, name, password;
	size_t colon;

	if (section.hasOption("channels")) {
		section.getOption<string>("channels", list);
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

void Irccd::readServers(const Parser &config)
{
	for (Section &s: config.findSections("server")) {
		Server *server = new Server();

		try {
			string name, address, commandToken, password, ident;
			int port;
			bool ssl = false;

			// General parameters
			if (s.getOption<string>("command-char", commandToken))
				server->setCommandToken(commandToken);

			// Get connection parameters
			s.getOption<string>("name", name, true);
			s.getOption<string>("address", address, true);
			s.getOption<int>("port", port, true);
			s.getOption<string>("password", password);
			s.getOption<bool>("ssl", ssl);

			// Extract the identity
			s.getOption<string>("identity", ident);

			server->setConnection(name, address, port, ssl, password);
			server->setIdentity(findIdentity(ident));

			extractChannels(s, server);
			m_servers.push_back(server);
		} catch (NotFoundException ex) {
			Logger::warn("missing required parameter %s", ex.which().c_str());
		}
	}
}

void Irccd::readIdentities(const parser::Parser &config)
{
	for (Section &s: config.findSections("identity")) {
		Identity identity;

		s.getOption<string>("name", identity.m_name);
		s.getOption<string>("nickname", identity.m_nickname);
		s.getOption<string>("username", identity.m_username);
		s.getOption<string>("realname", identity.m_realname);
		s.getOption<string>("version", identity.m_ctcpversion);

		m_identities.push_back(identity);
	};
}

void Irccd::readConfig(void)
{
	Parser config(m_configPath);

	if (!config.open()) {
		Logger::warn("failed to open: %s", m_configPath.c_str());
		exit(1);
	}

	readIdentities(config);
	readListeners(config);
	readServers(config);
}

void Irccd::openConfig(void)
{
	if (m_configPath.length() == 0) {
		m_configPath = Util::configFilePath("irccd.conf");
		readConfig();
	} else
		readConfig();
}

void Irccd::openModules(void)
{
	Directory dir(m_modulePath);

	if (!dir.open(true)) {
		Logger::warn("Failed to open %s: %s", m_modulePath.c_str(),
		    dir.getError().c_str());
	} else {
		for (const Entry &s : dir.getEntries()) {
			Plugin *plugin;
			ostringstream oss;

			if (s.m_isDirectory) {
				Logger::warn("%s is a directory", s.m_name.c_str());
				continue;
			}

			Logger::log("Opening module %s", s.m_name.c_str());

			plugin = new Plugin();
			oss << m_modulePath;
			oss << "/" << s.m_name;

			/*
			 * At this step, the open function will open the lua
			 * script, that script may want to call some bindings
			 * directly so we need to add it to the registered
			 * Lua plugins even if it has failed. So we remove
			 * it only on failure and expect plugins to be well
			 * coded.
			 */
			m_plugins.push_back(plugin);
			if (!plugin->open(oss.str())) {
				Logger::warn("failed to load module %s: %s",
				    s.m_name.c_str(), plugin->getError().c_str());

				m_plugins.erase(remove(m_plugins.begin(),
				    m_plugins.end(), plugin), m_plugins.end());
				delete plugin;
			}
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
	m_modulePath = string(MODDIR);		// see config.h.in
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

Plugin * Irccd::findPlugin(lua_State *state) const
{
	for (Plugin *p: m_plugins)
		if (p->getState() == state)
			return p;

	return nullptr;
}

void Irccd::setModulePath(const std::string &path)
{
	m_modulePath = path;
}
	
vector<Server *> & Irccd::getServers(void)
{
	return m_servers;
}

vector<Plugin *> & Irccd::getPlugins(void)
{
	return m_plugins;
}

void Irccd::setConfigPath(const std::string &path)
{
	m_configPath = path;
}

void Irccd::setVerbosity(bool verbose)
{
	Logger::setVerbose(verbose);
}

Server * Irccd::findServer(const std::string &name)
{
	for (Server *s : m_servers)
		if (s->getName() == name)
			return s;

	Logger::warn("Could not find server with resource %s", name.c_str());
	return nullptr;
}

const Identity & Irccd::findIdentity(const std::string &name)
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

	openModules();
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
