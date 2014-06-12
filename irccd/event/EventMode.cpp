/*
 * EventMode.cpp -- on channel mode
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

#include "EventMode.h"
#include "Plugin.h"

namespace irccd {

EventMode::EventMode(const std::shared_ptr<Server> &server,
		     const std::string &channel,
		     const std::string &nickname,
		     const std::string &mode,
		     const std::string &argument)
	: Event(server->info().name, channel)
	, m_server(server)
	, m_channel(channel)
	, m_nickname(nickname)
	, m_mode(mode)
	, m_argument(argument)
{
}

void EventMode::call(Plugin &p)
{
	p.onMode(m_server, m_channel, m_nickname, m_mode, m_argument);
}

const char *EventMode::name() const
{
	return "onMode";
}

} // !irccd
