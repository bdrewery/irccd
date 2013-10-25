/*
 * DefCall.cpp -- deferred plugin function call
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#include "DefCall.h"

namespace irccd
{

void DefCall::call(int nparams)
{
	lua_State *L = m_plugin->getState().get();

	bool result = lua_pcall(L, nparams, 0, 0) == LUA_OK;
	luaL_unref(L, LUA_REGISTRYINDEX, m_ref);
	
	if (!result)
	{
		std::string error = lua_tostring(L, -1);
		lua_pop(L, 1);

		throw Plugin::ErrorException(m_plugin->getName(), error);
	}
}

DefCall::DefCall()
{
}

DefCall::DefCall(IrcEventType type, std::shared_ptr<Plugin> plugin, int ref)
	: m_type(type)
	, m_plugin(plugin)
	, m_ref(ref)
{
}

IrcEventType DefCall::type() const
{
	return m_type;
}

void DefCall::onNames(const std::vector<std::string> &users)
{
	lua_State *L = m_plugin->getState().get();

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_ref);
	lua_createtable(L, users.size(), users.size());

	for (size_t i = 0; i < users.size(); ++i)
	{
		lua_pushstring(L, users[i].c_str());
		lua_rawseti(L, -2, i + 1);
	}
	
	call(1);
}

void DefCall::onWhois(const std::vector<std::string> &params)
{
	lua_State *L = m_plugin->getState().get();

	lua_rawgeti(L, LUA_REGISTRYINDEX, m_ref);
	lua_createtable(L, 0, 0);

	lua_pushstring(L, params[0].c_str());
	lua_setfield(L, -2, "nickname");

	lua_pushstring(L, params[1].c_str());
	lua_setfield(L, -2, "user");

	lua_pushstring(L, params[2].c_str());
	lua_setfield(L, -2, "host");

	lua_pushstring(L, params[3].c_str());
	lua_setfield(L, -2, "realname");

	if (params.size() >= 4)
	{
		lua_createtable(L, 0, 0);

		for (size_t i = 4; i < params.size(); ++i)
		{
			lua_pushstring(L, params[i].c_str());
			lua_rawseti(L, -2, i - 3);
		}

		lua_setfield(L, -2, "channels");
	}

	call(1);
}

bool DefCall::operator==(const DefCall &c1)
{
	return m_type == c1.m_type &&
	    m_plugin == c1.m_plugin &&
	    m_ref == c1.m_ref;
}

} // !irccd
