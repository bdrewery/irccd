/*
 * LuaPlugin.cpp -- Lua bindings for class Plugin
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#if 0

#include <common/Logger.h>
#include <common/Util.h>

#include <irccd/Luae.h>
#include <irccd/Plugin.h>
#include <irccd/PluginManager.h>

#include "LuaPlugin.h"

namespace irccd {

namespace {

int l_addPath(lua_State *L)
{
	PluginManager::instance().addPath(Luae::check<std::string>(L, 1));

	return 0;
}

int l_info(lua_State *L)
{
	int ret = 0;

	/*
	 * If the name is specified, search for a plugin, otherwise use
	 * ourselve.
	 */
	if (Luae::gettop(L) >= 1) {
		auto name = Luae::check<std::string>(L, 1);

		try {
			auto plugin = PluginManager::instance().find(name);
			auto state = plugin->getState();

			Luae::getfield(state, LUA_REGISTRYINDEX, Process::FieldInfo);
			LuaeValue::push(L, LuaeValue::copy(state, -1));
			Luae::pop(state);

			ret = 1;
		} catch (const std::exception &ex) {
			Luae::push(L, nullptr);
			Luae::push(L, ex.what());

			ret = 2;
		}
	} else {
		Luae::getfield(L, LUA_REGISTRYINDEX, Process::FieldInfo);

		ret = 1;
	}

	return ret;
}

int l_list(lua_State *L)
{
	auto list = PluginManager::instance().list();
	auto i = 0;

	/*
	 * Iterator function. Users may call in the following way:
	 *
	 * for p in plugin.list()
	 */
	auto iterator = [] (lua_State *L) -> int {
		auto i = Luae::get<int>(L, Luae::upvalueindex(2));
		auto length = Luae::get<int>(L, Luae::upvalueindex(3));

		if (i - 1 == length)
			return 0;

		// Push the current value
		Luae::push(L, i);
		Luae::gettable(L, Luae::upvalueindex(1));

		// Update i
		Luae::push(L, ++i);
		Luae::replace(L, Luae::upvalueindex(2));

		return 1;
	};

	// Create a Lua table as upvalue to keep the list.
	LuaeTable::create(L, list.size(), list.size());
	for (auto s : list) {
		Luae::push(L, s);
		Luae::rawset(L, -2, ++i);
	}

	Luae::push(L, 1);
	Luae::push(L, static_cast<int>(list.size()));
	Luae::pushfunction(L, iterator, 3);

	return 1;
}

int l_load(lua_State *L)
{
	auto path = Luae::check<std::string>(L, 1);

	try {
		PluginManager::instance().load(path, Util::isAbsolute(path));
	} catch (const std::exception &error) {
		Luae::push(L, nullptr);
		Luae::pushfstring(L, "plugin: %s", error.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_reload(lua_State *L)
{
	PluginManager::instance().reload(Luae::check<std::string>(L, 1));

	return 0;
}

int l_unload(lua_State *L)
{
	PluginManager::instance().unload(Luae::check<std::string>(L, 1));

	return 0;
}

const Luae::Reg functionList {
	{ "addPath",		l_addPath		},
	{ "info",		l_info			},
	{ "list",		l_list			},
	{ "load",		l_load			},
	{ "reload",		l_reload		},
	{ "unload",		l_unload		}
};

} // !namespace

int luaopen_plugin(lua_State *L)
{
	Luae::newlib(L, functionList);

	return 1;
}

} // !irccd

#endif