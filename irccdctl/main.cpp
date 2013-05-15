/*
 * main.cpp -- irccd controller main
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

#include <iostream>

#include <unistd.h>

#include "Irccdctl.h"

using namespace irccd;
using namespace std;

int main(int argc, char **argv)
{
	Irccdctl ctl;
	int ch;

	while ((ch = getopt(argc, argv, "c:v")) != -1) {
		switch (ch) {
		case 'c':
			ctl.setConfigPath(string(optarg));
			break;
		case 'v':
			ctl.setVerbosity(false);
			break;
		}
	}
	argc -= optind;
	argv += optind;

	return ctl.run(argc, argv);
}
