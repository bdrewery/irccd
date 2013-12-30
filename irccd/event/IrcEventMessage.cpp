/*
 * IrcEventMessage.cpp -- on channel message event
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

#include "IrcEventMessage.h"

namespace irccd {

IrcEventMessage::IrcEventMessage(Server::Ptr server,
				 const std::string &channel,
				 const std::string &who,
				 const std::string &message)
	: m_server(server)
	, m_channel(channel)
	, m_who(who)
	, m_message(message)
{
}

void IrcEventMessage::action(lua_State *L) const
{
	auto cc = m_server->getOptions().commandChar;
	auto sp = cc + Process::info(L).name;

	// handle special commands "!<plugin> command"
	if (cc.length() > 0 && m_message.compare(0, sp.length(), sp) == 0) {
		auto plugin = m_message.substr(cc.length(), sp.length() - cc.length());

		if (plugin == Process::info(L).name) {
			call(L, "onCommand", m_server, m_channel, m_who, m_message.substr(sp.length() + 1));
		}
	} else {
		call(L, "onMessage", m_server, m_channel, m_who, m_message);
	}
}

} // !irccd
