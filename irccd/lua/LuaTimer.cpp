/*
 * LuaTimer.cpp -- Lua bindings for class Timer
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

#include <irccd/Luae.h>
#include <irccd/PluginManager.h>
#include <irccd/Plugin.h>
#include <irccd/Process.h>

#include "LuaTimer.h"

namespace irccd {

namespace {

int l_add(lua_State *L)
{
	auto type = static_cast<TimerType>(luaL_checkinteger(L, 1));
	auto delay = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	// Add a new timer to that process
	auto process = PluginManager::instance().find(Process::info(L).name)->process();

	// Copy that function
	lua_pushvalue(L, 3);
	auto reference = luaL_ref(L, LUA_REGISTRYINDEX);

	process->addTimer(type, delay, reference);

	return 0;
}

int l_clear(lua_State *L)
{
	PluginManager::instance().find(Process::info(L).name)->process()->clearTimers();

	return 0;
}

const Luae::Reg functions {
	{ "add",		l_add				},
	{ "clear",		l_clear				}
};

const LuaeEnum::Def timerType {
	{ "Single",	static_cast<int>(TimerType::Single)	},
	{ "Repeat",	static_cast<int>(TimerType::Repeat)	}
};

const LuaeEnum::Def timerRet {
	{ "Quit",	-1					}
};

} // !namespace

int luaopen_timer(lua_State *L)
{
	Luae::newlib(L, functions);

	LuaeEnum::create(L, timerType, -1, "type");
	LuaeEnum::create(L, timerRet, -1, "result");

	return 1;
}

} // !irccd
