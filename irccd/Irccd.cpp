/*
 * Irccd.cpp -- main irccd class
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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
#include <cstddef>
#include <cstring>
#include <functional>
#include <map>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include <Logger.h>
#include <Parser.h>
#include <Util.h>

#include "Irccd.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

namespace irccd {

Irccd Irccd::m_instance;

/* {{{ Client handlers */

namespace {

using SocketFunction = std::function<void(const std::vector<std::string> &params)>;

/**
 * @struct ClientHandler
 * @brief Irccdctl function
 *
 * This describe a function to be called. It requires a number of arguments,
 * how many to split and a function to call.
 */
struct ClientHandler {
	int		m_noargs;
	int		m_nosplit;
	SocketFunction	m_function;	

	ClientHandler()
	{
	}

	ClientHandler(int noargs, int nosplit, SocketFunction function)
		: m_noargs(noargs)
		, m_nosplit(nosplit)
		, m_function(function)
	{
	}
};

void handleChannelNotice(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->cnotice(params[1], params[2]);
}

void handleInvite(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->invite(params[1], params[2]);
}

void handleJoin(const std::vector<std::string> &params)
{
	std::string password;
	if (params.size() == 3)
		password = params[2];

	Irccd::getInstance().findServer(params[0])->join(params[1], password);
}

void handleKick(const std::vector<std::string> &params)
{
	std::string reason;
	if (params.size() == 4)
		reason = params[3];

	Irccd::getInstance().findServer(params[0])->kick(params[1], params[2], reason);
}

void handleLoad(const std::vector<std::string> &params)
{
	Irccd::getInstance().loadPlugin(params[0]);
}

void handleMe(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->me(params[1], params[2]);
}

void handleMessage(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->say(params[1], params[2]);
}

void handleMode(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->mode(params[1], params[2]);
}

void handleNick(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->nick(params[1]);
}

void handleNotice(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->notice(params[1], params[2]);
}

void handlePart(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->part(params[1]);
}

void handleReload(const std::vector<std::string> &params)
{
	Irccd::getInstance().reloadPlugin(params[0]);
}

void handleTopic(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->topic(params[1], params[2]);
}

void handleUnload(const std::vector<std::string> &params)
{
	Irccd::getInstance().unloadPlugin(params[0]);
}

void handleUserMode(const std::vector<std::string> &params)
{
	Irccd::getInstance().findServer(params[0])->umode(params[1]);
}

std::unordered_map<std::string, ClientHandler> handlers {
	{ "CNOTICE",	ClientHandler(3, 3, handleChannelNotice)	},
	{ "INVITE",	ClientHandler(3, 3, handleInvite)		},
	{ "JOIN",	ClientHandler(2, 3, handleJoin)			},
	{ "KICK",	ClientHandler(3, 4, handleKick)			},
	{ "LOAD",	ClientHandler(1, 1, handleLoad)			},
	{ "ME",		ClientHandler(3, 3, handleMe)			},
	{ "MSG",	ClientHandler(3, 3, handleMessage)		},
	{ "MODE",	ClientHandler(3, 3, handleMode)			},
	{ "NICK",	ClientHandler(2, 2, handleNick)			},
	{ "NOTICE",	ClientHandler(3, 3, handleNotice)		},
	{ "PART",	ClientHandler(2, 2, handlePart)			},
	{ "RELOAD",	ClientHandler(1, 1, handleReload)		},
	{ "TOPIC",	ClientHandler(3, 3, handleTopic)		},
	{ "UMODE",	ClientHandler(2, 2, handleUserMode)		},
	{ "UNLOAD",	ClientHandler(1, 1, handleUnload)		}
};

}

/* }}} */

/* {{{ Private miscellaneous methods */

Irccd::Irccd()
	: m_running(true)
	, m_foreground(false)
{
	Socket::init();

	Logger::setVerbose(false);
}

