/*
 * Irccd.cpp -- main irccd class
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

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include <Filesystem.h>
#include <Logger.h>
#include <Util.h>

#include "Irccd.h"

using namespace std::string_literals;

namespace irccd {

Irccd::Irccd()
{
	/*
	 * This signal is called from the ServerManager.
	 */
	m_serverService.onEvent.connect([this] (ServerEvent event) {
		serverAddEvent(std::move(event));
	});
	m_transportService.onCommand.connect([this] (TransportCommand command) {
		transportAddCommand(std::move(command));
	});
}

Irccd::~Irccd()
{
	Logger::debug() << "irccd: waiting for transport to finish..." << std::endl;
	m_transportService.stop();
	Logger::debug() << "irccd: waiting for server to finish..." << std::endl;
	m_serverService.stop();
}

#if defined(WITH_JS)

void Irccd::pluginLoad(std::string path)
{
	std::shared_ptr<Plugin> plugin;
	std::string name;

	if (Filesystem::isRelative(path)) {
		name = path;

		/*
		 * Iterate over all plugin directories and try to load, we emit
		 * a warning for all failures but do not break unless we have
		 * found a plugin working.
		 */
		for (const std::string &dir : Util::pathsPlugins()) {
			std::string fullpath = dir + Filesystem::Separator + path + ".js";

			try {
				Logger::info() << "plugin " << name << ": trying " << fullpath << std::endl;

				plugin = std::make_shared<Plugin>(path, fullpath, m_pluginConf[name]);
				break;
			} catch (const std::exception &ex) {
				Logger::info() << "plugin " << name << ": " << fullpath << ": " << ex.what() << std::endl;
			}
		}
	} else {
		/* Extract name from the path */
		std::string::size_type first = path.rfind(Filesystem::Separator);
		std::string::size_type last = path.rfind(".js");

		if (first == std::string::npos || last == std::string::npos) {
			throw std::invalid_argument("unable to extract plugin name");
		}

		name = path.substr(first, last);

		Logger::info() << "plugin " << name << ": trying " << path << std::endl;

		try {
			plugin = std::make_shared<Plugin>(std::move(name), std::move(path), m_pluginConf[name]);
		} catch (const std::exception &ex) {
			Logger::info() << "plugin " << name << ": error: " << ex.what() << std::endl;
		}
	}

	if (!plugin) {
		Logger::warning() << "plugin " << name << ": unable to find suitable plugin" << std::endl;
		return;
	}

	/*
	 * These signals will be called from the Timer thread.
	 */
	plugin->setOnTimerSignal([this, plugin] (std::shared_ptr<Timer> timer) {
		timerAddEvent(TimerEvent(std::move(plugin), std::move(timer)));
	});
	plugin->setOnTimerEnd([this, plugin] (std::shared_ptr<Timer> timer) {
		timerAddEvent(TimerEvent(std::move(plugin), std::move(timer), TimerEventType::End));
	});
	plugin->onLoad();

	m_plugins.emplace(plugin->info().name, std::move(plugin));
}

void Irccd::pluginUnload(const std::string &name)
{
	std::shared_ptr<Plugin> plugin = pluginFind(name);

	plugin->onUnload();

	/*
	 * Erase any element in the timer event queue that match this plugin
	 * to avoid calling the event of a plugin that is not in irccd anymore.
	 */
	for (auto it = m_timerEvents.begin(); it != m_timerEvents.end(); ) {
		if (it->plugin() == plugin) {
			it = m_timerEvents.erase(it);
		} else {
			++ it;
		}
	}

	m_plugins.erase(plugin->info().name);
}

#endif

void Irccd::run()
{
	m_serverService.start();
	m_transportService.start();

	while (m_running) {
		// Wait
		std::unique_lock<std::mutex> lock(m_mutex);

		m_condition.wait(lock, [this] () {
			if (!m_running || m_serverEvents.size() > 0 || m_transportCommands.size() > 0) {
				return true;
			}
#if defined(WITH_JS)
			if (m_timerEvents.size() > 0) {
				return true;
			}
#endif
			return false;
		});

		if (!m_running) {
			continue;
		}

		/* Call transport commands */
		while (!m_transportCommands.empty()) {
			m_transportCommands.front().exec(*this);
			m_transportCommands.pop();
		}

		/* Call server events */
		while (!m_serverEvents.empty()) {
			/* Broadcast */
			m_transportService.broadcast(m_serverEvents.front().toJson());

#if defined(WITH_JS)
			/*
			 * Make a copy of the list of plugins because like
			 * timers, a callback may remove a plugin from the list
			 * while we are iterating.
			 */
			std::map<std::string, std::shared_ptr<Plugin>> plugins = m_plugins;
			for (auto &pair : plugins) {
				/* Only call if our plugin is still alive */
				if (m_plugins.count(pair.first) > 0) {
					m_serverEvents.front().call(*pair.second);
				}
			}
#endif
			m_serverEvents.pop();
		}


#if defined(WITH_JS)
		/*
		 * Don't use a for-range based loop because the timer event may
		 * modify this queue if the script request to unload a plugin.
		 *
		 * See pluginUnload function.
		 */
		while (!m_timerEvents.empty()) {
			m_timerEvents.front().call();
			m_timerEvents.pop_front();
		}
#endif
	}
}

