/*
 * IrcEventWhois.cpp -- on whois information reception
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

#include "IrcEventWhois.h"

namespace irccd {

IrcEventWhois::IrcEventWhois(Server::Ptr server, const IrcWhois &info)
	: m_server(server)
	, m_whois(info)
{
}

void IrcEventWhois::action(lua_State *L) const
{
	lua_getglobal(L, "onWhois");

	if (lua_type(L, -1) == LUA_TFUNCTION) {
		push(L, m_server);

		lua_createtable(L, 0, 0);
		lua_pushlstring(L, m_whois.nick.c_str(), m_whois.nick.length());
		lua_setfield(L, -2, "nickname");
		lua_pushlstring(L, m_whois.user.c_str(), m_whois.user.length());
		lua_setfield(L, -2, "user");
		lua_pushlstring(L, m_whois.host.c_str(), m_whois.host.length());
		lua_setfield(L, -2, "host");
		lua_pushlstring(L, m_whois.realname.c_str(), m_whois.realname.length());
		lua_setfield(L, -2, "realname");

		// Store optionnal channels
		lua_createtable(L, 0, 0);

		for (size_t i = 4; i < m_whois.channels.size(); ++i) {
			lua_pushstring(L, m_whois.channels[i].c_str());
			lua_rawseti(L, -2, i - 3);
		}

		lua_setfield(L, -2, "channels");

		callFunction(L, 2);
	} else {
		lua_pop(L, 1);
	}
}

} // !irccd
