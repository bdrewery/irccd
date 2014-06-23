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

#include <irccd/Irccd.h>
#include <irccd/Luae.h>

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
		Luae::getfield(L, 1, "channels");
		LuaeTable::read(L, -1, [&] (lua_State *L, int, int tvalue) {
			Server::Channel c;

			// Standard string channel (no password)
			if (tvalue == LUA_TSTRING) {
				c.name = Luae::get<std::string>(L, -1);
				info.channels.push_back(c);
			} else if (tvalue == LUA_TTABLE) {
				// First index is channel name
				Luae::rawget(L, -1, 1);
				if (Luae::type(L, -1) == LUA_TSTRING)
					c.name = Luae::get<std::string>(L, -1);
				Luae::pop(L, 1);

				// Second index is channel password
				Luae::rawget(L, -1, 2);
				if (Luae::type(L, -1) == LUA_TSTRING)
					c.password = Luae::get<std::string>(L, -1);
				Luae::pop(L, 1);
	
				info.channels.push_back(c);
			}
		});
		Luae::pop(L, 1);
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
		Luae::getfield(L, 1, "identity");
		LuaeTable::read(L, -1, [&] (lua_State *L, int tkey, int tvalue) {
			if (tkey == LUA_TSTRING && tvalue == LUA_TSTRING) {
				key = Luae::get<std::string>(L, -2);

				if (table.count(key) > 0)
					*table[key] = Luae::get<std::string>(L, -1);
			}
		});
		Luae::pop(L, 1);
	}

	LUAE_STACK_CHECKEQUALS(L);
}

void pushIdentity(lua_State *L, const Server::Identity &ident)
{
	LUAE_STACK_CHECKBEGIN(L);

	// Create the identity table result
	LuaeTable::create(L, 5, 5);

	LuaeTable::set(L, -1, "name", ident.name);
	LuaeTable::set(L, -1, "nickname", ident.nickname);
	LuaeTable::set(L, -1, "username", ident.username);
	LuaeTable::set(L, -1, "realname", ident.realname);

	LUAE_STACK_CHECKEND(L, - 1);
}

void pushGeneralInfo(lua_State *L, const Server::Info &info, unsigned options)
{
	LUAE_STACK_CHECKBEGIN(L);

	LuaeTable::set(L, -1, "name", info.name);
	LuaeTable::set(L, -1, "hostname", info.host);
	LuaeTable::set(L, -1, "port", static_cast<int>(info.port));
	LuaeTable::set(L, -1, "ssl", (options & Server::OptionSsl) == 1);
	LuaeTable::set(L, -1, "sslVerify", (options & Server::OptionSslNoVerify) == 0);
	LuaeTable::set(L, -1, "commandChar", info.command);

	LUAE_STACK_CHECKEQUALS(L);
}

void pushChannels(lua_State *L, const Server::ChannelList &list)
{
	LUAE_STACK_CHECKBEGIN(L);

	// Create table even if no channels
	LuaeTable::create(L, list.size());
	auto i = 0;
	for (const auto &c : list) {
		Luae::push(L, c.name);
		Luae::rawset(L, -2, ++i);
	}

	LUAE_STACK_CHECKEND(L, - 1);
}

/* --------------------------------------------------------
 * Public methods
 * -------------------------------------------------------- */

#if defined(COMPAT_1_1)

int l_getChannels(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	Luae::deprecate(L, "getChannels", "info");
	pushChannels(L, s->channels());

	return 1;
}

int l_getIdentity(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	Luae::deprecate(L, "getIdentity", "info");
	pushIdentity(L, s->identity());

	return 1;
}

int l_getInfo(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	Luae::deprecate(L, "getInfo", "info");
	LuaeTable::create(L);
	pushGeneralInfo(L, s->info(), s->options());

	return 1;
}

int l_getName(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	Luae::deprecate(L, "getName", "info");
	Luae::push(L, s->info().name);

	return 1;
}

#endif

