/*
 * JsUtil.cpp -- utilities for irccd util JS API
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

#include <cstring>
#include <initializer_list>
#include <unordered_map>
#include <sstream>

#include <Date.h>
#include <Util.h>
#include <Irccd.h>

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

const duk_number_list_entry colors[] {
	{ "ColorWhite",		static_cast<int>(Color::White)			},
	{ "ColorBlack",		static_cast<int>(Color::Black)			},
	{ "ColorBlue",		static_cast<int>(Color::Blue)			},
	{ "ColorGreen",		static_cast<int>(Color::Green)			},
	{ "ColorRed",		static_cast<int>(Color::Red)			},
	{ "ColorBrown",		static_cast<int>(Color::Brown)			},
	{ "ColorPurple",	static_cast<int>(Color::Purple)			},
	{ "ColorOrange",	static_cast<int>(Color::Orange)			},
	{ "ColorYellow",	static_cast<int>(Color::Yellow)			},
	{ "ColorLightGreen",	static_cast<int>(Color::LightGreen)		},
	{ "ColorCyan",		static_cast<int>(Color::Cyan)			},
	{ "ColorLightCyan",	static_cast<int>(Color::LightCyan)		},
	{ "ColorLightBlue",	static_cast<int>(Color::LightBlue)		},
	{ "ColorPink",		static_cast<int>(Color::Pink)			},
	{ "ColorGrey",		static_cast<int>(Color::Grey)			},
	{ "ColorLightGrey",	static_cast<int>(Color::LightGrey)		},
	{ nullptr,		0						}
};

const duk_number_list_entry attributes[] {
	{ "AttrBold",		static_cast<int>(Attribute::Bold)		},
	{ "AttrColor",		static_cast<int>(Attribute::Color)		},
	{ "AttrItalic",		static_cast<int>(Attribute::Italic)		},
	{ "AttrStrikeThrough",	static_cast<int>(Attribute::StrikeThrough)	},
	{ "AttrReset",		static_cast<int>(Attribute::Reset)		},
	{ "AttrUnderline",	static_cast<int>(Attribute::Underline)		},
	{ "AttrUnderline2",	static_cast<int>(Attribute::Underline2)		},
	{ "AttrReverse",	static_cast<int>(Attribute::Reverse)		},
	{ nullptr,		0						}
};

const duk_number_list_entry convertFlags[] {
	{ "ConvertEnv",		Util::ConvertEnv				},
	{ "ConvertDate",	Util::ConvertDate				},
	{ "ConvertHome",	Util::ConvertHome				},
	{ nullptr,		0						}
};

/* --------------------------------------------------------
 * Date object
 * -------------------------------------------------------- */

/*
 * Method: Date.calendar()
 * --------------------------------------------------------
 *
 * Returns a table containing the calendar information with the
 * following fields:
 *
 *   - seconds, the number of seconds
 *   - minutes, the number of minutes
 *   - hours, the number of hours
 *   - month, the month number (1 = January)
 *   - year, the year
 *   - dayOfMonth, day number from month (1-31)
 *   - dayOfWeek, day of week (0 = Sunday)
 *   - dayOfYear, day in the year (0-365)
 *
 * Returns:
 *   - The calendar information
 */
int Date_prototype_calendar(duk_context *ctx)
{
	dukx_with_this<Date>(ctx, [&] (const Date &date) {
		time_t timestamp = date.getTimestamp();
		struct tm *tm = localtime(&timestamp);

		duk_push_object(ctx);
		duk_push_int(ctx, static_cast<int>(tm->tm_sec));
		duk_put_prop_string(ctx, -2, "seconds");
		duk_push_int(ctx, static_cast<int>(tm->tm_min));
		duk_put_prop_string(ctx, -2, "minutes");
		duk_push_int(ctx, static_cast<int>(tm->tm_hour));
		duk_put_prop_string(ctx, -2, "hours");
		duk_push_int(ctx, static_cast<int>(tm->tm_mon + 1));
		duk_put_prop_string(ctx, -2, "month");
		duk_push_int(ctx, static_cast<int>(tm->tm_year + 1900));
		duk_put_prop_string(ctx, -2, "year");
		duk_push_int(ctx, static_cast<int>(tm->tm_mday));
		duk_put_prop_string(ctx, -2, "dayOfMonth");
		duk_push_int(ctx, static_cast<int>(tm->tm_wday));
		duk_put_prop_string(ctx, -2, "dayOfWeek");
		duk_push_int(ctx, static_cast<int>(tm->tm_yday));
		duk_put_prop_string(ctx, -2, "dayOfYear");
	});

	return 1;
}

