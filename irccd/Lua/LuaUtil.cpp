/*
 * LuaUtil.cpp -- Lua bindings for class Util
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

#include <cstring>
#include <initializer_list>
#include <unordered_map>
#include <sstream>

#include <Date.h>
#include <Directory.h>
#include <Logger.h>
#include <System.h>
#include <Util.h>

#include "Irccd.h"
#include "LuaUtil.h"

#if defined(COMPAT_1_0)
#  define DIR_TYPE	"Directory"
#endif

namespace irccd {

namespace util {

/* --------------------------------------------------------
 * Color and attributes
 * -------------------------------------------------------- */

enum class Color {
	White		= 0,
	Black		= 1,
	Blue		= 2,
	Green		= 3,
	Red		= 4,
	Brown		= 5,
	Purple		= 6,
	Orange		= 7,
	Yellow		= 8,
	LightGreen	= 9,
	Cyan		= 10,
	LightCyan	= 11,
	LightBlue	= 12,
	Pink		= 13,
	Grey		= 14,
	LightGrey	= 15
};

enum class Attribute {
	Bold		= '\x02',
	Color		= '\x03',
	Italic		= '\x09',
	StrikeThrough	= '\x13',
	Reset		= '\x0f',
	Underline	= '\x15',
	Underline2	= '\x1f',
	Reverse		= '\x16'
};

std::unordered_map<std::string, Color> colors = {
	{ "White",		Color::White			},
	{ "Black",		Color::Black			},
	{ "Blue",		Color::Blue			},
	{ "Green",		Color::Green			},
	{ "Red",		Color::Red			},
	{ "Brown",		Color::Brown			},
	{ "Purple",		Color::Purple			},
	{ "Orange",		Color::Orange			},
	{ "Yellow",		Color::Yellow			},
	{ "LightGreen",		Color::LightGreen		},
	{ "Cyan",		Color::Cyan			},
	{ "LightCyan",		Color::LightCyan		},
	{ "LightBlue",		Color::LightBlue		},
	{ "Pink",		Color::Pink			},
	{ "Grey",		Color::Grey			},
	{ "LightGrey",		Color::LightGrey		}
};

std::unordered_map<std::string, Attribute> attributes = {
	{ "Bold",		Attribute::Bold			},
	{ "Color",		Attribute::Color		},
	{ "Italic",		Attribute::Italic		},
	{ "StrikeThrough",	Attribute::StrikeThrough	},
	{ "Reset",		Attribute::Reset		},
	{ "Underline",		Attribute::Underline		},
	{ "Underline2",		Attribute::Underline2		},
	{ "Reverse",		Attribute::Reverse		}
};

int date(lua_State *L)
{
	if (lua_gettop(L) >= 1) {
		int tm = luaL_checkinteger(L, 1);
		new (L, DATE_TYPE) Date(tm);
	} else
		new (L, DATE_TYPE) Date();

	return 1;
}

int format(lua_State *L)
{
	std::string text = luaL_checkstring(L, 1);
	std::ostringstream oss;

	luaL_checktype(L, 2, LUA_TTABLE);

	/*
	 * First, if "fg" or "bg" field is defined we append the color
	 * escape code and then the background or the foreground
	 * without testing.
	 */
	if (Luae::typeField(L, 2, "fg") != LUA_TNIL ||
	    Luae::typeField(L, 2, "bg") != LUA_TNIL)
		oss << static_cast<char>(Attribute::Color);

	if (Luae::typeField(L, 2, "fg") != LUA_TNIL)
		oss << Luae::getField<int>(L, 2, "fg");
	if (Luae::typeField(L, 2, "bg") != LUA_TNIL)
		oss << ',' << Luae::getField<int>(L, 2, "bg");

	/*
	 * Attributes can be a table or a single attribute. If it's a table
	 * we iterate it and add every attributes.
	 */
	lua_getfield(L, 2, "attrs");

	if (lua_type(L, -1) == LUA_TTABLE) {
		int length = lua_rawlen(L, -1);

		for (int i = 1; i <= length; ++i) {
			lua_pushinteger(L, i);
			lua_gettable(L, -2);

			oss << static_cast<char>(lua_tointeger(L, -1));
			lua_pop(L, 1);
		}
	} else if (lua_type(L, -1) == LUA_TNUMBER)
		oss << static_cast<char>(lua_tointeger(L, -1));

	lua_pop(L, 1);
	oss << text << static_cast<char>(Attribute::Reset);
	lua_pushstring(L, oss.str().c_str());

	return 1;
}

int splituser(lua_State *L)
{
	auto target = luaL_checkstring(L, 1);
	char nick[32];

	std::memset(nick, 0, sizeof (nick));

	irc_target_get_nick(target, nick, sizeof (nick) -1);
	lua_pushstring(L, nick);

	return 1;
}

int splithost(lua_State *L)
{
	auto target = luaL_checkstring(L, 1);
	char host[32];

	std::memset(host, 0, sizeof (host));

	irc_target_get_host(target, host, sizeof (host) -1);
	lua_pushstring(L, host);

	printf("HOST = %s\n", host);

	return 1;
}

