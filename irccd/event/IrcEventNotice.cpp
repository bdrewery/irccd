/*
 * IrcEventNotice.cpp -- on personal notice
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

#include "IrcEventNotice.h"

namespace irccd {

IrcEventNotice::IrcEventNotice(Server::Ptr server,
			       const std::string &who,
			       const std::string &target,
			       const std::string &message)
	: m_server(server)
	, m_who(who)
	, m_target(target)
	, m_message(message)
{
}

void IrcEventNotice::action(lua_State *L) const
{
	call(L, "onNotice", m_server, m_who, m_target, m_message);
}

} // !irccd