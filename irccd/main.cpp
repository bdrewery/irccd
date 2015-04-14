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

#include <Ini.h>
#include <Util.h>

#include "Logger.h"
#include "Irccd.h"

namespace irccd {

void loadServer(const IniSection &sc)
{
	ServerInfo info;
	if (!sc.contains("name")) {
		throw std::invalid_argument("missing name");
	}
	if (!sc.contains("host")) {
		throw std::invalid_argument("missing host");
	}

	info.name = sc["name"].value();
	if (info.name.empty()) {
		throw std::invalid_argument("name can not be empty");
	}
}

void loadServers(const Ini &config)
{
	for (const IniSection &section : config) {
		if (section.key() == "server") {
			try {
				loadServer(section);
			} catch (const std::exception &ex) {
				Logger::warning() << "server: " << ex.what() << std::endl;
			}
		}
	}
}

void loadIdentity(const IniSection &sc)
{
	using std::move;
	using std::string;

	Identity id;
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

	irccd->identityAdd(Identity(move(name), move(nickname), move(username), move(realname), move(ctcpversion)));
}

void loadIdentities(const Ini &config)
{
	for (const IniSection &section : config) {
		if (section.key() == "identity") {
			try {
				loadIdentity(section);
			} catch (const std::exception &ex) {
				Logger::warning() << "identity: " << ex.what() << std::endl;
			}
		}
	}
}

void openConfig(const std::string &path)
{
	try {
		/*
		 * Order matters, take care changing this.
		 */
		Ini config(path);

		loadIdentities(config);
		loadServers(config);
	} catch (const std::exception &ex) {
		Logger::warning() << "irccd: " << ex.what() << std::endl;
		Logger::warning() << "irccd: exiting." << std::endl;
	}
}

void stop(int)
{
	irccd->stop();
}

} // !irccd

int main(int argc, char **argv)
{
	irccd::Util::setProgramPath(argv[0]);

	irccd::Irccd instance;
	irccd::irccd = &instance;

	signal(SIGINT, irccd::stop);
	signal(SIGTERM, irccd::stop);

	irccd::Logger::setVerbose(true);
	irccd::ServerInfo info;
	irccd::ServerSettings settings;

	irccd::openConfig("test.conf");

	info.name = "localhost";
	info.host = "localhost";
	info.port = 6667;
	settings.channels = {
		{ "#staff", "" }
	};

	try {
		instance.serverAdd(info, irccd::Identity(), settings);
	} catch (const std::exception &ex) {
		irccd::Logger::warning() << "failed to add a server: " << ex.what() << std::endl;
	}

	try {
		instance.transportAdd<irccd::TransportInet>(AF_INET6, 40000);
	} catch (const std::exception &ex) {
		irccd::Logger::warning() << "failed: " << ex.what() << std::endl;
	}

	instance.run();

	return 0;
}










#if 0
#include <cstdlib>
#include <cstring>

#include <signal.h>

#include <Logger.h>

#include "Irccd.h"
#include "Test.h"
#include "PluginManager.h"

using namespace irccd;
using namespace std;

namespace {

void quit(int)
{
	Irccd::instance().shutdown();
	Irccd::instance().stop();
}

void usage()
{
	Logger::warn("usage: %s [-fv] [-c config] [-p pluginpath] [-P plugin]", getprogname());
	Logger::fatal(1, "       %s test plugin.lua [command] [parameters...]", getprogname());
}

} // !namespace

int main(int argc, char **argv)
{
	Irccd &irccd = Irccd::instance();
	int ch;

	setprogname("irccd");
	atexit([] () {
		quit(0);
	});

	irccd.initialize();

	while ((ch = getopt(argc, argv, "fc:p:P:v")) != -1) {
		switch (ch) {
		case 'c':
			irccd.setConfigPath(string(optarg));
			irccd.override(Options::Config);
			break;
		case 'f':
			irccd.setForeground(true);
			irccd.override(Options::Foreground);
			break;
		case 'p':
#if defined(WITH_LUA)
			PluginManager::instance().addPath(string(optarg));
#endif
			break;
		case 'P':
			irccd.deferPlugin(string(optarg));
			break;
		case 'v':
			Logger::setVerbose(true);
			irccd.override(Options::Verbose);
			break;
		case '?':
		default:
			usage();
			// NOTREACHED
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0) {
		if (strcmp(argv[0], "test") == 0) {
#if defined(WITH_LUA)
			test(argc, argv);
			// NOTREACHED
#else
			Logger::fatal(1, "irccd: Lua support is disabled");
#endif
		}

		if (strcmp(argv[0], "version") == 0) {
			Logger::setVerbose(true);
			Logger::log("irccd version %s", VERSION);
			Logger::log("Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>");
			Logger::log("");
			Logger::log("Irccd is a customizable IRC bot daemon compatible with Lua plugins");
			Logger::log("to fit your needs.");
			Logger::log("");

#if defined(WITH_LUA)
			auto enabled = "enabled";
#else
			auto enabled = "disabled";
#endif
			Logger::log("* Lua support is %s", enabled);
			std::exit(0);
		}
	}

	signal(SIGINT, quit);
	signal(SIGTERM, quit);

#if defined(SIGQUIT)
	signal(SIGQUIT, quit);
#endif

	return irccd.run();
}

#endif
