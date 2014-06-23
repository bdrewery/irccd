/*
 * EventMe.cpp -- on CTCP Action
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

#include <irccd/Plugin.h>

#include "EventMe.h"

namespace irccd {

EventMe::EventMe(const std::shared_ptr<Server> &server,
		 const std::string &channel,
		 const std::string &nickname,
		 const std::string &message)
	: Event(server->info().name, channel)
	, m_server(server)
	, m_channel(channel)
	, m_nickname(nickname)
	, m_message(message)
{
}

void EventMe::call(Plugin &p)
{
	p.onMe(m_server, m_channel, m_nickname, tryEncode(m_message));
}

const char *EventMe::name() const
{
	return "onMe";
}

} // !irccd