void Irccd::stop()
{
	m_running = false;
	m_condition.notify_one();
}

Irccd *irccd{nullptr};

} // !irccd

#if 0

#if !defined(_WIN32)
#  include <sys/types.h>
#  include <grp.h>
#  include <pwd.h>
#  include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
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

#include "Irccd.h"
#include "Listener.h"
#include "System.h"
#include "ServerManager.h"
#include "PluginManager.h"

#include "server/Disconnected.h"
#include "server/Running.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#  include "EventQueue.h"
#  include "Rule.h"
#  include "RuleManager.h"
#endif

using namespace std::literals;

namespace irccd {

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

#if defined(WITH_LUA)
	// Add user's path
	oss << Util::pathUser() << "plugins/";
	PluginManager::instance().addPath(oss.str());

	// Add system's path
	oss.str("");
	if (!Util::isAbsolute(MODDIR))
		oss << Util::pathBase();

	oss << MODDIR << Util::DIR_SEP;
	PluginManager::instance().addPath(oss.str());
#endif
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
	try {
		if (!isOverriden(Options::Config)) {
			try {
				m_configPath = Util::findConfiguration("irccd.conf");
				config = Parser(m_configPath);
			} catch (const std::runtime_error &ex) {
				Logger::fatal(1, "irccd: %s", ex.what());
			}
		} else
			config = Parser(m_configPath);
	} catch (const std::runtime_error &ex) {
		Logger::fatal(1, "irccd: could not open %s, exiting", m_configPath.c_str());
	}

	Logger::log("irccd: using configuration %s", m_configPath.c_str());

	/*
	 * Order is important, load everything we can before plugins so that
	 * they can use identities, configuration and such, but servers
	 * requires plugin to be loaded.
	 */
	readGeneral(config);
	readIdentities(config);
	readRules(config);
	readListeners(config);
	readPlugins(config);

#if !defined(_WIN32)
	if (!m_foreground) {
		Logger::log("irccd: forking to background...");
		(void)daemon(0, 0);
	}
#endif

#if defined(WITH_LUA)
	/* Now, we load plugins specified by command line */
	for (auto s : m_wantedPlugins) {
		try {
			PluginManager::instance().load(s);
		} catch (const std::exception &error) {
			Logger::warn("irccd: %s", error.what());
		}
	}
#endif

	readServers(config);
}

void Irccd::readGeneral(const Parser &config)
{
	if (config.hasSection("general")) {
		auto general = config.getSection("general");

#if defined(WITH_LUA)
		// Extract parameters that are needed for the next
		if (general.hasOption("plugin-path"))
			PluginManager::instance().addPath(general.getOption<std::string>("plugin-path"));
#endif

#if !defined(_WIN32)
		if (general.hasOption("syslog"))
			Logger::setSyslog(general.getOption<bool>("syslog"));
		if (general.hasOption("foreground") && !isOverriden(Options::Foreground))
			m_foreground = general.getOption<bool>("foreground");
		if (general.hasOption("verbose") && !isOverriden(Options::Verbose))
			Logger::setVerbose(general.getOption<bool>("verbose"));

		m_uid = parse(general, "uid", false);
		m_gid = parse(general, "gid", true);

		if (setgid(m_gid) < 0)
			Logger::warn("irccd: failed to set gid to %s: %s",
			    idname(true).c_str(), std::strerror(errno));
		if (setuid(m_uid) < 0)
			Logger::warn("irccd: failed to set uid to %s: %s",
			    idname(false).c_str(), std::strerror(errno));
#endif
	}
}

#if !defined(_WIN32)

