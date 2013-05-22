/*
 * LuaUtil.cpp -- Lua bindings for class Util
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#define DATE_TYPE "DateType"

#include <Date.h>
#include <Util.h>

#include "LuaUtil.h"

using namespace irccd;
using namespace std;

namespace util {

static int basename(lua_State *L)
{
	string path = luaL_checkstring(L, 1);
	string ret = Util::basename(path);

	lua_pushstring(L, ret.c_str());

	return 1;
}

static int dateNow(lua_State *L)
{
	Date **ptr;

	ptr = (Date **)lua_newuserdata(L, sizeof (Date *));
	luaL_setmetatable(L, DATE_TYPE);

	*ptr = new Date();

	return 1;
}

static int dirname(lua_State *L)
{
	string path = luaL_checkstring(L, 1);
	string ret = Util::dirname(path);

	lua_pushstring(L, ret.c_str());

	return 1;
}

static int exist(lua_State *L)
{
	string path = luaL_checkstring(L, 1);
	bool ret = Util::exist(path);

	lua_pushboolean(L, ret);

	return 1;
}

static int getHome(lua_State *L)
{
	lua_pushstring(L, Util::getHome().c_str());

	return 1;
}

static int mkdir(lua_State *L)
{
	int mode = 0700;
	string path;

	path = luaL_checkstring(L, 1);
	if (lua_gettop(L) >= 2)
		mode = luaL_checkinteger(L, 2);

	try {
		Util::mkdir(path, mode);
	} catch (Util::ErrorException ex) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "");

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

} // !util

const luaL_Reg functions[] = {
	{ "basename",		util::basename		},
	{ "dateNow",		util::dateNow		},
	{ "dirname",		util::dirname		},
	{ "exist",		util::exist		},
	{ "getHome",		util::getHome		},
	{ "mkdir",		util::mkdir		},
	{ nullptr,		nullptr			}
};

namespace date {

static int format(lua_State *L)
{
	Date *d;
	string fmt, result;

	// Extract parameters
	d = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);
	fmt = luaL_checkstring(L, 2);

	result = d->format(fmt);
	lua_pushstring(L, result.c_str());

	return 1;
}

static int getCalendar(lua_State *L)
{
	Date *d;

	d = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);

	// Create the table result
	lua_createtable(L, 8, 8);

	lua_pushinteger(L, d->m_tm.tm_sec);
	lua_setfield(L, -2, "seconds");

	lua_pushinteger(L, d->m_tm.tm_min);
	lua_setfield(L, -2, "minutes");

	lua_pushinteger(L, d->m_tm.tm_hour);
	lua_setfield(L, -2, "hours");

	lua_pushinteger(L, d->m_tm.tm_mon + 1);
	lua_setfield(L, -2, "month");

	lua_pushinteger(L, d->m_tm.tm_year + 1900);
	lua_setfield(L, -2, "year");

	return 1;
}

} // !date

const luaL_Reg dateList[] = {
	{ "format",		date::format		},
	{ "getCalendar",	date::getCalendar	},
	{ nullptr,		nullptr			}
};

int irccd::luaopen_util(lua_State *L)
{
	luaL_newlib(L, functions);

	luaL_newmetatable(L, DATE_TYPE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, dateList, 0);
	lua_pop(L, 1);

	return 1;
}