#if defined(COMPAT_1_0)

void warn(lua_State *L, const char *func, const char *repl)
{
	auto name = Process::info(L).name;

	Logger::warn("plugin %s: `%s' is deprecated, please use `%s'",
		     name.c_str(), func, repl);
}

int basename(lua_State *L)
{
	warn(L, "util.basename", "fs.basename");

	std::string path = luaL_checkstring(L, 1);
	std::string ret = Util::baseName(path);

	lua_pushstring(L, ret.c_str());

	return 1;
}

int dirname(lua_State *L)
{
	warn(L, "util.dirname", "fs.dirname");

	std::string path = luaL_checkstring(L, 1);
	std::string ret = Util::dirName(path);

	lua_pushstring(L, ret.c_str());

	return 1;
}

int exist(lua_State *L)
{
	warn(L, "util.exist", "fs.exists");

	std::string path = luaL_checkstring(L, 1);
	bool ret = Util::exist(path);

	lua_pushboolean(L, ret);

	return 1;
}

int getEnv(lua_State *L)
{
	warn(L, "util.getEnv", "system.env");

	std::string var = luaL_checkstring(L, 1);
	char *value;

	if ((value = getenv(var.c_str())) != nullptr)
		lua_pushstring(L, value);
	else
		lua_pushstring(L, "");

	return 1;
}

int getHome(lua_State *L)
{
	warn(L, "util.getHome", "system.home");

	lua_pushstring(L, System::home().c_str());

	return 1;
}

int getTicks(lua_State *L)
{
	warn(L, "util.getTicks", "system.ticks");

	lua_pushinteger(L, static_cast<int>(System::ticks()));

	return 1;
}

int mkdir(lua_State *L)
{
	warn(L, "util.mkdir", "fs.mkdir");

	int mode = 0700;
	std::string path;

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

int opendir(lua_State *L)
{
	warn(L, "util.mkdir", "fs.opendir");

	std::string path;
	bool skipParents = false;

	path = luaL_checkstring(L, 1);

	// Optional boolean
	if (lua_gettop(L) >= 2) {
	       	luaL_checktype(L, 2, LUA_TBOOLEAN);
		skipParents = (lua_toboolean(L, 2) != 0);
	}

	Directory d(path);
	if (!d.open(skipParents)) {
		lua_pushnil(L);
		lua_pushstring(L, d.getError().c_str());

		return 2;
	}

	new (L, DIR_TYPE) Directory(d);

	return 1;
}

int splitUser(lua_State *L)
{
	warn(L, "util.splitUser", "util.splituser");

	char nick[64], host[128];
	const char* nickname;

	nickname = luaL_checkstring(L, 1);

	std::memset(nick, 0, sizeof (nick));
	std::memset(host, 0, sizeof (host));

	irc_target_get_nick(nickname, nick, sizeof (nick) - 1);
	irc_target_get_host(nickname, host, sizeof (host) - 1);

	lua_pushstring(L, nick);
	lua_pushstring(L, host);

	return 2;
}

int usleep(lua_State *L)
{
	warn(L, "util.usleep", "system.usleep");

	int msec = lua_tointeger(L, 1);

	System::usleep(msec);

	return 0;
}

#endif

} // !util

const luaL_Reg functions[] = {
/*
 * DEPRECATION:	1.1-002
 *
 * All the following functions have been moved to the irccd.system.
 */
#if defined(COMPAT_1_0)
	{ "getEnv",		util::getEnv		},
	{ "getTicks",		util::getTicks		},
	{ "getHome",		util::getHome		},
	{ "basename",		util::basename		},
	{ "dirname",		util::dirname		},
	{ "exist",		util::exist		},
	{ "mkdir",		util::mkdir		},
	{ "opendir",		util::opendir		},
	{ "splitUser",		util::splitUser		},
	{ "usleep",		util::usleep		},
#endif
	{ "date",		util::date		},
	{ "format",		util::format		},
	{ "splituser",		util::splituser		},
	{ "splithost",		util::splithost		},
	{ nullptr,		nullptr			}
};

/* --------------------------------------------------------
 * Date methods
 * -------------------------------------------------------- */

