/*
 * LuaLogger.cpp -- Lua bindings for class Logger
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

#include <Logger.h>

#include "Irccd.h"
#include "LuaLogger.h"

namespace irccd {

namespace {

std::string makeMessage(lua_State *L, const std::string &message)
{
	std::ostringstream oss;
	std::string name;

	name = Process::info(L).name;

	oss << "plugin " << name << ": " << message;

	return oss.str();
}

int l_debug(lua_State *L)
{
	Logger::debug("%s", makeMessage(L, Luae::check<std::string>(L, 1)).c_str());

	return 0;
}

int l_debugf(lua_State *L)
{
	Logger::debug("%s", makeMessage(L, Luae::format(L, 1)).c_str());

	return 0;
}

int l_log(lua_State *L)
{
	Logger::log("%s", makeMessage(L, Luae::check<std::string>(L, 1)).c_str());

	return 0;
}

int l_logf(lua_State *L)
{
	Logger::log("%s", makeMessage(L, Luae::format(L, 1)).c_str());

	return 0;
}

int l_warn(lua_State *L)
{
	Logger::warn("%s", makeMessage(L, Luae::check<std::string>(L, 1)).c_str());

	return 0;
}

int l_warnf(lua_State *L)
{
	Logger::warn("%s", makeMessage(L, Luae::format(L, 1)).c_str());

	return 0;
}

const Luae::Reg functions {
	{ "debug",	l_debug			},
	{ "debugf",	l_debugf		},
	{ "log",	l_log			},
	{ "logf",	l_logf			},
	{ "warn",	l_warn			},
	{ "warnf",	l_warnf			}
};

}

int luaopen_logger(lua_State *L)
{
	Luae::newlib(L, functions);

	return 1;
}

} // !irccd
