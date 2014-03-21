/*
 * LuaUtil.h -- Lua bindings for class Util
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

#ifndef _LUA_UTIL_H_
#define _LUA_UTIL_H_

/**
 * @file LuaUtil.h
 * @brief Lua bindings for class Util
 */

#include "Date.h"
#include "Luae.h"

namespace irccd {

/**
 * The date type.
 */
extern const char *DateType;

/**
 * @brief Overload for Date
 *
 * The object is used as userdata.
 */
template <>
struct Luae::Convert<Date> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the date.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const Date &value)
	{
		new (L, DateType) Date(value);
	}

	/**
	 * Get a date.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static Date *get(lua_State *L, int index)
	{
		return Luae::toType<Date *>(L, index);
	}

	/**
	 * Check for a date.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static Date *check(lua_State *L, int index)
	{
		return Luae::toType<Date *>(L, index, DateType);
	}
};

/**
 * The open function.
 *
 * @param L the Lua state
 * @return the number of args pushed
 */
int luaopen_util(lua_State *L);

} // !irccd

#endif // !_LUA_UTIL_H_