namespace date {

int calendar(lua_State *L)
{
	Date *date;
	time_t stamp;
	struct tm tm;

	date = Luae::toType<Date *>(L, 1, DATE_TYPE);
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

int format(lua_State *L)
{
	Date *d;
	std::string fmt, result;

	// Extract parameters
	d = Luae::toType<Date *>(L, 1, DATE_TYPE);
	fmt = luaL_checkstring(L, 2);

	result = d->format(fmt);
	lua_pushstring(L, result.c_str());

	return 1;
}

int timestamp(lua_State *L)
{
	Date *date;

	date = Luae::toType<Date *>(L, 1, DATE_TYPE);
	lua_pushinteger(L, (lua_Integer)date->getTimestamp());

	return 1;
}

} // !dateMethods

/* --------------------------------------------------------
 * Date meta methods
 * -------------------------------------------------------- */

namespace dateMt {

int equals(lua_State *L)
{
	Date *d1, *d2;

	d1 = Luae::toType<Date *>(L, 1, DATE_TYPE);
	d2 = Luae::toType<Date *>(L, 2, DATE_TYPE);

	lua_pushboolean(L, *d1 == *d2);

	return 1;
}

int gc(lua_State *L)
{
	Luae::toType<Date *>(L, 1, DATE_TYPE)->~Date();

	return 0;
}

int le(lua_State *L)
{
	Date *d1, *d2;

	d1 = Luae::toType<Date *>(L, 1, DATE_TYPE);
	d2 = Luae::toType<Date *>(L, 2, DATE_TYPE);

	lua_pushboolean(L, *d1 <= *d2);

	return 1;
}

int tostring(lua_State *L)
{
	Date *date;

	date = Luae::toType<Date *>(L, 1, DATE_TYPE);
	lua_pushfstring(L, "%d", date->getTimestamp());

	return 1;
}

} // !dateMt

const luaL_Reg dateMethodsList[] = {
	{ "calendar",		date::calendar		},
	{ "format",		date::format		},
	{ "timestamp",		date::timestamp		},
	{ nullptr,		nullptr			}
};

const luaL_Reg dateMtList[] = {
	{ "__eq",		dateMt::equals		},
	{ "__gc",		dateMt::gc		},
	{ "__le",		dateMt::le		},
	{ "__tostring",		dateMt::tostring	},
	{ nullptr,		nullptr			}
};


#if defined(COMPAT_1_0)

/* --------------------------------------------------------
 * Directory methods
 * -------------------------------------------------------- */

namespace dir {

int iter(lua_State *L)
{
	const Directory *d;
	int idx;

	d = reinterpret_cast<const Directory *>(lua_topointer(L, lua_upvalueindex(1)));
	idx = lua_tointeger(L, lua_upvalueindex(2));

	// End
	if ((size_t)idx >= d->getEntries().size())
		return 0;

	// Push name + isDirectory
	lua_pushstring(L, d->getEntries()[idx].m_name.c_str());
	lua_pushboolean(L, d->getEntries()[idx].m_isDirectory);

	lua_pushinteger(L, ++idx);
	lua_replace(L, lua_upvalueindex(2));

	return 2;
}

int count(lua_State *L)
{
	Directory *d;

	d = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	lua_pushinteger(L, d->getEntries().size());

	return 1;
}

int read(lua_State *L)
{
	Directory *d;

	d = Luae::toType<Directory *>(L, 1, DIR_TYPE);

	lua_pushlightuserdata(L, d);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, iter, 2);

	return 1;
}

} // !dir

/* --------------------------------------------------------
 * Directory metamethods
 * -------------------------------------------------------- */

namespace dirMt {

int eq(lua_State *L)
{
	Directory *d1, *d2;

	d1 = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	d2 = Luae::toType<Directory *>(L, 2, DIR_TYPE);

	lua_pushboolean(L, *d1 == *d2);

	return 1;
}

int gc(lua_State *L)
{
	Luae::toType<Directory *>(L, 1, DIR_TYPE)->~Directory();

	return 0;
}

int tostring(lua_State *L)
{
	Directory *d;

	d = Luae::toType<Directory *>(L, 1, DIR_TYPE);
	lua_pushfstring(L, "Directory %s has %d entries", d->getPath().c_str(),
	    d->getEntries().size());

	return 1;
}

} // !dirMt

const luaL_Reg dirMethodsList[] = {
	{ "count",		dir::count		},
	{ "read",		dir::read		},
	{ nullptr,		nullptr			}
};

const luaL_Reg dirMtList[] = {
	{ "__eq",		dirMt::eq		},
	{ "__gc",		dirMt::gc		},
	{ "__tostring",		dirMt::tostring		},
	{ nullptr,		nullptr			}
};

#endif

int luaopen_util(lua_State *L)
{
	// Util library
	luaL_newlib(L, functions);

	// Date type
	luaL_newmetatable(L, DATE_TYPE);
	luaL_setfuncs(L, dateMtList, 0);
	luaL_newlib(L, dateMethodsList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

#if defined(COMPAT_1_0)
	// Directory type
	luaL_newmetatable(L, DIR_TYPE);
	luaL_setfuncs(L, dirMtList, 0);
	luaL_newlib(L, dirMethodsList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
#endif

	// Colors
	lua_createtable(L, 0, 0);

	for (auto p : util::colors) {
		lua_pushinteger(L, static_cast<int>(p.second));
		lua_setfield(L, -2, p.first.c_str());
	}

	lua_setfield(L, -2, "color");

	// Attributes
	lua_createtable(L, 0, 0);

	for (auto p : util::attributes) {
		lua_pushinteger(L, static_cast<int>(p.second));
		lua_setfield(L, -2, p.first.c_str());
	}

	lua_setfield(L, -2, "attribute");

	return 1;
}

} // !irccd