int Date_prototype_format(duk_context *ctx)
{
	dukx_with_this<Date>(ctx, [&] (const Date &date) {
		duk_push_string(ctx, date.format(duk_require_string(ctx, 0)).c_str());
	});

	return 1;
}

int Date_prototype_toString(duk_context *ctx)
{
	dukx_with_this<Date>(ctx, [&] (const Date &date) {
		duk_push_string(ctx, std::to_string(date.getTimestamp()).c_str());
	});

	return 1;
}

const duk_function_list_entry dateMethods[] = {
	/* Methods */
	{ "calendar",	Date_prototype_calendar,	0	},
	{ "format",	Date_prototype_format,		1	},

	/* Special */
	{ "toString",	Date_prototype_toString,	0	},
	{ nullptr,	nullptr,			0	}
};

/* -------------------------------------------------------
 * Util functions
 * ------------------------------------------------------- */

/*
 * Function: Date(timestamp = undefined) [constructor]
 * --------------------------------------------------------
 *
 * Construct a new Date object.
 *
 * Arguments:
 *   - timestamp, the optional timestamp, if undefined current time
 */
duk_ret_t Util_Date(duk_context *ctx)
{
	if (duk_get_top(ctx) == 1) {
		dukx_set_class(ctx, new Date(duk_require_int(ctx, 0)));
	} else {
		dukx_set_class(ctx, new Date);
	}

	return 0;
}

duk_ret_t Util_convert(duk_context *)
{
	// TODO: common pattern have changed
#if 0
	const char *line = duk_require_string(L, 1);
	int flags = 0;
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
#endif
	return 0;
}

duk_ret_t Util_format(duk_context *)
{
#if 0
	std::string text = Luae::check<std::string>(L, 1);
	std::ostringstream oss;

	Luae::checktype(L, 2, LUA_TTABLE);

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

	Luae::pop(L);

	oss << text << static_cast<char>(Attribute::Reset);
	Luae::push(L, oss.str());

	return 1;
#endif
	return 0;
}

duk_ret_t Util_split(duk_context *)
{
#if 0
	auto str = Luae::check<std::string>(L, 1);
	auto delim = Luae::check<std::string>(L, 2);
	auto max = -1;

	if (Luae::gettop(L) >= 3)
		max = Luae::check<int>(L, 3);

	Luae::push(L, Util::split(str, delim, max));

	return 1;
#endif
	return 0;
}

duk_ret_t Util_splituser(duk_context *ctx)
{
	const char *target = duk_require_string(ctx, 0);
	char nick[32] = {0};

	irc_target_get_nick(target, nick, sizeof (nick) -1);
	duk_push_string(ctx, nick);

	return 1;
}

duk_ret_t Util_splithost(duk_context *ctx)
{
	const char *target = duk_require_string(ctx, 0);
	char host[32] = {0};

	irc_target_get_host(target, host, sizeof (host) -1);
	duk_push_string(ctx, host);

	return 1;
}

duk_ret_t Util_strip(duk_context *ctx)
{
	dukx_assert_begin(ctx);
	duk_push_string(ctx, Util::strip(duk_require_string(ctx, 0)).c_str());
	dukx_assert_end(ctx, 1);

	return 1;
}

const duk_function_list_entry utilFunctions[] = {
	{ "convert",		Util_convert,	1	},
	{ "format",		Util_format,	1	},
	{ "split",		Util_split,	1	},
	{ "splituser",		Util_splituser,	0	},
	{ "splithost",		Util_splithost,	0	},
	{ "strip",		Util_strip,	1	},
	{ nullptr,		nullptr,	0	},
};

} // !namespace

duk_ret_t dukopen_util(duk_context *ctx) noexcept
{
	dukx_assert_begin(ctx);
	duk_push_object(ctx);

	/* Util "class" */
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, utilFunctions);
	duk_put_number_list(ctx, -1, colors);
	duk_put_number_list(ctx, -1, attributes);
	duk_put_number_list(ctx, -1, convertFlags);
	duk_put_prop_string(ctx, -2, "Util");

	/* Date */
	duk_push_c_function(ctx, Util_Date, DUK_VARARGS);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, dateMethods);
	duk_put_prop_string(ctx, -2, "prototype");
	duk_put_prop_string(ctx, -2, "Date");

	dukx_assert_end(ctx, 1);

	return 1;
}

} // !irccd
