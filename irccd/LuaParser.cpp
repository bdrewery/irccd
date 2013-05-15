/*
 * LuaParser.cpp -- Lua bindings for class Parser
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

#define PARSER_TYPE	"ParserType"

#include <Logger.h>
#include <Parser.h>

#include "LuaParser.h"

using namespace irccd;
using namespace parser;
using namespace std;

namespace functions {

static int open(lua_State *L)
{
	Parser **ptr, *p;
	string path;

	if (lua_gettop(L) < 1) {
		lua_pushstring(L, "requires at least 1 argument");
		return 1;
	}

	// Get the flags
	path = luaL_checkstring(L, 1);

	p = new Parser(path);
	if (!p->open()) {
		lua_pushstring(L, p->getError().c_str());
		delete p;
		return 1;
	}

	ptr = (Parser **)lua_newuserdata(L, sizeof (Parser *));
	luaL_setmetatable(L, PARSER_TYPE);
	*ptr = p;

	return 0;
}

} // !functions

const luaL_Reg functionList[] = {
	{ "open",		functions::open		},
	{ nullptr,		nullptr			}
};

namespace methods {

static int findSections(lua_State *L)
{

	return 0;
}

} // !methods

const luaL_Reg methodList[] = {
	{ "findSections",	methods::findSections	},
	{ nullptr,		nullptr			}
};

int irccd::luaopen_parser(lua_State *L)
{
	luaL_newlib(L, functionList);
	luaL_newmetatable(L, PARSER_TYPE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, methodList, 0);
	lua_pop(L, 1);

	return 1;
}
