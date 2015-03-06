/*
 * ServerRunning.cpp -- server running in a forever loop
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

#include <common/Logger.h>

#include <irccd/Irccd.h>
#include <irccd/Server.h>

#include "Connecting.h"
#include "Running.h"
#include "Disconnected.h"

namespace irccd {

namespace state {

Running::Running()
{
	Logger::debug("server: switching to state \"Running\"");
}

void Running::exec(Server &server)
{
	/*
	 * Warning: do not want to reset the number of tries here, it is
	 * done in the event_connect function because we only know if
	 * we are connected later.
	 */
	server.session().run();
	server.next<Disconnected>();
}

std::string Running::which() const
{
	return "Running";
}

} // !state

} // !irccd
