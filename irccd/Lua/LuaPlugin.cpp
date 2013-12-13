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

#include "LuaPlugin.h"

#include "Plugin.h"
#include "Util.h"

namespace irccd {

namespace {

int l_addPath(lua_State *L)
{
	Plugin::addPath(luaL_checkstring(L, 1));

	return 0;
}

int l_info(lua_State *L)
{
	std::string name;
	int ret = 0;

	/*
	 * If the name is specified, search for a plugin, otherwise use
	 * ourselve.
	 */
	if (lua_gettop(L) >= 1) {
		name = luaL_checkstring(L, 1);

		try {
			auto plugin = Plugin::find(name);
			auto state = plugin->getState();

			lua_getfield(state, LUA_REGISTRYINDEX, Process::FieldInfo);
			LuaValue::push(L, LuaValue::copy(state, -1));
			lua_pop(state, 1);

			ret = 1;
		} catch (std::out_of_range ex) {
			lua_pushnil(L);
			lua_pushstring(L, ex.what());

			ret = 2;
		}
	} else {
		lua_getfield(L, LUA_REGISTRYINDEX, Process::FieldInfo);

		ret = 1;
	}

	return ret;
}

int l_list(lua_State *L)
{
	auto list = Plugin::list();
	auto i = 0;

	lua_createtable(L, list.size(), list.size());
	for (auto s : list) {
		lua_pushlstring(L, s.c_str(), s.length());
		lua_rawseti(L, -2, ++i);
	}

	return 1;
}

int l_load(lua_State *L)
{
	std::string path = luaL_checkstring(L, 1);

	try {
		Plugin::load(path, Util::isAbsolute(path));
	} catch (std::runtime_error error) {
		lua_pushnil(L);
		lua_pushfstring(L, "plugin: %s", error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_reload(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);

	Plugin::reload(name);

	return 0;
}

int l_unload(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);

	Plugin::unload(name);

	return 0;
}

const luaL_Reg functionList[] = {
	{ "addPath",		l_addPath		},
	{ "info",		l_info			},
	{ "list",		l_list			},
	{ "load",		l_load			},
	{ "reload",		l_reload		},
	{ "unload",		l_unload		},
	{ nullptr,		nullptr			}
};

}

int luaopen_plugin(lua_State *L)
{
	luaL_newlib(L, functionList);

	return 1;
}

} // !irccd
