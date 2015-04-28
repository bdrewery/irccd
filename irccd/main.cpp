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
			irccd.pluginLoad(option.key());
		} else {
			irccd.pluginLoad(option.value());
		}
	}
}

void loadPluginConfig(Irccd &irccd, const IniSection &sc, std::string name)
{
	PluginConfig config;

	for (const IniOption &option : sc) {
		config.emplace(option.key(), option.value());
	}

	irccd.pluginAddConfig(std::move(name), std::move(config));
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

	if (!sc.contains("name")) {
		throw std::invalid_argument("missing name");
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
		identity = irccd.identityFind(sc["identity"].value());
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

	irccd.serverAdd(std::move(info), std::move(identity), std::move(settings));
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

	ServerIdentity id;
	string name;
	string username = id.username();
	string realname = id.username();
	string nickname = id.nickname();
	string ctcpversion = id.ctcpversion();

	if (!sc.contains("name")) {
		throw std::invalid_argument("missing name");
	}

	name = sc["name"].value();
	if (name.empty()) {
		throw std::invalid_argument("name can not be empty");
	}

	/* Optional stuff */
	if (sc.contains("username")) {
		username = sc["username"].value();
	}
	if (sc.contains("realname")) {
		realname = sc["realname"].value();
	}
	if (sc.contains("nickname")) {
		nickname = sc["nickname"].value();
	}
	if (sc.contains("ctcp-version")) {
		ctcpversion = sc["ctcp-version"].value();
	}

	Logger::debug() << "identity " << name << ": "
			<< "nickname=" << nickname << ", username=" << username << ", "
			<< "realname=" << realname << ", ctcp-version=" << ctcpversion << std::endl;

	irccd.identityAdd(ServerIdentity(move(name), move(nickname), move(username), move(realname), move(ctcpversion)));
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
	using namespace irccd;

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

	return 0;
}

#if 0

{
	Logger::warn("usage: %s [-fv] [-c config] [-p pluginpath] [-P plugin]", getprogname());
	Logger::fatal(1, "       %s test plugin.lua [command] [parameters...]", getprogname());
}

#endif