bool Irccd::isOverriden(char c)
{
	return m_overriden.find(c) != m_overriden.end();
}

/* }}} */

/* {{{ Private socket management */

void Irccd::clientAdd(Socket &server)
{
	Socket client;

	try {
		Socket client = server.accept();

		// Add to clients to read data
		m_streamClients[client] = Message();
		m_listener.add(client);
	} catch (SocketError ex) {
		Logger::warn("listener: could not accept client: %s", ex.what());
	}
}

void Irccd::clientRead(Socket &client)
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
		if (length == 0)
			removeIt = true;
		else {
			std::string ret;

			data[length] = '\0';

			if (m_streamClients[client].isFinished(data, ret))
				execute(ret, client);
		}
	} catch (SocketError ex) {
		Logger::warn("listener: Could not read from client %s", ex.what());
		removeIt = true;
	}

	if (removeIt) {
		m_streamClients.erase(client);
		m_listener.remove(client);
	}
}

void Irccd::peerRead(Socket &s)
{
	SocketAddress addr;
	char data[128 + 1];
	int length;

	try {
		std::string ret;

		length = s.recvfrom(data, sizeof (data) - 1, addr);
		data[length] = '\0';

		// If no client, create first
		if (m_dgramClients.find(addr) == m_dgramClients.end())
			m_dgramClients[addr] = Message();

		if (m_dgramClients[addr].isFinished(data, ret)) {
			execute(ret, s, addr);

			// Clear the message buffer
			m_dgramClients[addr] = Message();
		}
	} catch (SocketError ex) {
		Logger::warn("listener: could not read %s", ex.what());
	}
}

void Irccd::execute(const std::string &cmd,
		    Socket &s,
		    const SocketAddress &addr)
{
	std::string cmdName;
	size_t cmdDelim;

	cmdDelim = cmd.find_first_of(" \t");
	if (cmdDelim != std::string::npos) {
		cmdName = cmd.substr(0, cmdDelim);
		if (handlers.find(cmdName) == handlers.end())
			Logger::warn("listener: invalid command %s", cmdName.c_str());
		else {
			auto h = handlers[cmdName];

			try {
				std::string lineArgs = cmd.substr(cmdDelim + 1);
				std::vector<std::string> params = Util::split(lineArgs, " \t", h.m_nosplit);

				/*
				 * Check the number of args needed.
				 */
				if (params.size() < static_cast<size_t>(h.m_noargs)) {
					std::ostringstream oss;
					std::string error;

					oss << cmdName << " requires at least ";
					oss << handlers[cmdName].m_noargs << "\n";
					error = oss.str();

					s.send(error);
				} else {
					/*
					 * Send a response "OK\n" to notify irccdctl.
					 */
					h.m_function(params);
					notifySocket("OK\n", s, addr);
				}
			} catch (std::out_of_range ex) {
				std::ostringstream oss;

				oss << ex.what() << "\n";

				notifySocket(oss.str(), s, addr);
			} catch (SocketError ex) {
				Logger::warn("listener: failed to send: %s", ex.what());
			}
		}
	}
}

void Irccd::notifySocket(const std::string &message,
			 Socket &s,
			 const SocketAddress &addr)
{
	if (s.getType() == SOCK_STREAM)
		s.send(message.c_str(), message.length());
	else
		s.sendto(message.c_str(), message.length(), addr);
}

/* }}} */

/* {{{ Private plugin management */

void Irccd::loadWantedPlugins()
{
	std::ostringstream oss;

	// Add user's path
	oss << Util::pathUser() << "plugins/";
	addPluginPath(oss.str());

	// Add system's path
	oss.str("");
	if (!Util::isAbsolute(MODDIR))
		oss << Util::pathBase();

	oss << MODDIR << Util::DIR_SEP;
	addPluginPath(oss.str());

	// Get list of modules to load from config
	for (const std::string &s : m_pluginWanted)
		loadPlugin(s);
}

