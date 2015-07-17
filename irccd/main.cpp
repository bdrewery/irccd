/*
 * main.cpp -- irccd main file
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

#include <csignal>
#include <iostream>
#include <memory>
#include <regex>

#include <IrccdConfig.h>

#include <Filesystem.h>
#include <Ini.h>
#include <Logger.h>
#include <Util.h>

#include "Irccd.h"

using namespace std::string_literals;

namespace irccd {

/*
 * Configuration format
 * --------------------------------------------------------
 *
 * [general]
 * uid = number or name (Unix only)
 * gid = number or name (Unix only)
 * foreground = true | false (Unix only)
 *
 * [logs]
 * verbose = true | false
 * type = file | console | syslog (Unix only)
 *
 * (If file)
 * path-log = path to normal messages
 * path-errors = path to error messages
 *
 * [plugins]
 * <plugin name> = path or ""
 *
 * [identity]
 * name = unique name in format [A-Za-z0-9-_]
 * username = IRC username to use
 * nickname = nickname to show
 * realname = real name to show
 * ctcp-version = version to reply
 *
 * [server]
 * name = unique name in format [A-Za-z0-9-_]
 * host = ip or domain name
 * port = port number (Optional, default: 6667)
 * ipv6 = use IPv6 or not (Optional, default: false)
 * ssl = use SSL (Optional, default: false)
 * ssl-verify = verify SSL (Optional, default: false)
 * identity = identity name (Optional, use default)
 * auto-rejoin = true | false (Optional, default: false)
 * channels = space separated list of channels to join in format channel[:password]
 *
 * [plugin.<plugin name>]
 * <parameter name> = <parameter value>
 *
 * [rule]
 * servers = a list of servers that will match the rule
 * channels = a list of channel
 * origins = a list of nicknames
 * plugins = which plugins
 * events = which events (e.g onCommand, onMessage, ...)
 */

void loadPlugin(Irccd &irccd, const IniSection &sc)
{
	for (const IniOption &option : sc) {
		if (option.value().empty()) {
			irccd.loadPlugin(option.key());
		} else {
			irccd.loadPlugin(option.value());
		}
	}
}

void loadPluginConfig(Irccd &irccd, const IniSection &sc, std::string name)
{
	PluginConfig config;

	for (const IniOption &option : sc) {
		config.emplace(option.key(), option.value());
	}

	irccd.addPluginConfig(std::move(name), std::move(config));
}

void loadPlugins(Irccd &irccd, const Ini &config)
{
	std::regex regex("^plugin\\.([A-Za-z0-9-_]+)$");
	std::smatch match;

	/*
	 * Load plugin configurations before we load plugins since we use them
	 * when we load the plugin itself.
	 */
	for (const IniSection &section : config) {
		if (std::regex_match(section.key(), match, regex)) {
			loadPluginConfig(irccd, section, match[1]);
		}
	}

	for (const IniSection &section : config) {
		if (section.key() == "plugins") {
			try {
				loadPlugin(irccd, section);
			} catch (const std::exception &ex) {
				Logger::warning() << "plugin: " << ex.what() << std::endl;
			}
		}
	}
}

void loadServer(Irccd &irccd, const IniSection &sc)
{
	ServerInfo info;
	ServerIdentity identity;
	ServerSettings settings;

	/* Verify name */
	if (!sc.contains("name")) {
		throw std::invalid_argument("missing name");
	} else if (!Util::isIdentifierValid(sc["name"].value())) {
		throw std::invalid_argument("name is not valid");
	} else if (irccd.containsServer(sc["name"].value())) {
		throw std::invalid_argument("server already exists");
	}

	if (!sc.contains("host")) {
		throw std::invalid_argument("missing host");
	}
	if (sc.contains("channels")) {
		for (const std::string &s : Util::split(sc["channels"].value(), " \t")) {
			ServerChannel channel;

			std::string::size_type pos = s.find(":");
			if (pos != std::string::npos) {
				channel.name = s.substr(0, pos);
				channel.password = s.substr(pos + 1);
			} else {
				channel.name = s;
			}

			settings.channels.push_back(std::move(channel));
		}
	}
	if (sc.contains("identity")) {
		identity = irccd.findIdentity(sc["identity"].value());
	}

	info.name = sc["name"].value();
	if (info.name.empty()) {
		throw std::invalid_argument("name can not be empty");
	}
	info.host = sc["host"].value();
	if (sc.contains("port")) {
		try {
			info.port = std::stoi(sc["port"].value());
		} catch (const std::exception &ex) {
			throw std::invalid_argument("`"s + sc["port"].value() + "'"s + ": invalid port number"s);
		}
	}

	irccd.addServer(std::make_shared<Server>(std::move(info), std::move(identity), std::move(settings)));
}

