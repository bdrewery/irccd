/*
 * LuaServer.cpp -- Lua API for Server class
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

#include <sstream>
#include <unordered_map>

#include "Irccd.h"
#include "Luae.h"
#include "LuaServer.h"

namespace irccd {

namespace {

/* --------------------------------------------------------
 * Private helpers
 * -------------------------------------------------------- */

void extractChannels(lua_State *L, Server::Info &info)
{
	LUAE_STACK_CHECKBEGIN(L);

	if (LuaeTable::type(L, 1, "channels") == LUA_TTABLE) {
		lua_getfield(L, 1, "channels");
		LuaeTable::read(L, -1, [&] (lua_State *L, int, int tvalue) {
			Server::Channel c;

			// Standard string channel (no password)
			if (tvalue == LUA_TSTRING) {
				c.name = lua_tostring(L, -1);
				info.channels.push_back(c);
			} else if (tvalue == LUA_TTABLE) {
				// First index is channel name
				lua_rawgeti(L, -1, 1);
				if (lua_type(L, -1) == LUA_TSTRING)
					c.name = lua_tostring(L, -1);
				lua_pop(L, 1);

				// Second index is channel password
				lua_rawgeti(L, -1, 2);
				if (lua_type(L, -1) == LUA_TSTRING)
					c.password = lua_tostring(L, -1);
				lua_pop(L, 1);
	
				info.channels.push_back(c);
			}
		});
		lua_pop(L, 1);
	}

	LUAE_STACK_CHECKEQUALS(L);
}

void extractIdentity(lua_State *L, Server::Identity &ident)
{
	LUAE_STACK_CHECKBEGIN(L);

	std::unordered_map<std::string, std::string *> table {
		{ "name",		&ident.name	},
		{ "nickname",		&ident.nickname	},
		{ "username",		&ident.username	},
		{ "realname",		&ident.realname	},
	};

	std::string key;

	if (LuaeTable::type(L, 1, "identity") == LUA_TTABLE) {
		lua_getfield(L, 1, "identity");
		LuaeTable::read(L, -1, [&] (lua_State *L, int tkey, int tvalue) {
			if (tkey == LUA_TSTRING && tvalue == LUA_TSTRING) {
				key = lua_tostring(L, -2);

				if (table.count(key) > 0)
					*table[key] = lua_tostring(L, -1);
			}
		});
		lua_pop(L, 1);
	}

	LUAE_STACK_CHECKEQUALS(L);
}

void pushIdentity(lua_State *L, const Server::Identity &ident)
{
	LUAE_STACK_CHECKBEGIN(L);

	// Create the identity table result
	lua_createtable(L, 5, 5);

	lua_pushlstring(L, ident.name.c_str(), ident.name.length());
	lua_setfield(L, -2, "name");

	lua_pushstring(L, ident.nickname.c_str());
	lua_setfield(L, -2, "nickname");

	lua_pushstring(L, ident.username.c_str());
	lua_setfield(L, -2, "username");

	lua_pushstring(L, ident.realname.c_str());
	lua_setfield(L, -2, "realname");

	LUAE_STACK_CHECKEND(L, - 1);
}

void pushGeneralInfo(lua_State *L, const Server::Info &info, const Server::Options &options)
{
	LUAE_STACK_CHECKBEGIN(L);

	lua_pushlstring(L, info.name.c_str(), info.name.length());
	lua_setfield(L, -2, "name");

	lua_pushlstring(L, info.host.c_str(), info.host.length());
	lua_setfield(L, -2, "hostname");

	lua_pushinteger(L, info.port);
	lua_setfield(L, -2, "port");

	lua_pushboolean(L, info.ssl);
	lua_setfield(L, -2, "ssl");

	lua_pushboolean(L, info.sslVerify);
	lua_setfield(L, -2, "sslVerify");

	lua_pushlstring(L, options.commandChar.c_str(), options.commandChar.length());
	lua_setfield(L, -2, "commandChar");

	LUAE_STACK_CHECKEQUALS(L);
}

void pushChannels(lua_State *L, const Server::ChanList &list)
{
	LUAE_STACK_CHECKBEGIN(L);

	int i = 0;

	// Create table even if no channels
	lua_createtable(L, list.size(), 0);
	i = 0;
	for (const auto &c : list) {
		lua_pushinteger(L, ++i);
		lua_pushlstring(L, c.name.c_str(), c.name.length());
		lua_settable(L, -3);
	}

	LUAE_STACK_CHECKEND(L, - 1);
}

/* --------------------------------------------------------
 * Public methods
 * -------------------------------------------------------- */

#if defined(COMPAT_1_1)

int l_getChannels(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);

	Luae::deprecate(L, "getChannels", "info");
	pushChannels(L, s->getChannels());

	return 1;
}

int l_getIdentity(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto &ident = s->getIdentity();

	Luae::deprecate(L, "getIdentity", "info");
	pushIdentity(L, ident);

	return 1;
}

int l_getInfo(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto &info = s->getInfo();
	auto &options = s->getOptions();

	Luae::deprecate(L, "getInfo", "info");
	lua_createtable(L, 0, 0);
	pushGeneralInfo(L, info, options);

	return 1;
}

int l_getName(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);

	Luae::deprecate(L, "getName", "info");
	lua_pushstring(L, s->getInfo().name.c_str());

	return 1;
}

#endif

int l_cnotice(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);
	auto notice = luaL_checkstring(L, 3);

	s->cnotice(channel, notice);

	return 0;
}

int l_info(lua_State *L)
{
	LUAE_STACK_CHECKBEGIN(L);

	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);

	lua_newtable(L);

	// Store the following fields
	pushGeneralInfo(L, s->getInfo(), s->getOptions());

	// Table for channels
	pushChannels(L, s->getChannels());
	lua_setfield(L, -2, "channels");

	// Table for identity
	pushIdentity(L, s->getIdentity());
	lua_setfield(L, -2, "identity");

	LUAE_STACK_CHECKEND(L, - 1);

	return 1;
}