bool Irccd::isPluginLoaded(const std::string &name)
{
#if defined(WITH_LUA)
	bool ret = true;

	try {
		(void)findPlugin(name);
	} catch (std::out_of_range ex) {
		ret = false;
	}

	return ret;
#else
	(void)name;

	return false;
#endif
}

/* }}} */

/* {{{ Private open functions (configuration) */

/*
 * Order is:
 * 1. Option -c passed to the command line
 * 2. User defined, usually ~/.config/irccd/irccd.conf
 * 3. Default cmake configured path, usually /usr/local/etc/irccd.conf
 */
void Irccd::openConfig()
{
	Parser config;

	// Open requested file by command line or default
	if (!isOverriden(Options::Config)) {
		try {
			m_configPath = Util::findConfiguration("irccd.conf");
			config = Parser(m_configPath);
		} catch (Util::ErrorException ex) {
			Logger::fatal(1, "%s: %s", getprogname(), ex.what());
		}
	} else
		config = Parser(m_configPath);

	if (!config.open())
		Logger::fatal(1, "irccd: could not open %s, exiting", m_configPath.c_str());

	Logger::log("irccd: using configuration %s", m_configPath.c_str());

#if !defined(_WIN32)
	if (!m_foreground) {
		Logger::log("irccd: forking to background...");
		daemon(0, 0);
	}
#endif

	/*
	 * Order is important, load everything we can before plugins so that
	 * they can use identities, configuration and such, but servers
	 * requires plugin to be loaded.
	 */
	readGeneral(config);
	readIdentities(config);
	readListeners(config);
	readPlugins(config);
	readServers(config);
}

void Irccd::readGeneral(const Parser &config)
{
	if (config.hasSection("general")) {
		Section general = config.getSection("general");

		// Extract parameters that are needed for the next
		if (general.hasOption("plugin-path"))
			addPluginPath(general.getOption<std::string>("plugin-path"));

		// Old way of loading plugins
		if (general.hasOption("plugins")) {
			Logger::warn("irccd: general.plugins option is deprecated, use [plugins]");

			std::string list = general.getOption<std::string>("plugins");
			for (auto s : Util::split(list, " \t"))
				addWantedPlugin(s);
		}

#if !defined(_WIN32)
		if (general.hasOption("syslog"))
			Logger::setSyslog(general.getOption<bool>("syslog"));
		if (general.hasOption("foreground") && !isOverriden(Options::Foreground))
			m_foreground = general.getOption<bool>("foreground");
		if (general.hasOption("verbose") && !isOverriden(Options::Verbose))
			Logger::setVerbose(general.getOption<bool>("verbose"));
#endif
	}
}

void Irccd::readPlugins(const Parser &config)
{
	// New way of loading plugins
	if (config.hasSection("plugins")) {
		Section plugins = config.getSection("plugins");

		for (auto opt : plugins.getOptions()) {
			if (opt.m_value.length() == 0)
				addWantedPlugin(opt.m_key);
			else
				addWantedPlugin(opt.m_value, true);
		}
	}

	loadWantedPlugins();
}

void Irccd::readIdentities(const Parser &config)
{
	config.findSections("identity", [&] (const Section &s) {
		Server::Identity identity;

		try {
			identity.m_name = s.requireOption<std::string>("name");

			if (s.hasOption("nickname"))
				identity.m_nickname = s.getOption<std::string>("nickname");
			if (s.hasOption("username"))
				identity.m_username = s.getOption<std::string>("username");
			if (s.hasOption("realname"))
				identity.m_realname = s.getOption<std::string>("realname");
			if (s.hasOption("ctcp-version"))
				identity.m_ctcpVersion = s.getOption<std::string>("ctcp-version");
			if (s.hasOption("ctcp-autoreply"))
				identity.m_ctcpReply = s.getOption<bool>("ctcp-autoreply");

			Logger::log("identity: found identity %s (%s, %s, \"%s\")",
			    identity.m_name.c_str(),
			    identity.m_nickname.c_str(), identity.m_username.c_str(),
			    identity.m_realname.c_str());

			m_identities.push_back(identity);
		} catch (NotFoundException ex) {
			Logger::log("identity: missing parameter %s", ex.which().c_str());
		}
	});
}

