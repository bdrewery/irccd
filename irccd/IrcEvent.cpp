/*
 * IrcEvent.cpp -- IRC event passed through plugins
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

#include <vector>
#include <string>

#include "Lua/LuaServer.h"

#include "IrcEvent.h"
#include "Luae.h"
#include "Server.h"

namespace irccd {

template <>
void IrcEvent::push<int>(lua_State *L, const int &value) const
{
	lua_pushinteger(L, value);
}

template <>
void IrcEvent::push<std::string>(lua_State *L, const std::string &value) const
{
	lua_pushlstring(L, value.c_str(), value.length());
}

template <>
void IrcEvent::push<std::vector<std::string>>(lua_State *L,
					      const std::vector<std::string> &value) const
{
	int i = 0;

	lua_createtable(L, value.size(), 0);
	for (const auto &s : value) {
		lua_pushlstring(L, s.c_str(), s.length());
		lua_rawseti(L, -2, ++i);
	}
}

template <>
void IrcEvent::push<Server::Ptr>(lua_State *L, const Server::Ptr &server) const
{
	Luae::pushShared<Server>(L, server, ServerType);
}

void IrcEvent::callFunction(lua_State *L, int np) const
{
	if (lua_pcall(L, np, 0, 0) != LUA_OK) {
		auto error = lua_tostring(L, -1);
		lua_pop(L, 1);
		
		throw Plugin::ErrorException(Process::info(L).name, error);
	}
}

IrcEvent::~IrcEvent()
{
}

} // !irccd