int l_invite(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto nick = luaL_checkstring(L, 2);
	auto channel = luaL_checkstring(L, 3);

	s->invite(nick, channel);

	return 0;
}

int l_join(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);
	auto password = "";

	// optional password
	if (lua_gettop(L) == 3)
		password = luaL_checkstring(L, 3);

	s->join(channel, password);

	return 0;
}

int l_kick(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto target = luaL_checkstring(L, 2);
	auto channel = luaL_checkstring(L, 3);
	auto reason = "";

	// optional reason
	if (lua_gettop(L) == 4)
		reason = luaL_checkstring(L, 4);

	s->kick(target, channel, reason);

	return 0;
}

int l_me(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto target = luaL_checkstring(L, 2);
	auto message = luaL_checkstring(L, 3);

	s->me(target, message);

	return 0;
}

int l_mode(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);
	auto mode = luaL_checkstring(L, 3);

	s->mode(channel, mode);

	return 0;
}

int l_names(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);

	s->names(channel);

	return 0;
}

int l_nick(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto newnick = luaL_checkstring(L, 2);

	s->nick(newnick);

	return 0;
}

int l_notice(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto nickname = luaL_checkstring(L, 2);
	auto notice = luaL_checkstring(L, 3);

	s->notice(nickname, notice);

	return 0;
}

int l_part(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);
	auto reason = "";

	if (lua_gettop(L) >= 3)
		reason = luaL_checkstring(L, 3);

	s->part(channel, reason);

	return 0;
}

int l_query(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto target = luaL_checkstring(L, 2);
	auto message = luaL_checkstring(L, 3);

	s->query(target, message);

	return 0;
}

int l_say(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto target = luaL_checkstring(L, 2);
	auto message = luaL_checkstring(L, 3);

	s->say(target, message);

	return 0;
}

int l_send(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto message = luaL_checkstring(L, 2);

	s->send(message);

	return 0;
}

int l_topic(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto channel = luaL_checkstring(L, 2);
	auto topic = luaL_checkstring(L, 3);

	s->topic(channel, topic);

	return 0;
}

int l_umode(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto mode = luaL_checkstring(L, 2);

	s->umode(mode);

	return 0;
}

int l_whois(lua_State *L)
{
	auto s = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto target = luaL_checkstring(L, 2);

	s->whois(target);

	return 0;
}

int l_tostring(lua_State *L)
{
	auto server = LuaeClass::getShared<Server>(L, 1, ServerType);
	std::ostringstream oss;

	oss << "Server " << server->getInfo().name;
	oss << " at " << server->getInfo().host;

	if (server->getInfo().ssl)
		oss << " (using SSL)" << std::endl;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

int l_equals(lua_State *L)
{
	auto s1 = LuaeClass::getShared<Server>(L, 1, ServerType);
	auto s2 = LuaeClass::getShared<Server>(L, 2, ServerType);

	lua_pushboolean(L, s1 == s2);

	return 1;
}

int l_gc(lua_State *L)
{
	(static_cast<Server::Ptr *>(luaL_checkudata(L, 1, ServerType)))->~shared_ptr<Server>();

	return 0;
}

const LuaeClass::Def serverDef {
	ServerType,
	{
	/*
	 * DEPRECATION:	1.2-001
	 *
	 * These functions has been deprecated in favor of Server:info().
	 */
#if defined(COMPAT_1_1)
		{ "getChannels",	l_getChannels		},
		{ "getIdentity",	l_getIdentity		},
		{ "getInfo",		l_getInfo		},
		{ "getName",		l_getName		},
#endif
		{ "cnotice",		l_cnotice		},
		{ "info",		l_info			},
		{ "invite",		l_invite		},
		{ "join",		l_join			},
		{ "kick",		l_kick			},
		{ "me",			l_me			},
		{ "mode",		l_mode			},
		{ "names",		l_names			},
		{ "nick",		l_nick			},
		{ "notice",		l_notice		},
		{ "part",		l_part			},
		{ "query",		l_query			},
		{ "say",		l_say			},
		{ "send",		l_send			},
		{ "topic",		l_topic			},
		{ "umode",		l_umode			},
		{ "whois",		l_whois			},
	},
	{
		{ "__tostring",		l_tostring		},
		{ "__eq",		l_equals		},
		{ "__gc",		l_gc			},
	},
	nullptr
};

int l_find(lua_State *L)
{
	auto name = luaL_checkstring(L, 1);
	int ret;

	try {
		Server::Ptr server = Server::get(name);

		LuaeClass::pushShared<Server>(L, server, ServerType);

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
	Server::RetryInfo reco;

	luaL_checktype(L, 1, LUA_TTABLE);

	info.name	= LuaeTable::require<std::string>(L, 1, "name");
	info.host	= LuaeTable::require<std::string>(L, 1, "host");
	info.port	= LuaeTable::require<int>(L, 1, "port");

	if (Server::has(info.name)) {
		lua_pushboolean(L, false);
		lua_pushfstring(L, "server %s already connected",
		    info.name.c_str());

		return 2;
	}

	if (LuaeTable::type(L, 1, "password") == LUA_TSTRING)
		info.password = LuaeTable::require<std::string>(L, 1, "password");

	extractChannels(L, info);
	extractIdentity(L, ident);

	server = std::make_shared<Server>(info, ident, options, reco);
	Server::add(server);

	lua_pushboolean(L, true);

	return 1;
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

	LuaeClass::create(L, serverDef);

	return 1;
}

} // !irccd
