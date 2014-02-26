/*
 * IrcEventKick.cpp -- on channel kick
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

#include "IrcEventKick.h"

namespace irccd {

IrcEventKick::IrcEventKick(Server::Ptr server,
			   const std::string &originator,
			   const std::string &channel,
			   const std::string &target,
			   const std::string &reason)
	: m_server(server)
	, m_originator(originator)
	, m_channel(channel)
	, m_target(target)
	, m_reason(reason)
{
}

void IrcEventKick::action(lua_State *L) const
{
	call(L, "onKick", m_server, m_channel, m_originator, m_target, m_reason);
}

} // !irccd