void loadServers(Irccd &irccd, const Ini &config)
{
	for (const IniSection &section : config) {
		if (section.key() == "server") {
			try {
				loadServer(irccd, section);
			} catch (const std::exception &ex) {
				Logger::warning() << "server: " << ex.what() << std::endl;
			}
		}
	}
}

void loadIdentity(Irccd &irccd, const IniSection &sc)
{
	using std::move;
	using std::string;

	ServerIdentity identity;

	if (!sc.contains("name")) {
		throw std::invalid_argument("missing name");
	}

	identity.name = sc["name"].value();
	if (identity.name.empty()) {
		throw std::invalid_argument("name can not be empty");
	}

	/* Optional stuff */
	if (sc.contains("username")) {
		identity.username = sc["username"].value();
	}
	if (sc.contains("realname")) {
		identity.realname = sc["realname"].value();
	}
	if (sc.contains("nickname")) {
		identity.nickname = sc["nickname"].value();
	}
	if (sc.contains("ctcp-version")) {
		identity.ctcpversion = sc["ctcp-version"].value();
	}

	Logger::debug() << "identity " << identity.name << ": "
			<< "nickname=" << identity.nickname << ", username=" << identity.username << ", "
			<< "realname=" << identity.realname << ", ctcp-version=" << identity.ctcpversion << std::endl;

	irccd.addIdentity(std::move(identity));
}

void loadIdentities(Irccd &irccd, const Ini &config)
{
	for (const IniSection &section : config) {
		if (section.key() == "identity") {
			try {
				loadIdentity(irccd, section);
			} catch (const std::exception &ex) {
				Logger::warning() << "identity: " << ex.what() << std::endl;
			}
		}
	}
}

void loadListenerInet(Irccd &irccd, const IniSection &sc)
{
	// TODO ipv4 and ipv6 back
	if (!sc.contains("port")) {
		throw std::invalid_argument("missing port");
	}

	int port = std::stoi(sc["port"].value());

	/* Optional stuff */
	std::string address{"*"};

	if (!sc.contains("address")) {
		address = sc["address"].value();
	}

	irccd.addTransport(std::make_shared<TransportServerIpv4>(address, port));
}

void loadListenerUnix(Irccd &irccd, const IniSection &sc)
{
	(void)irccd;
	(void)sc;
}

void loadListeners(Irccd &irccd, const Ini &config)
{
	for (const IniSection &section : config) {
		if (section.key() == "listener") {
			try {
				if (!section.contains("type")) {
					throw std::invalid_argument("missing type parameter");
				}

				auto type = section["type"].value();

				if (type == "ip")
					loadListenerInet(irccd, section);
				else if (type == "unix")
					loadListenerUnix(irccd, section);
				else
					throw std::invalid_argument("invalid type given");
			} catch (const std::exception &ex) {
				Logger::warning() << "transport: " << ex.what() << std::endl;
			}
		}
	}
}

bool openConfig(Irccd &irccd, const std::string &path)
{
	try {
		/*
		 * Order matters, take care when you change this.
		 */
		Ini config(path);

		loadIdentities(irccd, config);
		loadServers(irccd, config);
		loadPlugins(irccd, config);
		loadListeners(irccd, config);
	} catch (const std::exception &ex) {
		Logger::info() << getprogname() << ": " << path << ": " << ex.what() << std::endl;
		return false;
	}

	return true;
}

void stop(int)
{
	Irccd::stop();
}

} // !irccd

int main(int, char **argv)
{
	try {
	using namespace irccd;

	Logger::setVerbose(true);

	setprogname("irccd");
	Util::setProgramPath(argv[0]);

		Irccd irccd;

	signal(SIGINT, irccd::stop);
	signal(SIGTERM, irccd::stop);

	for (const std::string &path : irccd::Util::pathsConfig()) {
		irccd::Logger::info() << getprogname() << ": trying " << path << irccd::Filesystem::Separator << "irccd.conf" << std::endl;

		if (openConfig(irccd, path + irccd::Filesystem::Separator + "irccd.conf")) {
			break;
		}
	}

	irccd.run();
	} catch (const irccd::SocketError &ex) {
		std::cerr << ex.function() << ": " << ex.what() << std::endl;
	}

	return 0;
}

#if 0

{
	Logger::warn("usage: %s [-fv] [-c config] [-p pluginpath] [-P plugin]", getprogname());
	Logger::fatal(1, "       %s test plugin.lua [command] [parameters...]", getprogname());
}

#endif
