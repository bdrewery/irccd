/*
 * IrcEventChannelNotice.cpp -- on channel notice event
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

#include "IrcEventChannelNotice.h"

namespace irccd {

IrcEventChannelNotice::IrcEventChannelNotice(Server::Ptr server,
					     const std::string &channel,
					     const std::string &who,
					     const std::string &notice)
	: m_server(server)
	, m_channel(channel)
	, m_who(who)
	, m_notice(notice)
{
}

void IrcEventChannelNotice::action(lua_State *L) const
{
	call(L, "onChannelNotice", m_server, m_who, m_channel, m_notice);
}

} // !irccd
