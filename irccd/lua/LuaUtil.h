/*
 * LuaUtil.h -- Lua bindings for class Util
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

#ifndef _IRCCD_LUA_UTIL_H_
#define _IRCCD_LUA_UTIL_H_

/**
 * @file LuaUtil.h
 * @brief Lua bindings for class Util
 */

#include <common/Date.h>
#include <irccd/Luae.h>

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
struct Luae::IsUserdata<Date> : std::true_type {
	/**
	 * The metatable name.
	 */
	static const char *MetatableName;
};

/**
 * The open function.
 *
 * @param L the Lua state
 * @return the number of args pushed
 */
int luaopen_util(lua_State *L);

} // !irccd

#endif // !_IRCCD_LUA_UTIL_H_
