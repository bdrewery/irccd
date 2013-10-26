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

#include <sstream>

#include "Irccd.h"
#include "LuaServer.h"

namespace irccd
{

void LuaServer::pushObject(lua_State *L, std::shared_ptr<Server> server)
{
	new (L, SERVER_TYPE) std::shared_ptr<Server>(server);
}

#define TO_SSERVER(L, idx) \
	*reinterpret_cast<std::shared_ptr<Server> *>(luaL_checkudata((L), (idx), SERVER_TYPE));

static int serverGetChannels(lua_State *L)
{
	std::shared_ptr<Server> s;
	int i = 0;

	s = TO_SSERVER(L, 1);

	// Create table even if no channels
	lua_createtable(L, s->getChannels().size(), s->getChannels().size());
	i = 0;
	for (const Server::Channel &c : s->getChannels())
	{
		lua_pushinteger(L, ++i);
		lua_pushstring(L, c.m_name.c_str());
		lua_settable(L, -3);
	}

	return 1;
}

static int serverGetIdentity(lua_State *L)
{
	std::shared_ptr<Server> s = TO_SSERVER(L, 1);
	const Server::Identity &ident = s->getIdentity();

	// Create the identity table result
	lua_createtable(L, 5, 5);

	lua_pushstring(L, ident.m_name.c_str());
	lua_setfield(L, -2, "name");

	lua_pushstring(L, ident.m_nickname.c_str());
	lua_setfield(L, -2, "nickname");

	lua_pushstring(L, ident.m_username.c_str());
	lua_setfield(L, -2, "username");

	lua_pushstring(L, ident.m_realname.c_str());
	lua_setfield(L, -2, "realname");

#if 0
	lua_pushstring(L, ident.m_ctcpversion.c_str());
	lua_setfield(L, -2, "ctcpversion");
#endif

	return 1;
}

static int serverGetInfo(lua_State *L)
{
	std::shared_ptr<Server> s = TO_SSERVER(L, 1);

	lua_createtable(L, 3, 3);

	lua_pushstring(L, s->getInfo().m_name.c_str());
	lua_setfield(L, -2, "name");

	lua_pushstring(L, s->getInfo().m_host.c_str());
	lua_setfield(L, -2, "hostname");

	lua_pushinteger(L, s->getInfo().m_port);
	lua_setfield(L, -2, "port");

	lua_pushboolean(L, s->getInfo().m_ssl);
	lua_setfield(L, -2, "ssl");

	lua_pushboolean(L, s->getInfo().m_sslVerify);
	lua_setfield(L, -2, "sslVerify");

	return 1;
}

static int serverGetName(lua_State *L)
{
	std::shared_ptr<Server> s = TO_SSERVER(L, 1);

	lua_pushstring(L, s->getInfo().m_name.c_str());

	return 1;
}

static int serverCnotice(lua_State *L)
{
	if (lua_gettop(L) < 3)
		return luaL_error(L, "server:cnotice needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string channel, notice;

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);
	notice = luaL_checkstring(L, 3);

	s->cnotice(channel, notice);

	return 0;
}

static int serverInvite(lua_State *L)
{
	if (lua_gettop(L) < 3)
		return luaL_error(L, "server:invite needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string nick, channel;

	s = TO_SSERVER(L, 1);
	nick = luaL_checkstring(L, 2);
	channel = luaL_checkstring(L, 3);

	s->invite(nick, channel);

	return 0;
}

static int serverJoin(lua_State *L)
{
	if (lua_gettop(L) < 2)
		return luaL_error(L, "server:join needs at least 1 argument");

	std::shared_ptr<Server> s;
	std::string channel, password = "";

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);

	// optional password
	if (lua_gettop(L) == 3)
		password = luaL_checkstring(L, 3);

	s->join(channel, password);

	return 0;
}

static int serverKick(lua_State *L)
{
	if (lua_gettop(L) < 3)
		return luaL_error(L, "server:kick needs at least 2 arguments");

	std::shared_ptr<Server> s;
	std::string channel, target, reason = "";

	s = TO_SSERVER(L, 1);
	target = luaL_checkstring(L, 2);
	channel = luaL_checkstring(L, 3);

	// optional reason
	if (lua_gettop(L) == 4)
		reason = luaL_checkstring(L, 4);

	s->kick(target, channel, reason);

	return 0;
}

static int serverMe(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:me needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string target, message;

	s = TO_SSERVER(L, 1);
	target = luaL_checkstring(L, 2);
	message = luaL_checkstring(L, 3);

	s->me(target, message);

	return 0;
}

static int serverMode(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:mode needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string channel, mode;

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);
	mode = luaL_checkstring(L, 3);

	s->mode(channel, mode);

	return 0;
}

static int serverNames(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:names needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string channel;
	int ref;

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	try
	{
		std::shared_ptr<Plugin> p = Irccd::getInstance()->findPlugin(L);

		// Get the function reference.
		lua_pushvalue(L, 3);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Irccd::getInstance()->addDeferred(
		    s, DefCall(IrcEventType::Names, p, ref)
		);

		s->names(channel);
	}
	catch (std::out_of_range)
	{
	}

	// Deferred call
	return 0;
}

