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
#include <Util.h>

#include "Irccd.h"
#include "LuaUtil.h"

namespace irccd {

namespace {

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

LuaeEnum::Def colors {
	{ "White",		static_cast<int>(Color::White)			},
	{ "Black",		static_cast<int>(Color::Black)			},
	{ "Blue",		static_cast<int>(Color::Blue)			},
	{ "Green",		static_cast<int>(Color::Green)			},
	{ "Red",		static_cast<int>(Color::Red)			},
	{ "Brown",		static_cast<int>(Color::Brown)			},
	{ "Purple",		static_cast<int>(Color::Purple)			},
	{ "Orange",		static_cast<int>(Color::Orange)			},
	{ "Yellow",		static_cast<int>(Color::Yellow)			},
	{ "LightGreen",		static_cast<int>(Color::LightGreen)		},
	{ "Cyan",		static_cast<int>(Color::Cyan)			},
	{ "LightCyan",		static_cast<int>(Color::LightCyan)		},
	{ "LightBlue",		static_cast<int>(Color::LightBlue)		},
	{ "Pink",		static_cast<int>(Color::Pink)			},
	{ "Grey",		static_cast<int>(Color::Grey)			},
	{ "LightGrey",		static_cast<int>(Color::LightGrey)		}
};

LuaeEnum::Def attributes {
	{ "Bold",		static_cast<int>(Attribute::Bold)		},
	{ "Color",		static_cast<int>(Attribute::Color)		},
	{ "Italic",		static_cast<int>(Attribute::Italic)		},
	{ "StrikeThrough",	static_cast<int>(Attribute::StrikeThrough)	},
	{ "Reset",		static_cast<int>(Attribute::Reset)		},
	{ "Underline",		static_cast<int>(Attribute::Underline)		},
	{ "Underline2",		static_cast<int>(Attribute::Underline2)		},
	{ "Reverse",		static_cast<int>(Attribute::Reverse)		}
};

LuaeEnum::Def convertFlags {
	{ "ConvertEnv",		Util::ConvertEnv				},
	{ "ConvertDate",	Util::ConvertDate				},
	{ "ConvertHome",	Util::ConvertHome				},
};

int l_convert(lua_State *L)
{
	auto line = Luae::check<std::string>(L, 1);
	auto flags = 0;
	Util::Args args;

	/* Parse table */
	Luae::checktype(L, 2, LUA_TTABLE);
	LuaeTable::read(L, 2, [&] (lua_State *L, int tkey, int tvalue) {
		if (tkey != LUA_TSTRING || tvalue != LUA_TSTRING)
			return;

		auto key = Luae::check<std::string>(L, -2);

		// Handle date in a special case
		if (key == "date")
			args.timestamp = Luae::check<int>(L, -1);
		else if (key.size() == 1)
			args.keywords[key[0]] = Luae::check<std::string>(L, -1);
	});

	if (Luae::gettop(L) >= 3)
		flags = LuaeEnum::get(L, 3);

	Luae::push(L, Util::convert(line, args, flags));

	return 1;
}

int l_date(lua_State *L)
{
	if (Luae::gettop(L) >= 1)
		Luae::push(L, Date(Luae::get<int>(L, 1)));
	else
		Luae::push(L, Date());

	return 1;
}

int l_format(lua_State *L)
{
	std::string text = Luae::check<std::string>(L, 1);
	std::ostringstream oss;

	luaL_checktype(L, 2, LUA_TTABLE);

	/*
	 * First, if "fg" or "bg" field is defined we append the color
	 * escape code and then the background or the foreground
	 * without testing.
	 */
	if (LuaeTable::type(L, 2, "fg") != LUA_TNIL ||
	    LuaeTable::type(L, 2, "bg") != LUA_TNIL)
		oss << static_cast<char>(Attribute::Color);

	if (LuaeTable::type(L, 2, "fg") != LUA_TNIL)
		oss << LuaeTable::get<int>(L, 2, "fg");
	if (LuaeTable::type(L, 2, "bg") != LUA_TNIL)
		oss << ',' << LuaeTable::get<int>(L, 2, "bg");

	/*
	 * Attributes can be a table or a single attribute. If it's a table
	 * we iterate it and add every attributes.
	 */
	Luae::getfield(L, 2, "attrs");

	if (Luae::type(L, -1) == LUA_TTABLE) {
		int length = Luae::rawlen(L, -1);

		for (int i = 1; i <= length; ++i) {
			Luae::push(L, i);
			Luae::gettable(L, -2);

			oss << static_cast<char>(Luae::get<int>(L, -1));
			Luae::pop(L, 1);
		}
	} else if (Luae::type(L, -1) == LUA_TNUMBER)
		oss << static_cast<char>(Luae::get<int>(L, -1));

	Luae::pop(L, 1);

	oss << text << static_cast<char>(Attribute::Reset);
	Luae::push(L, oss.str());

	return 1;
}

int l_split(lua_State *L)
{
	auto str = Luae::check<std::string>(L, 1);
	auto delim = Luae::check<std::string>(L, 2);
	auto max = -1;

	if (Luae::gettop(L) >= 3)
		max = Luae::check<int>(L, 3);

	Luae::push(L, Util::split(str, delim, max));

	return 1;
}

int l_splituser(lua_State *L)
{
	auto target = Luae::check<std::string>(L, 1);
	char nick[32];

	std::memset(nick, 0, sizeof (nick));

	irc_target_get_nick(target.c_str(), nick, sizeof (nick) -1);
	Luae::push(L, nick);

	return 1;
}

int l_splithost(lua_State *L)
{
	auto target = Luae::check<std::string>(L, 1);
	char host[32];

	std::memset(host, 0, sizeof (host));

	irc_target_get_host(target.c_str(), host, sizeof (host) -1);
	Luae::push(L, host);

	return 1;
}

int l_strip(lua_State *L)
{
	auto value = Luae::check<std::string>(L, 1);

	Luae::push(L, Util::strip(value));

	return 1;
}

const luaL_Reg functions[] = {
	{ "convert",		l_convert		},
	{ "date",		l_date			},
	{ "format",		l_format		},
	{ "split",		l_split			},
	{ "splituser",		l_splituser		},
	{ "splithost",		l_splithost		},
	{ "strip",		l_strip			},
	{ nullptr,		nullptr			}
};

/* --------------------------------------------------------
 * Date methods
 * -------------------------------------------------------- */

int l_dateCalendar(lua_State *L)
{
	auto date = Luae::get<Date>(L, 1);
	auto stamp = date->getTimestamp();
	auto tm = *localtime(&stamp);

	// Create the table result
	LuaeTable::create(L, 0, 8);

	LuaeTable::set(L, -1, "seconds", static_cast<int>(tm.tm_sec));
	LuaeTable::set(L, -1, "minutes", static_cast<int>(tm.tm_min));
	LuaeTable::set(L, -1, "hours", static_cast<int>(tm.tm_hour));
	LuaeTable::set(L, -1, "month", static_cast<int>(tm.tm_mon + 1));
	LuaeTable::set(L, -1, "year", static_cast<int>(tm.tm_year + 1900));
	LuaeTable::set(L, -1, "monthDay", static_cast<int>(tm.tm_mday));
	LuaeTable::set(L, -1, "weekDay", static_cast<int>(tm.tm_wday));
	LuaeTable::set(L, -1, "yearDay", static_cast<int>(tm.tm_yday));

	return 1;
}

int l_dateFormat(lua_State *L)
{
	// Extract parameters
	auto d = Luae::check<Date>(L, 1);
	auto fmt = Luae::check<std::string>(L, 2);

	Luae::push(L, d->format(fmt));

	return 1;
}

int l_dateTimestamp(lua_State *L)
{
	auto date = Luae::check<Date>(L, 1);

	Luae::push(L, static_cast<int>(date->getTimestamp()));

	return 1;
}

/* --------------------------------------------------------
 * Date meta methods
 * -------------------------------------------------------- */

int l_dateEquals(lua_State *L)
{
	auto d1 = Luae::check<Date>(L, 1);
	auto d2 = Luae::check<Date>(L, 2);

	Luae::push(L, *d1 == *d2);

	return 1;
}

int l_dateGc(lua_State *L)
{
	Luae::check<Date>(L, 1)->~Date();

	return 0;
}

int l_dateLe(lua_State *L)
{
	auto d1 = Luae::check<Date>(L, 1);
	auto d2 = Luae::check<Date>(L, 2);

	Luae::push(L, *d1 <= *d2);

	return 1;
}

int l_dateTostring(lua_State *L)
{
	auto date = Luae::check<Date>(L, 1);

	lua_pushfstring(L, "%d", date->getTimestamp());

	return 1;
}

const Luae::Reg dateMethodsList {
	{ "calendar",		l_dateCalendar		},
	{ "format",		l_dateFormat		},
	{ "timestamp",		l_dateTimestamp		}
};

const Luae::Reg dateMtList {
	{ "__eq",		l_dateEquals		},
	{ "__gc",		l_dateGc		},
	{ "__le",		l_dateLe		},
	{ "__tostring",		l_dateTostring		}
};

}

const char *DateType = "Date";

const char *Luae::IsUserdata<Date>::MetatableName = DateType;

int luaopen_util(lua_State *L)
{
	// Util library
	Luae::newlib(L, functions);

	// Date type
	Luae::newmetatable(L, DateType);
	Luae::setfuncs(L, dateMtList);
	Luae::newlib(L, dateMethodsList);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	// Colors
	LuaeEnum::create(L, colors, -1, "color");

	// Attributes
	LuaeEnum::create(L, attributes, -1, "attribute");

	// Conversion flags
	LuaeEnum::create(L, convertFlags, -1);

	return 1;
}

} // !irccd