int Irccd::parse(const Section &section, const char *name, bool isgid)
{
	int result(0);

	if (!section.hasOption(name))
		return 0;

	auto value = section.getOption<std::string>(name);

	try {
		if (isgid) {
			auto group = getgrnam(value.c_str());
			result = (group == nullptr) ? std::stoi(value) : group->gr_gid;
		} else {
			auto pw = getpwnam(value.c_str());
			result = (pw == nullptr) ? std::stoi(value) : pw->pw_uid;
		}
	} catch (...) {
		Logger::warn("irccd: invalid %sid %s", ((isgid) ? "g" : "u"), value.c_str());
	}

	return result;
}

std::string Irccd::idname(bool isgid)
{
	std::string result;

	if (isgid) {
		auto group = getgrgid(m_gid);
		result = (group == nullptr) ? std::to_string(m_gid) : group->gr_name;
	} else {
		auto pw = getpwuid(m_uid);
		result = (pw == nullptr) ? std::to_string(m_uid) : pw->pw_name;
	}

	return result;
}

#endif

void Irccd::readPlugins(const Parser &config)
{
	// New way of loading plugins
	if (config.hasSection("plugins")) {
#if defined(WITH_LUA)
		Section section = config.getSection("plugins");

		for (auto opt : section) {
			try {
				if (opt.second.length() == 0)
					PluginManager::instance().load(opt.first);
				else
					PluginManager::instance().load(opt.second, true);
			} catch (const std::runtime_error &error) {
				Logger::warn("irccd: %s", error.what());
			}
		}
#else
		(void)config;
		Logger::warn("irccd: ignoring plugins, Lua support is disabled");
#endif
	}
}

void Irccd::readIdentities(const Parser &config)
{
	config.findSections("identity", [&] (const Section &s) {
		Server::Identity identity;

		try {
			identity.name = s.requireOption<std::string>("name");

			if (s.hasOption("nickname"))
				identity.nickname = s.getOption<std::string>("nickname");
			if (s.hasOption("username"))
				identity.username = s.getOption<std::string>("username");
			if (s.hasOption("realname"))
				identity.realname = s.getOption<std::string>("realname");
			if (s.hasOption("ctcp-version"))
				identity.ctcpVersion = s.getOption<std::string>("ctcp-version");

			Logger::log("identity: found identity %s (%s, %s, \"%s\")",
			    identity.name.c_str(),
			    identity.nickname.c_str(), identity.username.c_str(),
			    identity.realname.c_str());

			m_identities.push_back(identity);
		} catch (const std::out_of_range &ex) {
			Logger::log("identity: parameter %s", ex.what());
		}
	});
}

void Irccd::readRules(const Parser &config)
{
#if defined(WITH_LUA)
	using std::string;

	config.findSections("rule", [&] (const Section &s) {
		RuleMap servers = getList(s, "servers");
		RuleMap channels = getList(s, "channels");
		RuleMap nicknames = getList(s, "nicknames");
		RuleMap plugins = getList(s, "plugins");
		RuleMap events = getList(s, "events");
		RuleAction action{RuleAction::Accept};

		if (!s.hasOption("action")) {
			Logger::warn("rule: missing action");
			return;
		}

		auto value = s.getOption<std::string>("action");

		if (value == "drop") {
			action = RuleAction::Drop;
		} else if (value == "accept") {
			action = RuleAction::Accept;
		} else {
			Logger::warn("rule: invalid action value `%s`", value.c_str());
			return;
		}

		Logger::debug("rule: found rule (%s)", (action == RuleAction::Accept) ? "accept" : "drop");

		RuleManager::instance().add(Rule{servers, channels, nicknames, plugins, events, action});
	});
#else
	(void)config;
	Logger::warn("irccd: ignoring rules, Lua support is disabled");
#endif
}

void Irccd::readListeners(const Parser &config)
{
	config.findSections("listener", [&] (const Section &s) {
		try {
			std::string type = s.requireOption<std::string>("type");

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
		} catch (const std::exception &ex) {
			Logger::warn("listener: parameter %s", ex.what());
		}
	});
}

void Irccd::extractInternet(const Section &s)
{
	std::vector<std::string> protocols;
	std::string address = "*", family;
	int port;
	bool ipv4 = false, ipv6 = false;

	if (s.hasOption("address"))
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

		Socket inet((ipv6) ? AF_INET6 : AF_INET, SOCK_STREAM, 0);

		inet.set(SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));
		if (ipv6) {
			int mode = !ipv4;
			inet.set(IPPROTO_IPV6, IPV6_V6ONLY, &mode, sizeof (mode));
		}

		inet.bind(BindAddressIP(address, port, (ipv6) ? AF_INET6 : AF_INET));
		inet.listen(64);

		Listener::instance().add(inet);
		Logger::log("listener: listening for clients on port %d...", port);
	} catch (const SocketError &ex) {
		Logger::warn("listener: internet socket error: %s", ex.what());
	}
}