void Irccd::readListeners(const Parser &config)
{
	config.findSections("listener", [&] (const Section &s) {
		try {
			std::string type;
			std::string proto = "tcp";

			type = s.requireOption<std::string>("type");

			// Protocol is TCP by default
			if (s.hasOption("protocol"))
				proto = s.getOption<std::string>("protocol");
	
			if (proto != "tcp" && proto != "udp") {
				Logger::warn("listener: protocol not valid, must be tcp or udp");
				return;
			}

			if (type == "internet")
				extractInternet(s, proto == "tcp" ? SOCK_STREAM : SOCK_DGRAM);
			else if (type == "unix") {
#if !defined(_WIN32)
				extractUnix(s, proto == "tcp" ? SOCK_STREAM : SOCK_DGRAM);
#else
				Logger::warn("listener: unix sockets are not supported on Windows");
#endif
			} else
				Logger::warn("listener: unknown listener type `%s'", type.c_str());
		} catch (NotFoundException ex) {
			Logger::warn("listener: missing parameter %s", ex.which().c_str());
		}
	});
}

void Irccd::extractInternet(const Section &s, int type)
{
	std::vector<std::string> protocols;
	std::string address, family;
	int port;
	bool ipv4 = false, ipv6 = false;

	address = s.getOption<std::string>("address");
	family = s.getOption<std::string>("family");
	port = s.getOption<int>("port");

	// extract list of family
	protocols = Util::split(family, " \t");

	for (auto p : protocols) {
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

		Socket inet((ipv6) ? AF_INET6 : AF_INET, type, 0);

		inet.set(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));
		if (ipv6) {
			int mode = !ipv4;
			inet.set(IPPROTO_IPV6, IPV6_V6ONLY, &mode, sizeof (mode));
		}

		inet.bind(BindAddressIP(address, port, (ipv6) ? AF_INET6 : AF_INET));

		if (type == SOCK_STREAM)
			inet.listen(64);

		// On success add to listener and servers
		m_socketServers.push_back(inet);
		m_listener.add(inet);

		Logger::log("listener: listening for clients on port %d...", port);
	} catch (SocketError ex) {
		Logger::warn("listener: internet socket error: %s", ex.what());
	}
}

#if !defined(_WIN32)
void Irccd::extractUnix(const Section &s, int type)
{
	std::string path;

	path = s.requireOption<std::string>("path");

	// First remove the dust
	if (Util::exist(path) && remove(path.c_str()) < 0) {
		Logger::warn("listener: error removing %s: %s", path.c_str(), strerror(errno));
	} else {
		try {
			Socket unix(AF_UNIX, type, 0);

			unix.bind(AddressUnix(path, true));

			if (type == SOCK_STREAM)
				unix.listen(64);

			// On success add to listener and servers
			m_socketServers.push_back(unix);
			m_listener.add(unix);

			Logger::log("listener: listening for clients on %s...", path.c_str());
		} catch (SocketError ex) {
			Logger::warn("listener: unix socket error: %s", ex.what());
		}
	}
}
#endif