static int serverNick(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "server:nick needs 1 argument");

	std::shared_ptr<Server> s;
	std::string newnick;

	s = TO_SSERVER(L, 1);
	newnick = luaL_checkstring(L, 2);

	s->nick(newnick);

	return 0;
}

static int serverNotice(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "server:notice needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string nickname, notice;

	s = TO_SSERVER(L, 1);
	nickname = luaL_checkstring(L, 2);
	notice = luaL_checkstring(L, 3);

	s->notice(nickname, notice);

	return 0;
}

static int serverPart(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "server:part needs 1 argument");

	std::shared_ptr<Server> s;
	std::string channel;

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);

	s->part(channel);

	return 0;
}

static int serverQuery(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:query needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string target, message;

	s = TO_SSERVER(L, 1);
	target = luaL_checkstring(L, 2);
	message = luaL_checkstring(L, 3);

	s->query(target, message);

	return 0;
}

static int serverSay(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:say needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string target, message;

	s = TO_SSERVER(L, 1);
	target = luaL_checkstring(L, 2);
	message = luaL_checkstring(L, 3);

	s->say(target, message);

	return 0;
}

static int serverSend(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "server:send needs 1 argument");

	std::shared_ptr<Server> s	= TO_SSERVER(L, 1);
	std::string message		= luaL_checkstring(L, 2);

	s->sendRaw(message);

	return 0;
}

static int serverTopic(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:topic needs 2 arguments");

	std::shared_ptr<Server> s;
	std::string channel, topic;

	s = TO_SSERVER(L, 1);
	channel = luaL_checkstring(L, 2);
	topic = luaL_checkstring(L, 3);

	s->topic(channel, topic);

	return 0;
}

static int serverUmode(lua_State *L)
{
	if (lua_gettop(L) != 2)
		return luaL_error(L, "server:umode needs 1 argument");

	std::shared_ptr<Server> s;
	std::string mode;

	s = TO_SSERVER(L, 1);
	mode = luaL_checkstring(L, 2);

	s->umode(mode);

	return 0;
}

static int serverWhois(lua_State *L)
{
	if (lua_gettop(L) != 3)
		return luaL_error(L, "server:whois needs 2 argument");

	std::shared_ptr<Server> s;
	std::string target;
	int ref;

	s = TO_SSERVER(L, 1);
	target = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	try
	{
		std::shared_ptr<Plugin> p = Irccd::getInstance()->findPlugin(L);

		// Get the function reference.
		lua_pushvalue(L, 3);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Irccd::getInstance()->addDeferred(
		    s, DefCall(IrcEventType::Whois, p, ref)
		);

		s->whois(target);
	}
	catch (std::out_of_range)
	{
	}

	// Deferred call
	return 0;
}

static const luaL_Reg serverMethods[] = {
	{ "getChannels",	serverGetChannels		},
	{ "getIdentity",	serverGetIdentity		},
	{ "getInfo",		serverGetInfo			},
	{ "getName",		serverGetName			},
	{ "cnotice",		serverCnotice			},
	{ "invite",		serverInvite			},
	{ "join",		serverJoin			},
	{ "kick",		serverKick			},
	{ "me",			serverMe			},
	{ "mode",		serverMode			},
	{ "names",		serverNames			},
	{ "nick",		serverNick			},
	{ "notice",		serverNotice			},
	{ "part",		serverPart			},
	{ "query",		serverQuery			},
	{ "say",		serverSay			},
	{ "send",		serverSend			},
	{ "topic",		serverTopic			},
	{ "umode",		serverUmode			},
	{ "whois",		serverWhois			},
	{ nullptr,		nullptr				}
};

static int serverTostring(lua_State *L)
{
	std::shared_ptr<Server> server;
	std::ostringstream oss;

	server = TO_SSERVER(L, 1);
	oss << "Server " << server->getInfo().m_name;
	oss << " at " << server->getInfo().m_host;

	if (server->getInfo().m_ssl)
		oss << " (using SSL)" << std::endl;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

static int serverEquals(lua_State *L)
{
	std::shared_ptr<Server> s1, s2;

	s1 = TO_SSERVER(L, 1);
	s2 = TO_SSERVER(L, 2);

	lua_pushboolean(L, s1 == s2);

	return 1;
}

static int serverGc(lua_State *L)
{
	(static_cast<std::shared_ptr<Server> *>(luaL_checkudata(L, 1, SERVER_TYPE)))->~shared_ptr<Server>();

	return 0;
}

static const luaL_Reg serverMt[] = {
	{ "__tostring",		serverTostring			},
	{ "__eq",		serverEquals			},
	{ "__gc",		serverGc			},
	{ nullptr,		nullptr				}
};

int luaopen_server(lua_State *L)
{
	// Create the metatable for Server
	luaL_newmetatable(L, SERVER_TYPE);
	luaL_setfuncs(L, serverMt, 0);
	luaL_newlib(L, serverMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 0;
}

} // !irccd
