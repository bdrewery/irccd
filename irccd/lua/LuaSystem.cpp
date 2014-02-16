/*
 * LuaSystem.cpp -- Lua bindings for system information
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

#include <Directory.h>

#include "Luae.h"
#include "LuaSystem.h"
#include "System.h"

namespace irccd {

namespace {

int l_name(lua_State *L)
{
	auto name = System::name();

	lua_pushlstring(L, name.c_str(), name.length());

	return 1;
}

int l_version(lua_State *L)
{
	auto version = System::version();

	lua_pushlstring(L, version.c_str(), version.length());

	return 1;
}

int l_uptime(lua_State *L)
{
	lua_pushinteger(L, System::uptime());

	return 1;
}

int l_sleep(lua_State *L)
{
	auto seconds = luaL_checkinteger(L, 1);

	System::sleep(seconds);

	return 0;
}

int l_usleep(lua_State *L)
{
	auto ms = luaL_checkinteger(L, 1);

	System::usleep(ms);

	return 0;
}

int l_ticks(lua_State *L)
{
	lua_pushinteger(L, System::ticks());

	return 1;
}

int l_env(lua_State *L)
{
	auto name = luaL_checkstring(L, 1);
	auto value = System::env(name);

	lua_pushlstring(L, value.c_str(), value.length());

	return 1;
}

int l_home(lua_State *L)
{
	auto home = System::home();

	lua_pushlstring(L, home.c_str(), home.length());

	return 1;
}

const luaL_Reg functions[] = {
	{ "name",		l_name		},
	{ "version",		l_version	},
	{ "uptime",		l_uptime	},
	{ "sleep",		l_sleep		},
	{ "usleep",		l_usleep	},
	{ "ticks",		l_ticks		},
	{ "env",		l_env		},
	{ "home",		l_home		},
	{ nullptr,		nullptr		}
};

}

int luaopen_system(lua_State *L)
{
	luaL_newlib(L, functions);

	return 1;
}

} // !irccd
