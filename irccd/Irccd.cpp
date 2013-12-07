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
#include <Socket.h>
#include <SocketAddress.h>
#include <Util.h>

#include "Irccd.h"
#include "Listener.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

namespace irccd {

Irccd Irccd::m_instance;

Irccd::Irccd()
	: m_running(true)
	, m_foreground(false)
{
}

void Irccd::initialize()
{
	std::ostringstream oss;

	Socket::init();
	Logger::setVerbose(false);

	// Add user's path
	oss << Util::pathUser() << "plugins/";
	Plugin::addPath(oss.str());

	// Add system's path
	oss.str("");
	if (!Util::isAbsolute(MODDIR))
		oss << Util::pathBase();

	oss << MODDIR << Util::DIR_SEP;
	Plugin::addPath(oss.str());
}

bool Irccd::isOverriden(char c)
{
	return m_overriden.find(c) != m_overriden.end();
}

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

	/* Now, we load plugins specified by command line */
	for (auto s : m_wantedPlugins) {
		try {
			Plugin::load(s);
		} catch (std::runtime_error error) {
			Logger::warn("irccd: %s", error.what());
		}
	}

	readServers(config);
}

void Irccd::readGeneral(const Parser &config)
{
	if (config.hasSection("general")) {
		Section general = config.getSection("general");

		// Extract parameters that are needed for the next
		if (general.hasOption("plugin-path"))
			Plugin::addPath(general.getOption<std::string>("plugin-path"));

#if !defined(WITH_NOCOMPAT)
		// Old way of loading plugins
		if (general.hasOption("plugins")) {
			Logger::warn("irccd: general.plugins option is deprecated, use [plugins]");

			std::string list = general.getOption<std::string>("plugins");
			for (auto s : Util::split(list, " \t")) {
				try {
					Plugin::load(s);
				} catch (std::runtime_error error) {
					Logger::warn("irccd: %s", error.what());
				}
			}
		}
#endif

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
		Section section = config.getSection("plugins");

		for (auto opt : section.getOptions()) {
			if (opt.m_value.length() == 0)
				Plugin::load(opt.m_key);
			else
				Plugin::load(opt.m_value, false);
		}
	}
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

		Listener::add(inet);
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

			Listener::add(unix);
			Logger::log("listener: listening for clients on %s...", path.c_str());
		} catch (SocketError ex) {
			Logger::warn("listener: unix socket error: %s", ex.what());
		}
	}
}
#endif

void Irccd::readServers(const Parser &config)
{
	config.findSections("server", [&] (const Section &s) {
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
			if (Server::has(info.m_name))
				Logger::warn("server %s: duplicated server", info.m_name.c_str());
			else
				Server::add(server);
		} catch (NotFoundException ex) {
			Logger::warn("server: missing parameter %s", ex.which().c_str());
		}
	});
}

void Irccd::extractChannels(const Section &section, Server::Ptr server)
{
	std::vector<std::string> channels;
	std::string list, name, password;

	if (section.hasOption("channels")) {
		list = section.getOption<std::string>("channels");
		channels = Util::split(list, " \t");

		for (const std::string &s : channels)
			server->addChannel(Server::toChannel(s));
	}
}

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

void Irccd::deferPlugin(const std::string &name)
{
	m_wantedPlugins.push_back(name);
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

int Irccd::run()
{
	openConfig();

	while (m_running) {
		/*
		 * If no listeners is enabled, we must wait a bit to avoid
		 * CPU usage exhaustion. But we only wait for 250 millisecond
		 * because some plugins are time specific and requires
		 * precision (i.e badwords).
		 */
		if (Listener::count() == 0)
			Util::usleep(250);
		else
			Listener::process();

		if (m_running)
			Server::flush();
	}

	stop();

	return 0;
}

bool Irccd::isRunning() const
{
	return m_running;
}

void Irccd::shutdown()
{
	m_running = false;
}

void Irccd::stop()
{
	Server::forAll([] (Server::Ptr s) {
		s->stop();
	});

	Listener::close();
}

} // !irccd
