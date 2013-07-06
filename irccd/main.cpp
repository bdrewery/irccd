/*
 * main.cpp -- irccd main file
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

#include <cstdlib>

#include <signal.h>

#include <Logger.h>

#include "Irccd.h"
#include "Test.h"

using namespace irccd;
using namespace std;

static void quit(int)
{
	Irccd::getInstance()->stop();
}

static void usage()
{
	Logger::warn("usage: %s [-fv] [-c config] [-p pluginpath] [-P plugin]", getprogname());
	Logger::warn("       %s test plugin.lua [command] [parameters...]", getprogname());

	exit(1);
}

int main(int argc, char **argv)
{
	Irccd *irccd = Irccd::getInstance();
	int ch;
	bool foreground = false;

	setprogname(argv[0]);
	
	while ((ch = getopt(argc, argv, "fc:p:P:v")) != -1) {
		switch (ch) {
		case 'c':
			irccd->setConfigPath(string(optarg));
			break;
		case 'f':
			foreground = true;
			break;
		case 'p':
			irccd->addPluginPath(string(optarg));
			break;
		case 'P':
			irccd->addWantedPlugin(string(optarg));
			break;
		case 'v':
			Logger::setVerbose(true);
			break;
		case '?':
		default:
			usage();
			// NOTREACHED
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0 && strcmp(argv[0], "test") == 0) {
#if defined(WITH_LUA)
		test(argc, argv);
		// NOTREACHED
#else
		Logger::warn("The command test is not available, Lua support is disabled");
		exit(1);
#endif
	}

	signal(SIGINT, quit);
	signal(SIGTERM, quit);

#if defined(SIGQUIT)
	signal(SIGQUIT, quit);
#endif

#if !defined(_WIN32)
	if (!foreground)
		daemon(0, 0);
#endif

	return irccd->run();
}
