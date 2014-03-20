/*
 * LuaUtf8.cpp -- Lua bindings for class Utf8
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

#include "Luae.h"
#include "LuaUtf8.h"
#include "Utf8.h"

namespace irccd {

namespace {

int pushArray(lua_State *L, int index)
{
	std::string str = luaL_checkstring(L, index);
	std::u32string result;

	try {
		result = Utf8::toucs(str);
	} catch (std::invalid_argument error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_createtable(L, 0, 0);

	for (size_t i = 0; i < result.size(); ++i) {
		lua_pushinteger(L, result[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int iterator(lua_State *L)
{
	auto i = lua_tointeger(L, lua_upvalueindex(2));
	auto length = lua_rawlen(L, lua_upvalueindex(1));

	if (i - 1 == static_cast<int>(length))
		return 0;

	lua_rawgeti(L, lua_upvalueindex(1), i);
	auto value = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushinteger(L, ++i);
	lua_replace(L, lua_upvalueindex(2));
	lua_pushinteger(L, value);

	return 1;
}

int l_length(lua_State *L)
{
	auto str = luaL_checkstring(L, 1);

	try {
		lua_pushinteger(L, Utf8::length(str));
	} catch (std::invalid_argument error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	return 1;
}

int l_tostring(lua_State *L)
{
	std::u32string array;
	std::string result;

	if (lua_type(L, 1) == LUA_TTABLE) {
		luaL_checktype(L, 1, LUA_TTABLE);

		LuaeTable::read(L, 1, [&] (lua_State *, int tkey, int tvalue) {
			if (tkey != LUA_TNUMBER || tvalue != LUA_TNUMBER)
				luaL_error(L, "invalid UCS-4 string");

			array.push_back(lua_tointeger(L, -1));
		});
	} else if (lua_type(L, 1) == LUA_TNUMBER) {
		array.push_back(luaL_checkinteger(L, 1));
	} else
		return luaL_error(L, "expected a table or number");

	try {
		result = Utf8::toutf8(array);
	} catch (std::invalid_argument error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushlstring(L, result.c_str(), result.length());

	return 1;
}

int l_toarray(lua_State *L)
{
	return pushArray(L, 1);
}

int l_list(lua_State *L)
{
	if (pushArray(L, 1) == 2)
		return 2;

	lua_pushinteger(L, 1);
	lua_pushcclosure(L, iterator, 2);

	return 1;
}

const luaL_Reg functions[] = {
	{ "length",		l_length	},
	{ "tostring",		l_tostring	},
	{ "toarray",		l_toarray	},
	{ "list",		l_list		},
	{ nullptr,		nullptr		}
};

}

int luaopen_utf8(lua_State *L)
{
	luaL_newlib(L, functions);

	return 1;
}

} // !irccd