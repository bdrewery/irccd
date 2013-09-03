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

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

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

	Irccd::getInstance()->findServer(params[0])->cnotice(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->invite(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->join(params[1], password);

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

	Irccd::getInstance()->findServer(params[0])->kick(params[1], params[2], reason);

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

	Irccd::getInstance()->findServer(params[0])->me(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->say(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->mode(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->nick(params[1]);

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

	Irccd::getInstance()->findServer(params[0])->notice(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->part(params[1]);

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

	Irccd::getInstance()->findServer(params[0])->topic(params[1], params[2]);

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

	Irccd::getInstance()->findServer(params[0])->umode(params[1]);

	return true;
}

static map<string, Handler> createHandlers()
{
	map<string, Handler> handlers;

	handlers["CNOTICE"]	= handleChannelNotice;
	handlers["INVITE"]	= handleInvite;
	handlers["JOIN"]	= handleJoin;
	handlers["KICK"]	= handleKick;
	handlers["LOAD"]	= handleLoad;
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

bool Irccd::isOverriden(char c)
{
	return m_overriden.find(c) != m_overriden.end();
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
void Irccd::openConfig()
{
	// Set some defaults
	addPluginPath(Util::configDirectory() + "plugins");		// own directory
	addPluginPath(MODDIR);						// see config.h.in

	// Open requested file by command line or default
	if (!isOverriden(options::Config)) {
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

#if !defined(_WIN32)
	if (!m_foreground) {
		Logger::log("irccd: forking to background...");
		daemon(0, 0);
	}
#endif

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

#if !defined(_WIN32)
	if (general.hasOption("syslog"))
		Logger::setSyslog(general.getOption<bool>("syslog"));
	if (general.hasOption("foreground") && !isOverriden(options::Foreground))
		m_foreground = general.getOption<bool>("foreground");
#endif

	if (general.hasOption("verbose") && !isOverriden(options::Verbose))
		Logger::setVerbose(general.getOption<bool>("verbose"));


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

		Plugin *p = new Plugin(name);
		m_plugins.push_back(shared_ptr<Plugin>(p));		// don't remove that

		m_pluginLock.unlock();

		if (!p->open(finalPath)) {
			Logger::warn("irccd: failed to load module %s: %s",
			    name.c_str(), p->getError().c_str());

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
	vector<shared_ptr<Plugin>>::iterator i;	

	m_pluginLock.lock();
	i = find_if(m_plugins.begin(), m_plugins.end(), [&] (shared_ptr<Plugin> &p) -> bool {
		return p->getName() == name;
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
		findPlugin(name)->onReload();
	} catch (out_of_range ex) {
		Logger::warn("irccd: %s", ex.what());
	}
#else
	Logger::warn("irccd: can't reload plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::openPlugins()
{
	// Get list of modules to load from config
	for (const string &s : m_pluginWanted)
		loadPlugin(s);
}

void Irccd::openIdentities(const Parser &config)
{
	for (Section &s: config.findSections("identity")) {
		Server::Identity identity;

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

			Logger::log("identity: found identity %s (%s, %s, \"%s\")",
			    identity.m_name.c_str(),
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
	if (Util::exist(path) && remove(path.c_str()) < 0) {
		Logger::warn("listener: error removing %s: %s", path.c_str(), strerror(errno));
	} else {
		try {
			unix.create(AF_UNIX);
			unix.bind(UnixPoint(path));
			unix.listen(64);

			// On success add to listener and servers
			m_socketServers.push_back(unix);
			m_listener.add(unix);

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
		try {
			Server::Info info;
			Server::Options options;
			Server::Identity identity;

			// Server information
			info.m_name = s.requireOption<string>("name");
			info.m_host = s.requireOption<string>("host");
			info.m_port = s.requireOption<int>("port");
			if (s.hasOption("ssl"))
				info.m_ssl = s.getOption<bool>("ssl");
			if (s.hasOption("ssl-verify"))
				info.m_sslVerify = s.getOption<bool>("ssl-verify");
			if (s.hasOption("password"))
				info.m_password = s.getOption<string>("password");

			// Identity
			if (s.hasOption("identity"))
				identity = findIdentity(s.getOption<string>("identity"));

			// Some options
			if (s.hasOption("command-char"))
				options.m_commandChar = s.getOption<string>("command-char");
			if (s.hasOption("join-invite"))
				options.m_joinInvite = s.getOption<bool>("join-invite");
			if (s.hasOption("ctcp-autoreply"))
				options.m_ctcpReply = s.getOption<bool>("ctcp-autoreply");

			shared_ptr<Server> server = make_shared<Server>(info, identity, options);

			// Extract channels to auto join
			extractChannels(s, server);
			m_servers.push_back(std::move(server));
		} catch (NotFoundException ex) {
			Logger::warn("server: missing parameter %s", ex.which().c_str());
		}
	}
}

void Irccd::extractChannels(const Section &section, shared_ptr<Server> server)
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

Irccd::Irccd()
	: m_running(true)
	, m_foreground(false)
{
	Socket::init();

	Logger::setVerbose(false);
}

Irccd::~Irccd()
{
	Socket::finish();
}

Irccd * Irccd::getInstance()
{
	if (m_instance == nullptr)
		m_instance = new Irccd();

	return m_instance;
}

void Irccd::override(char c)
{
	m_overriden[c] = true;
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

shared_ptr<Plugin> Irccd::findPlugin(lua_State *state)
{
	for (shared_ptr<Plugin> p : m_plugins)
		if (p->getState().get() == state)
			return p;

	// This one should not happen
	throw out_of_range("plugin not found");
}

shared_ptr<Plugin> Irccd::findPlugin(const string &name)
{
	ostringstream oss;

	for (shared_ptr<Plugin> p : m_plugins)
		if (p->getName() == name)
			return p;

	oss << "plugin " << name << " not found";

	throw out_of_range(oss.str());
}

PluginList & Irccd::getPlugins()
{
	return m_plugins;
}

std::mutex & Irccd::getPluginLock()
{
	return m_pluginLock;
}

void Irccd::addDeferred(shared_ptr<Server> server, DefCall call)
{
	m_deferred[server].push_back(call);
}

#endif
	
ServerList & Irccd::getServers()
{
	return m_servers;
}

void Irccd::setConfigPath(const string &path)
{
	m_configPath = path;
}

void Irccd::setForeground(bool mode)
{
	m_foreground = mode;
}

shared_ptr<Server> & Irccd::findServer(const string &name)
{
	for (shared_ptr<Server> &s : m_servers)
		if (s->getInfo().m_name == name)
			return s;

	throw out_of_range("could not find server with resource " + name);
}

const Server::Identity & Irccd::findIdentity(const string &name)
{
	/*
	 * When name is length 0 that mean user hasn't defined an identity
	 * because it's optional, we don't write an empty message error.
	 */

	if (name.length() == 0)
		return m_defaultIdentity;

	for (const Server::Identity &i : m_identities)
		if (i.m_name == name)
			return i;

	Logger::warn("identity: %s not found", name.c_str());

	return m_defaultIdentity;
}

int Irccd::run()
{
	Logger::log("irccd: user config path %s", Util::configDirectory().c_str());
	Logger::log("irccd: user default plugin path %s", Util::pluginDirectory().c_str());

	openConfig();

	if (m_servers.size() <= 0) {
		Logger::warn("irccd: no server defined, exiting");
		return 1;
	}

	// Start all servers
	for (shared_ptr<Server> &s : m_servers) {
		Logger::log("server %s: trying to connect to %s...",
		    s->getInfo().m_name.c_str(), s->getInfo().m_host.c_str());
		s->startConnection();
	}

	while (m_running) {
		if (m_socketServers.size() == 0) {
			Util::usleep(1000);
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
		} catch (Socket::ErrorException) {
		}
	}

	return 0;
}

void Irccd::stop()
{
	m_running = false;

	for (shared_ptr<Server> &s : m_servers)
		s->stopConnection();

	for (Socket &s : m_socketServers)
		s.close();
}

void Irccd::handleIrcEvent(const IrcEvent &ev)
{
	/*
	 * The following function does not call plugin at all, they just do some
	 * specific action like joining, managing channels, etc.
	 */
	if (ev.m_type == IrcEventType::Connection)
		handleConnection(ev);
	else if (ev.m_type == IrcEventType::Invite)
		handleInvite(ev);
	else if (ev.m_type == IrcEventType::Kick)
		handleKick(ev);

#if defined(WITH_LUA)
	lock_guard<mutex> ulock(m_pluginLock);

	/**
	 * This is the handle of deferred calls, they are not handled in the
	 * next callPlugin block.
	 */
	try {
		if (ev.m_type == IrcEventType::Names ||
		    ev.m_type == IrcEventType::Whois)
			callDeferred(ev);
	} catch (Plugin::ErrorException ex) {
		Logger::warn("plugin %s: %s", ex.which().c_str(), ex.what());
	}

	/**
	 * And this is the block of normal events.
	 */
	for (shared_ptr<Plugin> p : m_plugins) {
		try {
			callPlugin(p, ev);
		} catch (Plugin::ErrorException ex) {
			Logger::warn("plugin %s: %s", p->getName().c_str(), ex.what());
		}
	}
#endif
}

void Irccd::handleConnection(const IrcEvent &event)
{
	shared_ptr<Server> server = event.m_server;

	Logger::log("server %s: successfully connected", server->getInfo().m_name.c_str());

	// Auto join channels
	for (Server::Channel c : server->getChannels()) {
		Logger::log("server %s: autojoining channel %s",
		    server->getInfo().m_name.c_str(), c.m_name.c_str());

		server->join(c.m_name, c.m_password);
	}
}

void Irccd::handleInvite(const IrcEvent &event)
{
	shared_ptr<Server> server = event.m_server;

	// if join-invite is set to true join it
	if (server->getOptions().m_joinInvite)
		server->join(event.m_params[0], "");
}

void Irccd::handleKick(const IrcEvent &event)
{
	shared_ptr<Server> server = event.m_server;

	// If I was kicked, I need to remove the channel list
	if (server->getIdentity().m_nickname == event.m_params[2])
		server->removeChannel(event.m_params[0]);
}

#if defined(WITH_LUA)

void Irccd::callPlugin(shared_ptr<Plugin> p, const IrcEvent &ev)
{
	switch (ev.m_type) {
	case IrcEventType::Connection:
		p->onConnect(ev.m_server);
		break;
	case IrcEventType::ChannelNotice:
		p->onNotice(ev.m_server, ev.m_params[0], ev.m_params[1],
			   ev.m_params[2]);
		break;
	case IrcEventType::Invite:
		p->onInvite(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Join:
		p->onJoin(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Kick:
		p->onKick(ev.m_server, ev.m_params[0], ev.m_params[1],
			 ev.m_params[2], ev.m_params[3]);
		break;
	case IrcEventType::Message:
	{
		string cc = ev.m_server->getOptions().m_commandChar;
		string sp = cc + p->getName();
		string msg = ev.m_params[2];

		// handle special commands "!<plugin> command"
		if (cc.length() > 0 && msg.compare(0, sp.length(), sp) == 0) {
			string plugin = msg.substr(
			    cc.length(), sp.length() - cc.length());

			if (plugin == p->getName()) {
				p->onCommand(ev.m_server,
						ev.m_params[0],
						ev.m_params[1],
						msg.substr(sp.length())
				);
			}
		} else
			p->onMessage(ev.m_server, ev.m_params[0], ev.m_params[1],
				    ev.m_params[2]);
	}
		break;
	case IrcEventType::Mode:
		p->onMode(ev.m_server, ev.m_params[0], ev.m_params[1],
			 ev.m_params[2], ev.m_params[3]);
		break;
	case IrcEventType::Nick:
		p->onNick(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	case IrcEventType::Notice:
		p->onNotice(ev.m_server, ev.m_params[0], ev.m_params[1],
			   ev.m_params[2]);
		break;
	case IrcEventType::Part:
		p->onPart(ev.m_server, ev.m_params[0], ev.m_params[1],
			 ev.m_params[2]);
		break;
	case IrcEventType::Query:
		p->onQuery(ev.m_server, ev.m_params[0], ev.m_params[1]);
		break;
	default:
		break;
	}
}

void Irccd::callDeferred(const IrcEvent &ev)
{
	// Check if we have deferred call for this server
	if (m_deferred.find(ev.m_server) == m_deferred.end())
		return;

	vector<DefCall>::iterator it = m_deferred[ev.m_server].begin();

	for (; it != m_deferred[ev.m_server].end(); ) {
		bool deleteIt = true;

		if (ev.m_type == it->type()) {
			switch (it->type()) {
			case IrcEventType::Names:
				it->onNames(ev.m_params);
				break;
			case IrcEventType::Whois:
				it->onWhois(ev.m_params);
				break;
			default:
				deleteIt = false;
				break;
			}
		} else
			deleteIt = false;

		/*
		 * If found and executed, break the loop if not, we will
		 * call multiple times for the same event.
		 */
		if (deleteIt) {
			it = m_deferred[ev.m_server].erase(it);
			break;
		} else
			++it;
	}
}

#endif
