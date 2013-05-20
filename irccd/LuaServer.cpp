/*
 * LuaServer.cpp -- Lua API for Server class
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

#define SERVER_TYPE	"ServerType"

#include <Logger.h>

#include "Server.h"
#include "LuaServer.h"

using namespace irccd;
using namespace std;

namespace methods {

static int getName(lua_State *L)
{
	Server *s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);

	lua_pushstring(L, s->getName().c_str());

	return 1;
}

static int join(lua_State *L)
{
	if (lua_gettop(L) < 2) {
		Logger::warn("server:join needs at least 1 argument");
	} else {
		Server *s;
		string channel, password = "";

		s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);
		channel = luaL_checkstring(L, 2);

		// optional password
		if (lua_gettop(L) == 3)
			password = luaL_checkstring(L, 3);

		s->join(channel, password);
	}

	return 0;
}

static int kick(lua_State *L)
{
	if (lua_gettop(L) < 3) {
		Logger::warn("server:kick needs at least 2 arguments");
	} else {
		Server *s;
		string channel, target, reason = "";

		s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);
		target = luaL_checkstring(L, 2);
		channel = luaL_checkstring(L, 3);

		// optional reason
		if (lua_gettop(L) == 4)
			reason = luaL_checkstring(L, 4);

		s->kick(target, channel, reason);
	}

	return 0;
}

static int me(lua_State *L)
{
	if (lua_gettop(L) != 3) {
		Logger::warn("server:me needs 2 arguments");
	} else {
		Server *s;
		string target, message;

		s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);
		target = luaL_checkstring(L, 2);
		message = luaL_checkstring(L, 3);

		s->me(target, message);
	}

	return 0;
}

static int nick(lua_State *L)
{
	if (lua_gettop(L) != 2) {
		Logger::warn("server:nick needs 1 argument");
	} else {
		Server *s;
		string newnick;

		s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);
		newnick = luaL_checkstring(L, 2);

		s->nick(newnick);
	}

	return 0;
}

static int say(lua_State *L)
{
	if (lua_gettop(L) != 3) {
		Logger::warn("server:say needs 2 arguments");
	} else {
		Server *s;
		string target, message;

		s = *(Server **)luaL_checkudata(L, 1, SERVER_TYPE);
		target = luaL_checkstring(L, 2);
		message = luaL_checkstring(L, 3);

		s->say(target, message);
	}

	return 0;
}

} // !methods

static const luaL_Reg methodList[] = {
	{ "getName",	methods::getName		},
	{ "join",	methods::join			},
	{ "kick",	methods::kick			},
	{ "me",		methods::me			},
	{ "nick",	methods::nick			},
	{ "say",	methods::say			},
	{ nullptr,	nullptr				}
};

void LuaServer::pushObject(lua_State *L, Server *server)
{
	Server **ptr;
	ptr = (Server **)lua_newuserdata(L, sizeof (Server *));
	luaL_setmetatable(L, SERVER_TYPE);

	*ptr = server;
}

int irccd::luaopen_server(lua_State *L)
{
	// Create the metatable for Server
	luaL_newmetatable(L, SERVER_TYPE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, methodList, 0);
	lua_pop(L, 1);

	return 0;
}