int l_cnotice(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->cnotice(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_info(lua_State *L)
{
	LUAE_STACK_CHECKBEGIN(L);

	auto s = Luae::check<Server::Ptr>(L, 1);

	LuaeTable::create(L);

	// Store the following fields
	pushGeneralInfo(L, s->info(), s->options());

	// Table for channels
	pushChannels(L, s->channels());
	Luae::setfield(L, -2, "channels");

	// Table for identity
	pushIdentity(L, s->identity());
	Luae::setfield(L, -2, "identity");

	LUAE_STACK_CHECKEND(L, - 1);

	return 1;
}

int l_invite(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->invite(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_join(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);
	std::string password = "";

	// optional password
	if (Luae::gettop(L) == 3)
		password = Luae::check<std::string>(L, 3);

	s->join(Luae::check<std::string>(L, 2), password);

	return 0;
}

int l_kick(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);
	auto target = Luae::check<std::string>(L, 2);
	auto channel = Luae::check<std::string>(L, 3);
	std::string reason = "";

	// optional reason
	if (Luae::gettop(L) == 4)
		reason = Luae::check<std::string>(L, 4);

	s->kick(target, channel, reason);

	return 0;
}

int l_me(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->me(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_mode(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->mode(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_names(lua_State *L)
{
	Luae::check<Server::Ptr>(L, 1)->names(Luae::check<std::string>(L, 2));

	return 0;
}

int l_nick(lua_State *L)
{
	Luae::check<Server::Ptr>(L, 1)->nick(Luae::check<std::string>(L, 2));

	return 0;
}

int l_notice(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->notice(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_part(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);
	auto channel = Luae::check<std::string>(L, 2);
	std::string reason = "";

	if (Luae::gettop(L) >= 3)
		reason = Luae::check<std::string>(L, 3);

	s->part(channel, reason);

	return 0;
}

int l_query(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->query(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_say(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->say(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_send(lua_State *L)
{
	Luae::check<Server::Ptr>(L, 1)->send(Luae::check<std::string>(L, 2));

	return 0;
}

int l_topic(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);

	s->topic(Luae::check<std::string>(L, 2), Luae::check<std::string>(L, 3));

	return 0;
}

int l_umode(lua_State *L)
{
	Luae::check<Server::Ptr>(L, 1)->umode(Luae::check<std::string>(L, 2));

	return 0;
}

int l_whois(lua_State *L)
{
	Luae::check<Server::Ptr>(L, 1)->whois(Luae::check<std::string>(L, 2));

	return 0;
}

int l_tostring(lua_State *L)
{
	auto s = Luae::check<Server::Ptr>(L, 1);
	std::ostringstream oss;

	oss << "Server " << s->info().name;
	oss << " at " << s->info().host;

	if (s->options() & Server::OptionSsl)
		oss << " (using SSL)" << std::endl;

	Luae::push(L, oss.str());

	return 1;
}

int l_equals(lua_State *L)
{
	Luae::push(L, Luae::check<Server::Ptr>(L, 1) == Luae::check<Server::Ptr>(L, 2));

	return 1;
}

int l_gc(lua_State *L)
{
	return LuaeClass::deleteShared<Server>(L, 1);
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
	auto name = Luae::check<std::string>(L, 1);
	int ret;

	try {
		Server::Ptr server = Server::get(name);

		Luae::push(L, server);

		ret = 1;
	} catch (std::out_of_range ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		ret = 2;
	}

	return ret;
}

int l_connect(lua_State *L)
{
	Server::Info info;
	Server::Identity ident;
	Server::RetryInfo reco;
	unsigned options = 0;

	Luae::checktype(L, 1, LUA_TTABLE);

	info.name	= LuaeTable::require<std::string>(L, 1, "name");
	info.host	= LuaeTable::require<std::string>(L, 1, "host");
	info.port	= LuaeTable::require<int>(L, 1, "port");

	if (Server::has(info.name)) {
		Luae::push(L, false);
		Luae::pushfstring(L, "server %s already connected", info.name.c_str());

		return 2;
	}

	if (LuaeTable::type(L, 1, "password") == LUA_TSTRING)
		info.password = LuaeTable::require<std::string>(L, 1, "password");

	extractChannels(L, info);
	extractIdentity(L, ident);

	auto server = std::make_shared<Server>(info, ident, reco, options);
	Server::add(server);

	Luae::push(L, true);

	return 1;
}

const Luae::Reg functions {
	{ "find",		l_find				},
	{ "connect",		l_connect			},
};

}

const char *ServerType = "Server";

const char *Luae::IsUserdata<Server>::MetatableName = ServerType;

int luaopen_server(lua_State *L)
{
	Luae::newlib(L, functions);

	LuaeClass::create(L, serverDef);

	return 1;
}

} // !irccd
