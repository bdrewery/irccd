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

#define DATE_TYPE	"DateType"
#define DIR_TYPE	"DirectoryType"

#include <Date.h>
#include <Directory.h>
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

static int getTicks(lua_State *L)
{
	lua_pushinteger(L, Util::getTicks());

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

static int openDir(lua_State *L)
{
	Directory **ptr, *d;
	string path;
	bool skipParents = false;

	path = luaL_checkstring(L, 1);

	// Optional boolean
	if (lua_gettop(L) >= 2) {
	       	luaL_checktype(L, 2, LUA_TBOOLEAN);
		skipParents = lua_toboolean(L, 2);
	}

	d = new Directory(path);
	if (!d->open(skipParents)) {
		lua_pushnil(L);
		lua_pushstring(L, d->getError().c_str());

		return 2;
	}

	ptr = (Directory **)lua_newuserdata(L, sizeof (Directory *));
	luaL_setmetatable(L, DIR_TYPE);
	*ptr = d;

	return 1;
}

} // !util

const luaL_Reg functions[] = {
	{ "basename",		util::basename		},
	{ "dateNow",		util::dateNow		},
	{ "dirname",		util::dirname		},
	{ "exist",		util::exist		},
	{ "getHome",		util::getHome		},
	{ "getTicks",		util::getTicks		},
	{ "mkdir",		util::mkdir		},
	{ "openDir",		util::openDir		},
	{ nullptr,		nullptr			}
};

/* --------------------------------------------------------
 * Date methods
 * -------------------------------------------------------- */

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
	Date *date;
	time_t stamp;
	struct tm tm;

	date = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);
	stamp = date->getTimestamp();
	tm = *localtime(&stamp);

	// Create the table result
	lua_createtable(L, 8, 8);

	lua_pushinteger(L, tm.tm_sec);
	lua_setfield(L, -2, "seconds");

	lua_pushinteger(L, tm.tm_min);
	lua_setfield(L, -2, "minutes");

	lua_pushinteger(L, tm.tm_hour);
	lua_setfield(L, -2, "hours");

	lua_pushinteger(L, tm.tm_mon + 1);
	lua_setfield(L, -2, "month");

	lua_pushinteger(L, tm.tm_year + 1900);
	lua_setfield(L, -2, "year");

	return 1;
}

} // !dateMethods

/* --------------------------------------------------------
 * Date meta methods
 * -------------------------------------------------------- */

namespace dateMt {

static int equals(lua_State *L)
{
	Date *d1, *d2;

	d1 = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);
	d2 = *(Date **)luaL_checkudata(L, 2, DATE_TYPE);

	lua_pushboolean(L, *d1 == *d2);

	return 1;
}

static int le(lua_State *L)
{
	Date *d1, *d2;

	d1 = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);
	d2 = *(Date **)luaL_checkudata(L, 2, DATE_TYPE);

	lua_pushboolean(L, *d1 <= *d2);

	return 1;
}

static int tostring(lua_State *L)
{
	Date *date;

	date = *(Date **)luaL_checkudata(L, 1, DATE_TYPE);
	lua_pushfstring(L, "%d", date->getTimestamp());

	return 1;
}

} // !dateMt

/* --------------------------------------------------------
 * Directory methods
 * -------------------------------------------------------- */

namespace dir {

static int iter(lua_State *L)
{
	Directory *d;
	int idx;

	d = (Directory *)lua_topointer(L, lua_upvalueindex(1));
	idx = lua_tointeger(L, lua_upvalueindex(2));

	// End
	if ((size_t)idx >= d->getEntries().size()) {
		delete d;
		return 0;
	}

	// Push name + isDirectory
	lua_pushstring(L, d->getEntries()[idx].m_name.c_str());
	lua_pushboolean(L, d->getEntries()[idx].m_isDirectory);

	lua_pushinteger(L, ++idx);
	lua_replace(L, lua_upvalueindex(2));

	return 2;
}

static int count(lua_State *L)
{
	Directory *d;

	d = *(Directory **)luaL_checkudata(L, 1, DIR_TYPE);
	lua_pushinteger(L, d->getEntries().size());

	return 1;
}

static int read(lua_State *L)
{
	Directory *d, *copy;

	d = *(Directory **)luaL_checkudata(L, 1, DIR_TYPE);

	copy = new Directory(*d);
	lua_pushlightuserdata(L, copy);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, iter, 2);

	return 1;
}

} // !dir

/* --------------------------------------------------------
 * Directory metamethods
 * -------------------------------------------------------- */

namespace dirMt {

static int eq(lua_State *L)
{
	Directory *d1, *d2;

	d1 = *(Directory **)luaL_checkudata(L, 1, DIR_TYPE);
	d2 = *(Directory **)luaL_checkudata(L, 2, DIR_TYPE);

	lua_pushboolean(L, *d1 == *d2);

	return 1;
}

static int gc(lua_State *L)
{
	Directory *d;

	d = *(Directory **)luaL_checkudata(L, 1, DIR_TYPE);
	delete d;

	return 0;
}

static int tostring(lua_State *L)
{
	Directory *d;

	d = *(Directory **)luaL_checkudata(L, 1, DIR_TYPE);
	lua_pushfstring(L, "Directory %s has %d entries", d->getPath().c_str(),
	    d->getEntries().size());

	return 1;
}

} // !dirMt

static const luaL_Reg dateMethodsList[] = {
	{ "format",		date::format		},
	{ "getCalendar",	date::getCalendar	},
	{ nullptr,		nullptr			}
};

static const luaL_Reg dateMtList[] = {
	{ "__eq",		dateMt::equals		},
	{ "__le",		dateMt::le		},
	{ "__tostring",		dateMt::tostring	},
	{ nullptr,		nullptr			}
};

static const luaL_Reg dirMethodsList[] = {
	{ "count",		dir::count		},
	{ "read",		dir::read		},
	{ nullptr,		nullptr			}
};

static const luaL_Reg dirMtList[] = {
	{ "__eq",		dirMt::eq		},
	{ "__gc",		dirMt::gc		},
	{ "__tostring",		dirMt::tostring		},
	{ nullptr,		nullptr			}
};

int irccd::luaopen_util(lua_State *L)
{
	// Util library
	luaL_newlib(L, functions);

	// Date type
	luaL_newmetatable(L, DATE_TYPE);
	luaL_setfuncs(L, dateMtList, 0);
	luaL_newlib(L, dateMethodsList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// Directory type
	luaL_newmetatable(L, DIR_TYPE);
	luaL_setfuncs(L, dirMtList, 0);
	luaL_newlib(L, dirMethodsList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}
