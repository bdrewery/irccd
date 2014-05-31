/*
 * main.cpp -- irccd main file
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include <cstdlib>
#include <cstring>

#include <signal.h>

#include <Logger.h>

#include "Irccd.h"
#include "Test.h"

using namespace irccd;
using namespace std;

namespace {

void quit(int)
{
	Irccd::instance().shutdown();
}

void usage()
{
	Logger::warn("usage: %s [-fv] [-c config] [-p pluginpath] [-P plugin]", getprogname());
	Logger::fatal(1, "       %s test plugin.lua [command] [parameters...]", getprogname());
}

}

int main(int argc, char **argv)
{
	Irccd &irccd = Irccd::instance();
	int ch;

	setprogname("irccd");

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
			Plugin::addPath(string(optarg));
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
			Logger::log("Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>");
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
