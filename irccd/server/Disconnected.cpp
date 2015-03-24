/*
 * Disconnected.cpp -- server disconnected but not dead
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

#include "Connecting.h"
#include "Dead.h"
#include "Disconnected.h"
#include "Server.h"

namespace irccd {

namespace state {

Disconnected::Disconnected()
{
	Logger::debug("server: switching to state \"Disconnected\"");
}

void Disconnected::prepare(Server &server, fd_set &, fd_set &, int &)
{
	const ServerInfo &info = server.info();
	ServerSettings &settings = server.settings();

	// if ServerSettings::recotries it set to -1, reconnection is completely disabled.
	if (settings.recotries < 0) {
		Logger::warn("server %s: reconnection disabled, skipping", info.name.c_str());
		server.next<state::Dead>();
	} else if ((settings.recocurrent + 1) > settings.recotries) {
		Logger::warn("server %s: giving up", info.name.c_str());
		server.next<state::Dead>();
	} else {
		if (m_timer.elapsed() > static_cast<unsigned>(settings.recotimeout * 1000)) {
			irc_disconnect(server.session());

			settings.recocurrent ++;
			server.next<state::Connecting>();
		}
	}
}

int Disconnected::state() const noexcept
{
	return ServerState::Disconnected;
}

} // !state

} // !irccd
