/*
 * Connected.cpp -- server is connected
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

#include <Logger.h>

#include "Connected.h"
#include "Disconnected.h"
#include "Server.h"

namespace irccd {

namespace state {

Connected::Connected()
{
	Logger::debug() << "server: switching to state \"Connected\"" << std::endl;
}

void Connected::prepare(Server &server, fd_set &setinput, fd_set &setoutput, int &maxfd)
{
	if (!irc_is_connected(server.session())) {
		const ServerSettings &settings = server.settings();

		Logger::warning() << "server " << server.info().name << ": disconnected" << std::endl;

		if (settings.recotimeout > 0) {
			Logger::warning() << "server " << server.info().name << ": retrying in "
					  << settings.recotimeout << " seconds" << std::endl;
		}

		server.next<state::Disconnected>();
	} else {
		irc_add_select_descriptors(server.session(), &setinput, &setoutput, &maxfd);
	}
}

int Connected::state() const noexcept
{
	return ServerState::Connected;
}

} // !state

} // !irccd
