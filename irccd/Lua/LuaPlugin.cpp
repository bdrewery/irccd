/*
 * LuaPlugin.cpp -- Lua bindings for class Plugin
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

#include "Plugin.h"
#include "LuaPlugin.h"

namespace irccd {

namespace {

int getName(lua_State *L)
{
	auto name = Luae::requireField<std::string>(L,
	    LUA_REGISTRYINDEX, Plugin::FieldName);

	lua_pushlstring(L, name.c_str(), name.length());

	return 1;
}

int getHome(lua_State *L)
{
	auto home = Luae::requireField<std::string>(L,
	    LUA_REGISTRYINDEX, Plugin::FieldHome);

	lua_pushlstring(L, home.c_str(), home.length());

	return 1;
}

const luaL_Reg functionList[] = {
	{ "getName",		getName			},
	{ "getHome",		getHome			},
	{ nullptr,		nullptr			}
};

}

int luaopen_plugin(lua_State *L)
{
	luaL_newlib(L, functionList);

	return 1;
}

} // !irccd
