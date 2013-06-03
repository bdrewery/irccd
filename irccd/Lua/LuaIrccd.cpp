/*
 * LuaIrccd.cpp -- Lua bindings for class Irccd
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

#include "Irccd.h"
#include "LuaIrccd.h"

using namespace irccd;
using namespace std;

int irccd::luaopen_irccd(lua_State *L)
{
	// Use a standard table, for the moment there is no function
	lua_createtable(L, 3, 3);

	lua_pushinteger(L, MAJOR);
	lua_setfield(L, -2, "VERSION_MAJOR");

	lua_pushinteger(L, MINOR);
	lua_setfield(L, -2, "VERSION_MINOR");

	lua_pushfstring(L, "%d.%d", MAJOR, MINOR);
	lua_setfield(L, -2, "VERSION");

	return 1;
}
