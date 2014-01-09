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
	const auto &cc = m_server->getOptions().commandChar;
	const auto &name = Process::info(L).name;
	const auto &msg = m_message;

	// handle special commands "!<plugin> command"
	if (cc.length() > 0) {
		auto pos = msg.find_first_of(" \t");

		/*
		 * If the message that comes is "!foo" without spaces we
		 * compare the command char + the plugin name. If there
		 * is a space, we check until we find a space, if not
		 * typing "!foo123123" will trigger foo plugin.
		 */
		bool should = false;

		if (pos == std::string::npos) {
			should = msg.compare(cc.length(), name.length(), name) == 0 &&
			    msg.length() == cc.length() + name.length();
		} else {
			should = msg.compare(cc.length(), pos - 1, name) == 0;
		}

		if (should) {
			call(L, "onCommand", m_server, m_channel, m_who, m_message.substr(pos + 1));
		}
	} else {
		call(L, "onMessage", m_server, m_channel, m_who, m_message);
	}
}

} // !irccd
