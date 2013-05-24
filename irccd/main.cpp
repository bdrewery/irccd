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

#include <unistd.h>

#include <Logger.h>

#include "Irccd.h"
#include "Test.h"

using namespace irccd;
using namespace std;

static void quit(void)
{
	for (Server *s : Irccd::getInstance()->getServers())
		delete s;
	for (Plugin *p : Irccd::getInstance()->getPlugins())
		delete p;
}

int main(int argc, char **argv)
{
	Irccd *irccd = Irccd::getInstance();
	int ch;

	setprogname(argv[0]);

	while ((ch = getopt(argc, argv, "c:p:v")) != -1) {
		switch (ch) {
		case 'c':
			irccd->setConfigPath(string(optarg));
			break;
		case 'p':
			irccd->addPluginPath(string(optarg));
			break;
		case 'v':
			irccd->setVerbosity(true);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (strcmp(argv[0], "test") == 0)
		test(argc, argv);
		// NOTREACHED
	
	atexit(quit);

	return irccd->run(argc, argv);
}