#if !defined(_WIN32)
void Irccd::extractUnix(const Section &s)
{
	auto path = s.requireOption<std::string>("path");

	// First remove the dust
	if (Util::exist(path) && remove(path.c_str()) < 0) {
		Logger::warn("listener: error removing %s: %s", path.c_str(), strerror(errno));
	} else {
		try {
			Socket un(AF_UNIX, SOCK_STREAM, 0);

			un.bind(AddressUnix(path, true));
			un.listen(64);

			Listener::instance().add(un);
			Logger::log("listener: listening for clients on %s...", path.c_str());
		} catch (const SocketError &ex) {
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
			Server::Identity identity;
			Server::RetryInfo reco;
			unsigned options = 0;

			// Server information
			info.name = s.requireOption<std::string>("name");
			info.host = s.requireOption<std::string>("host");
			info.port = s.requireOption<int>("port");

			if (s.hasOption("command-char"))
				info.command = s.getOption<std::string>("command-char");

			// Some boolean options
			if (s.hasOption("ssl") && s.getOption<bool>("ssl"))
				options |= Server::OptionSsl;
			if (s.hasOption("ssl-verify") && !s.getOption<bool>("ssl-verify"))
				options |= Server::OptionSslNoVerify;
			if (s.hasOption("join-invite") && s.getOption<bool>("join-invite"))
				options |= Server::OptionJoinInvite;
			if (s.hasOption("auto-rejoin") && s.getOption<bool>("auto-rejoin"))
				options |= Server::OptionAutoRejoin;

			if (s.hasOption("password"))
				info.password = s.getOption<std::string>("password");

			// Identity
			if (s.hasOption("identity"))
				identity = findIdentity(s.getOption<std::string>("identity"));

			// Reconnection settings
			if (s.hasOption("reconnect"))
				reco.enabled = s.getOption<bool>("reconnect");
			if (s.hasOption("reconnect-tries"))
				reco.maxretries = s.getOption<int>("reconnect-tries");
			if (s.hasOption("reconnect-timeout"))
				reco.timeout = s.getOption<int>("reconnect-timeout");

			auto server = std::make_shared<Server>(info, identity, reco, options);
			auto &manager = ServerManager::instance();

			// Extract channels to auto join
			extractChannels(s, server);
			if (manager.has(info.name))
				Logger::warn("server %s: duplicated server", info.name.c_str());
			else
				manager.add(std::move(server));
		} catch (const std::out_of_range &ex) {
			Logger::warn("server: parameter %s", ex.what());
		}
	});
}

void Irccd::extractChannels(const Section &section, std::shared_ptr<Server> &server)
{
	std::vector<std::string> channels;
	std::string list, name, password;

	if (section.hasOption("channels")) {
		list = section.getOption<std::string>("channels");
		channels = Util::split(list, " \t");

		for (const auto &s : channels)
			server->addChannel(Server::toChannel(s));
	}
}

Irccd::~Irccd()
{
	Socket::finish();
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
#if defined(WITH_LUA)
	m_wantedPlugins.push_back(name);
#else
	(void)name;
#endif
}

const Server::Identity &Irccd::findIdentity(const std::string &name)
{
	/*
	 * When name is length 0 that mean user hasn't defined an identity
	 * because it's optional, we don't write an empty message error.
	 */
	if (name.length() == 0)
		return m_defaultIdentity;

	for (const auto &i : m_identities)
		if (i.name == name)
			return i;

	Logger::warn("identity: %s not found", name.c_str());

	return m_defaultIdentity;
}

int Irccd::run()
{
#if defined(WITH_LUA)
	// Start the IrcEvent thread
	//EventQueue::instance().start();
#endif

	openConfig();

	while (m_running) {
		try {
			/*
			 * If no listeners is enabled, we must wait a bit to avoid
			 * CPU usage exhaustion.
			 */
			if (Listener::instance().count() == 0)
				std::this_thread::sleep_for(50ms);
			else
				Listener::instance().process();
		} catch (const std::exception &ex) {
			Logger::warn(ex.what());
		}
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
	Listener::instance().close();
	ServerManager::instance().forAll([] (auto &s) {
		s->stop();
	});
}

} // !irccd

#endif
