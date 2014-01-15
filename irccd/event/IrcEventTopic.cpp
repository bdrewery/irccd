/*
 * IrcEventTopic.cpp -- on channel topic changes
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include "IrcEventTopic.h"

namespace irccd {

IrcEventTopic::IrcEventTopic(Server::Ptr server,
			     const std::string &who,
			     const std::string &channel,
			     const std::string &topic)
	: m_server(server)
	, m_who(who)
	, m_channel(channel)
	, m_topic(topic)
{
}

void IrcEventTopic::action(lua_State *L) const
{
	call(L, "onTopic", m_server, m_channel, m_who, m_topic);
}

} // !irccd
