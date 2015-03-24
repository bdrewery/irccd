/*
 * LuaSystem.cpp -- Lua bindings for system information
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

#include <chrono>
#include <thread>

#include <common/Directory.h>

#include <irccd/Luae.h>
#include <irccd/System.h>

#include "LuaSystem.h"

namespace irccd {

namespace {

int l_name(lua_State *L)
{
	Luae::push(L, System::name());

	return 1;
}

int l_version(lua_State *L)
{
	Luae::push(L, System::version());

	return 1;
}

int l_uptime(lua_State *L)
{
	Luae::push(L, static_cast<int>(System::uptime()));

	return 1;
}

int l_sleep(lua_State *L)
{
	std::this_thread::sleep_for(std::chrono::seconds(Luae::check<int>(L, 1)));

	return 0;
}

int l_usleep(lua_State *L)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(Luae::check<int>(L, 1)));

	return 0;
}

int l_ticks(lua_State *L)
{
	Luae::push(L, static_cast<int>(System::ticks()));

	return 1;
}

int l_env(lua_State *L)
{
	auto name = Luae::check<std::string>(L, 1);
	auto value = System::env(name);

	Luae::push(L, value);

	return 1;
}

int l_home(lua_State *L)
{
	Luae::push(L, System::home());

	return 1;
}

const Luae::Reg functions {
	{ "name",		l_name		},
	{ "version",		l_version	},
	{ "uptime",		l_uptime	},
	{ "sleep",		l_sleep		},
	{ "usleep",		l_usleep	},
	{ "ticks",		l_ticks		},
	{ "env",		l_env		},
	{ "home",		l_home		}
};

}

int luaopen_system(lua_State *L)
{
	Luae::newlib(L, functions);

	return 1;
}

} // !irccd
