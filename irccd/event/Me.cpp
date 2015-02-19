/*
 * Me.cpp -- on CTCP Action
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

#include "Me.h"

namespace irccd {

namespace event {

Me::Me(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string message)
	: Event(server->info().name, channel)
	, m_server(std::move(server))
	, m_channel(std::move(channel))
	, m_nickname(std::move(nickname))
	, m_message(std::move(message))
{
}

void Me::call(Plugin &p)
{
	p.onMe(m_server, m_channel, m_nickname, tryEncode(m_message));
}

const char *Me::name(Plugin &) const
{
	return "onMe";
}

} // !event

} // !irccd