void Irccd::readServers(const Parser &config)
{
	config.findSections("server", [&]  (const Section &s) {
		try {
			Server::Info info;
			Server::Options options;
			Server::Identity identity;

			// Server information
			info.m_name = s.requireOption<std::string>("name");
			info.m_host = s.requireOption<std::string>("host");
			info.m_port = s.requireOption<int>("port");
			if (s.hasOption("ssl"))
				info.m_ssl = s.getOption<bool>("ssl");
			if (s.hasOption("ssl-verify"))
				info.m_sslVerify = s.getOption<bool>("ssl-verify");
			if (s.hasOption("password"))
				info.m_password = s.getOption<std::string>("password");

			// Identity
			if (s.hasOption("identity"))
				identity = findIdentity(s.getOption<std::string>("identity"));

			// Some options
			if (s.hasOption("command-char"))
				options.m_commandChar = s.getOption<std::string>("command-char");
			if (s.hasOption("join-invite"))
				options.m_joinInvite = s.getOption<bool>("join-invite");

			// Reconnection settings
			if (s.hasOption("reconnect"))
				options.m_retry = s.getOption<bool>("reconnect");
			if (s.hasOption("reconnect-tries"))
				options.m_maxretries = s.getOption<int>("reconnect-tries");
			if (s.hasOption("reconnect-timeout"))
				options.m_timeout = s.getOption<int>("reconnect-timeout");

			Server::Ptr server = std::make_shared<Server>(info, identity, options);

			// Extract channels to auto join
			extractChannels(s, server);
			m_servers.push_back(std::move(server));
		} catch (NotFoundException ex) {
			Logger::warn("server: missing parameter %s", ex.which().c_str());
		}
	});
}

