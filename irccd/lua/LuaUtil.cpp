/*
 * LuaUtil.cpp -- Lua bindings for class Util
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

std::unordered_map<std::string, Color> colors {
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

std::unordered_map<std::string, Attribute> attributes {
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

	return 1;
}

} // !util

const luaL_Reg functions[] = {
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
