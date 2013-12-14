/*
 * LuaServer.cpp -- Lua API for Server class
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


#include <sstream>
#include <unordered_map>

#include "Irccd.h"
#include "DefCall.h"
#include "LuaServer.h"

namespace irccd {

namespace {

void extractChannels(lua_State *L, Server::Info &info)
{
	if (Luae::typeField(L, 1, "channels") == LUA_TTABLE) {
		lua_getfield(L, 1, "channels");
		Luae::readTable(L, -1, [&] (lua_State *L, int, int tvalue) {
			Server::Channel c;

			// Standard string channel (no password)
			if (tvalue == LUA_TSTRING) {
				c.m_name = lua_tostring(L, -1);
				info.m_channels.push_back(c);
			} else if (tvalue == LUA_TTABLE) {
				// First index is channel name
				lua_rawgeti(L, -1, 1);
				if (lua_type(L, -1) == LUA_TSTRING)
					c.m_name = lua_tostring(L, -1);
				lua_pop(L, 1);

				// Second index is channel password
				lua_rawgeti(L, -1, 2);
				if (lua_type(L, -1) == LUA_TSTRING)
					c.m_password = lua_tostring(L, -1);
				lua_pop(L, 1);
	
				info.m_channels.push_back(c);
			}
		});
		lua_pop(L, 1);
	}
}

void extractIdentity(lua_State *L, Server::Identity &ident)
{
	std::unordered_map<std::string, std::string &> table {
		{ "name",		ident.m_name		},
		{ "nickname",		ident.m_nickname	},
		{ "username",		ident.m_username	},
		{ "realname",		ident.m_realname	},
	};

	std::string key;

	if (Luae::typeField(L, 1, "identity") == LUA_TTABLE) {
		lua_getfield(L, 1, "identity");
		Luae::readTable(L, -1, [&] (lua_State *L, int tkey, int tvalue) {
			if (tkey == LUA_TSTRING && tvalue == LUA_TSTRING) {
				key = lua_tostring(L, -2);

				if (table.count(key) > 0)
					table[key] = lua_tostring(L, -1);
			}
		});
		lua_pop(L, 1);
	}
}

int serverGetChannels(lua_State *L)
{
	Server::Ptr s;
	int i = 0;

	s = Luae::getShared<Server>(L, 1, ServerType);

	// Create table even if no channels
	lua_createtable(L, s->getChannels().size(), s->getChannels().size());
	i = 0;
	for (auto c : s->getChannels()) {
		lua_pushinteger(L, ++i);
		lua_pushstring(L, c.m_name.c_str());
		lua_settable(L, -3);
	}

	return 1;
}

int serverGetIdentity(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
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

int serverGetInfo(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);

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

int serverGetName(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);

	lua_pushstring(L, s->getInfo().m_name.c_str());

	return 1;
}

int serverCnotice(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);
	std::string notice = luaL_checkstring(L, 3);

	s->cnotice(channel, notice);

	return 0;
}

int serverInvite(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string nick = luaL_checkstring(L, 2);
	std::string channel = luaL_checkstring(L, 3);

	s->invite(nick, channel);

	return 0;
}

int serverJoin(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);
	std::string password;

	// optional password
	if (lua_gettop(L) == 3)
		password = luaL_checkstring(L, 3);

	s->join(channel, password);

	return 0;
}

int serverKick(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string target = luaL_checkstring(L, 2);
	std::string channel = luaL_checkstring(L, 3);
	std::string reason;

	// optional reason
	if (lua_gettop(L) == 4)
		reason = luaL_checkstring(L, 4);

	s->kick(target, channel, reason);

	return 0;
}

int serverMe(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string target = luaL_checkstring(L, 2);
	std::string message = luaL_checkstring(L, 3);

	s->me(target, message);

	return 0;
}

int serverMode(lua_State *L)
{
	std::shared_ptr<Server> s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);
	std::string mode = luaL_checkstring(L, 3);

	s->mode(channel, mode);

	return 0;
}

int serverNames(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);
	int ref;

	luaL_checktype(L, 3, LUA_TFUNCTION);

	try {
		Plugin::Ptr p = Plugin::find(L);

		// Get the function reference.
		lua_pushvalue(L, 3);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Plugin::defer(s, DefCall(IrcEventType::Names, p, ref));
		s->names(channel);
	} catch (std::out_of_range) { }

	// Deferred call
	return 0;
}

int serverNick(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string newnick = luaL_checkstring(L, 2);

	s->nick(newnick);

	return 0;
}

int serverNotice(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string nickname = luaL_checkstring(L, 2);
	std::string notice = luaL_checkstring(L, 3);

	s->notice(nickname, notice);

	return 0;
}

int serverPart(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);

	s->part(channel);

	return 0;
}

int serverQuery(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string target = luaL_checkstring(L, 2);
	std::string message = luaL_checkstring(L, 3);

	s->query(target, message);

	return 0;
}

int serverSay(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string target = luaL_checkstring(L, 2);
	std::string message = luaL_checkstring(L, 3);

	s->say(target, message);

	return 0;
}

int serverSend(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string message = luaL_checkstring(L, 2);

	s->sendRaw(message);

	return 0;
}

int serverTopic(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string channel = luaL_checkstring(L, 2);
	std::string topic = luaL_checkstring(L, 3);

	s->topic(channel, topic);

	return 0;
}

int serverUmode(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string mode = luaL_checkstring(L, 2);

	s->umode(mode);

	return 0;
}

int serverWhois(lua_State *L)
{
	Server::Ptr s = Luae::getShared<Server>(L, 1, ServerType);
	std::string target = luaL_checkstring(L, 2);
	int ref;

	luaL_checktype(L, 3, LUA_TFUNCTION);

	try {
		Plugin::Ptr p = Plugin::find(L);

		// Get the function reference.
		lua_pushvalue(L, 3);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);

		Plugin::defer(s, DefCall(IrcEventType::Whois, p, ref));

		s->whois(target);
	} catch (std::out_of_range) { }

	// Deferred call
	return 0;
}

const luaL_Reg serverMethods[] = {
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

int serverTostring(lua_State *L)
{
	Server::Ptr server;
	std::ostringstream oss;

	server = Luae::getShared<Server>(L, 1, ServerType);
	oss << "Server " << server->getInfo().m_name;
	oss << " at " << server->getInfo().m_host;

	if (server->getInfo().m_ssl)
		oss << " (using SSL)" << std::endl;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

int serverEquals(lua_State *L)
{
	Server::Ptr s1, s2;

	s1 = Luae::getShared<Server>(L, 1, ServerType);
	s2 = Luae::getShared<Server>(L, 2, ServerType);

	lua_pushboolean(L, s1 == s2);

	return 1;
}

int serverGc(lua_State *L)
{
	(static_cast<Server::Ptr *>(luaL_checkudata(L, 1, ServerType)))->~shared_ptr<Server>();

	return 0;
}

const luaL_Reg serverMt[] = {
	{ "__tostring",		serverTostring			},
	{ "__eq",		serverEquals			},
	{ "__gc",		serverGc			},
	{ nullptr,		nullptr				}
};

int l_find(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	int ret;

	try {
		Server::Ptr server = Server::get(name);

		Luae::pushShared<Server>(L, server, ServerType);

		ret = 1;
	} catch (std::out_of_range ex) {
		lua_pushnil(L);
		lua_pushstring(L, ex.what());

		ret = 2;
	}

	return ret;
}

int l_connect(lua_State *L)
{
	Server::Ptr server;
	Server::Info info;
	Server::Identity ident;
	Server::Options options;

	luaL_checktype(L, 1, LUA_TTABLE);

	info.m_name	= Luae::requireField<std::string>(L, 1, "name");
	info.m_host	= Luae::requireField<std::string>(L, 1, "host");
	info.m_port	= Luae::requireField<int>(L, 1, "port");

	if (Luae::typeField(L, 1, "password") == LUA_TSTRING)
		info.m_password = Luae::requireField<std::string>(L, 1, "password");

	extractChannels(L, info);
	extractIdentity(L, ident);

	server = std::make_shared<Server>(info, ident, options);
	Server::add(server);

	return 0;
}

const luaL_Reg functions[] = {
	{ "find",		l_find				},
	{ "connect",		l_connect			},
	{ nullptr,		nullptr				}
};

}

const char *ServerType = "Server";

int luaopen_server(lua_State *L)
{
	luaL_newlib(L, functions);

	// Create the metatable for Server
	luaL_newmetatable(L, ServerType);
	luaL_setfuncs(L, serverMt, 0);
	luaL_newlib(L, serverMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd
