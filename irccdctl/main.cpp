/*
 * main.cpp -- irccd controller main
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

#include <IrccdConfig.h>

#include <Logger.h>
#include <OptionParser.h>

#include "Irccdctl.h"

using namespace irccd;

int main(int argc, char **argv)
{
	setprogname("irccd");

	Irccdctl ctl;
	OptionParser parser{
		{ "c",	"config",			},
		{ "v",	"verbose",	Option::NoArg	}
	};

	OptionPack pack = parser.parse(--argc, ++argv);

#if 0
	for (const OptionValue &option : pack) {
		if (option == "c") {
			ctl.define("c", option.value());
		} else if (option == "v") {
			Logger::setVerbose(true);
			ctl.define("v", "");
		}
	}
#endif

	argc -= pack.parsed();
	argv += pack.parsed();

	return ctl.exec(argc, argv);
}