void Irccd::extractChannels(const Section &section, Server::Ptr server)
{
	std::vector<std::string> channels;
	std::string list, name, password;
	size_t colon;

	if (section.hasOption("channels")) {
		list = section.getOption<std::string>("channels");
		channels = Util::split(list, " \t");

		for (const std::string &s : channels) {
			// detect an optional channel password
			colon = s.find_first_of(':');
			if (colon != std::string::npos) {
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

/* }}} */

/* {{{ Private server management */

void Irccd::handleConnection(const IrcEvent &event)
{
	Server::Ptr server = event.m_server;

	Logger::log("server %s: successfully connected",
	    server->getInfo().m_name.c_str());

	// Auto join channels
	for (auto c : server->getChannels()) {
		Logger::log("server %s: autojoining channel %s",
		    server->getInfo().m_name.c_str(), c.m_name.c_str());

		server->join(c.m_name, c.m_password);
	}
}

void Irccd::handleInvite(const IrcEvent &event)
{
	Server::Ptr server = event.m_server;

	// if join-invite is set to true join it
	if (server->getOptions().m_joinInvite)
		server->join(event.m_params[0], "");
}

void Irccd::handleKick(const IrcEvent &event)
{
	Server::Ptr server = event.m_server;

	// If I was kicked, I need to remove the channel list
	if (server->getIdentity().m_nickname == event.m_params[2])
		server->removeChannel(event.m_params[0]);
}

#if defined(WITH_LUA)

void Irccd::callPlugin(Plugin::Ptr p, const IrcEvent &ev)
{
	switch (ev.m_type) {
	case IrcEventType::Connection:
		p->onConnect(ev.m_server);
		break;
	case IrcEventType::ChannelNotice:
		p->onChannelNotice(ev.m_server, ev.m_params[0], ev.m_params[1],
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
		std::string cc = ev.m_server->getOptions().m_commandChar;
		std::string sp = cc + p->getName();
		std::string msg = ev.m_params[2];

		// handle special commands "!<plugin> command"
		if (cc.length() > 0 && msg.compare(0, sp.length(), sp) == 0) {
			std::string plugin = msg.substr(
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
	case IrcEventType::Me:
		p->onMe(ev.m_server, ev.m_params[1], ev.m_params[0], ev.m_params[2]);
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
	case IrcEventType::Topic:
		p->onTopic(ev.m_server, ev.m_params[0], ev.m_params[1],
		     ev.m_params[2]);
		break;
	case IrcEventType::UserMode:
		p->onUserMode(ev.m_server, ev.m_params[0], ev.m_params[1]);
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

	std::vector<DefCall>::iterator it = m_deferred[ev.m_server].begin();

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

/* }}} */

/* {{{ Public constructor, destructor and miscellaneous methods */

Irccd::~Irccd()
{
	Socket::finish();
}

Irccd &Irccd::getInstance()
{
	return m_instance;
}

void Irccd::override(char c)
{
	m_overriden[c] = true;
}

void Irccd::setConfigPath(const std::string &path)
{
	m_configPath = path;
}

void Irccd::setForeground(bool mode)
{
	m_foreground = mode;
}

const Server::Identity &Irccd::findIdentity(const std::string &name)
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

/* }}} */

/* {{{ Public Plugin management */

void Irccd::addPluginPath(const std::string &path)
{
	m_pluginDirs.push_back(path);
}

void Irccd::addWantedPlugin(const std::string &name, bool specified)
{
	m_pluginWanted.push_back(name);

	if (specified)
		m_pluginSpecified[name] = true;
}

void Irccd::loadPlugin(const std::string &name)
{
#if defined(WITH_LUA)
	std::ostringstream oss;
	std::string realname, realpath;
	bool found = false;

	if (isPluginLoaded(name))
		return;

	/*
	 * If the plugin has been specified by path using foo = /path then
	 * it should contains the .lua extension, otherwise we search
	 * for it.
	 */
	if (m_pluginSpecified.count(name) >= 1) {
		Logger::log("irccd: checking for plugin %s", name.c_str());
		found = Util::exist(name);

		/*
		 * Compute the name by removing .lua extension and optional
		 * directory.
		 */
		realpath = name;
		realname = Util::baseName(realpath);

		auto pos = realname.find(".lua");
		if (pos != std::string::npos)
			realname.erase(pos);
	} else {
		realname = name;

		// Seek the plugin in the directories.
		for (auto p : m_pluginDirs) {
			oss.str("");
			oss << p;

			// Add a / or \\ only if needed
			if (p.length() > 0 && p[p.length() - 1] != Util::DIR_SEP)
				oss << Util::DIR_SEP;

			oss << name << ".lua";

			realpath = oss.str();
			Logger::log("irccd: checking for plugin %s", realpath.c_str());

			if (Util::exist(realpath)) {
				found = true;
				break;
			}
		}
	}

	if (!found)
		Logger::warn("irccd: plugin %s not found", realname.c_str());
	else {
		/*
		 * At this step, the open function will open the lua
		 * script, that script may want to call some bindings
		 * directly so we need to add it to the registered
		 * Lua plugins even if it has failed. So we remove
		 * it only on failure and expect plugins to be well
		 * coded.
		 */
		m_pluginLock.lock();
		Plugin *p = new Plugin(realname, realpath);

		m_pluginMap[p->getState()] = Plugin::Ptr(p);
		m_pluginLock.unlock();

		if (!p->open()) {
			Logger::warn("irccd: failed to load plugin %s: %s",
			    realname.c_str(), p->getError().c_str());

			m_pluginLock.lock();
			m_pluginMap.erase(p->getState());
			m_pluginLock.unlock();
		}
	}
#else
	Logger::warn("irccd: can't load plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::unloadPlugin(const std::string &name)
{
#if defined(WITH_LUA)
	m_pluginLock.lock();

	try {
		auto i = findPlugin(name);

		try {
			i->onUnload();
		} catch (Plugin::ErrorException ex) {
			Logger::warn("irccd: error while unloading %s: %s",
			    name.c_str(), ex.what());
		}

		m_pluginMap.erase(i->getState());
	} catch (std::out_of_range) {
		Logger::warn("irccd: there is no plugin %s loaded", name.c_str());
	}

	m_pluginLock.unlock();
#else
	Logger::warn("irccd: can't unload plugin %s, Lua support disabled", name.c_str());
#endif
}

void Irccd::reloadPlugin(const std::string &name)
{
#if defined(WITH_LUA)
	try {
		findPlugin(name)->onReload();
	} catch (std::out_of_range ex) {
		Logger::warn("irccd: %s", ex.what());
	}
#else
	Logger::warn("irccd: can't reload plugin %s, Lua support disabled", name.c_str());
#endif
}

#if defined(WITH_LUA)

Plugin::Ptr Irccd::findPlugin(lua_State *state)
{
	for (auto plugin : m_pluginMap) {
		/*
		 * Test if the current plugin is the one with that
		 * Lua state.
		 */
		if (plugin.first == state)
			return plugin.second;
	}

	for (auto plugin : m_threadMap) {
		if (plugin.first == state)
			return plugin.second;
	}

	throw std::out_of_range("plugin not found");
}

Plugin::Ptr Irccd::findPlugin(const std::string &name)
{
	using type = std::pair<lua_State *, Plugin::Ptr>;

	std::ostringstream oss;

	auto i = std::find_if(m_pluginMap.begin(), m_pluginMap.end(),
	    [&] (type p) -> bool {
		return p.second->getName() == name;
	    }
	);

	if (i == m_pluginMap.end()) {
		oss << "plugin " << name << " not found";

		throw std::out_of_range(oss.str());
	}

	return (*i).second;
}

void Irccd::addDeferred(Server::Ptr server, DefCall call)
{
	m_deferred[server].push_back(call);
}

void Irccd::registerThread(lua_State *L, Plugin::Ptr plugin)
{
	Lock lk(m_pluginLock);

	m_threadMap[L] = plugin;
}

void Irccd::unregisterThread(lua_State *L)
{
	Lock lk(m_pluginLock);
	
	m_threadMap.erase(L);
}

#endif

/* }}} */
	
/* {{{ Public Server management */

ServerList &Irccd::getServers()
{
	return m_servers;
}

Server::Ptr Irccd::findServer(const std::string &name)
{
	for (auto s : m_servers)
		if (s->getInfo().m_name == name)
			return s;

	throw std::out_of_range("could not find server with resource " + name);
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
	Lock lk(m_pluginLock);

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

	for (auto p : m_pluginMap) {
		/*
		 * Ignore Lua threads that share the same Plugin
		 * object.
		 */
		if (m_threadMap.find(p.first) != m_threadMap.end())
			continue;

		try {
			callPlugin(p.second, ev);
		} catch (Plugin::ErrorException ex) {
			Logger::warn("plugin %s: %s",
			    p.second->getName().c_str(), ex.what());
		}
	}
#endif
}

/* }}} */

/* {{{ Irccd management */

int Irccd::run()
{
	openConfig();

	if (m_servers.size() <= 0) {
		Logger::warn("irccd: no server defined, exiting");
		return 1;
	}

	// Start all servers
	for (auto s : m_servers) {
		Logger::log("server %s: trying to connect to %s...",
		    s->getInfo().m_name.c_str(), s->getInfo().m_host.c_str());
		s->startConnection();
	}

	while (m_running) {
		/*
		 * If no listeners is enabled, we must wait a bit to avoid
		 * CPU usage exhaustion. But we only wait for 250 millisecond
		 * because some plugins are time specific and requires
		 * precision (i.e badwords).
		 */
		if (m_socketServers.size() == 0) {
			Util::usleep(250);
			continue;
		}

		try {
			Socket s = m_listener.select(0);

			/*
			 * For stream based server add a client and wait for its data,
			 * otherwise, read the UDP socket and try to execute it.
			 */
			if (s.getType() == SOCK_STREAM) {
				if (find(m_socketServers.begin(), m_socketServers.end(), s) != m_socketServers.end())
					clientAdd(s);
				else
					clientRead(s);
			} else
				peerRead(s);
		} catch (SocketError er) {
			Logger::warn("listener: socket error %s", er.what());
		}
	}

	return 0;
}

void Irccd::stop()
{
	m_running = false;

	for (auto s : m_servers)
		s->stopConnection();

	for (auto s : m_socketServers)
		s.close();
}

/* }}} */

} // !irccd

#endif
