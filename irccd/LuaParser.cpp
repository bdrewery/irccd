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
#define SECTION_TYPE	"SectionType"
#define OPTION_TYPE	"OptionType"

#include <sstream>

#include <Logger.h>

#include "LuaParser.h"

using namespace irccd;
using namespace parser;
using namespace std;

int LuaParser::readTuning(lua_State *L, int idx)
{
	int tuning = 0;
	int rel = idx;

	if (lua_type(L, rel) == LUA_TTABLE) {
		lua_pushnil(L);

		if (rel < 0)
			--rel;

		while (lua_next(L, rel)) {
			if (lua_isnumber(L, -1))
				tuning |= lua_tointeger(L, -1);

			lua_pop(L, 1);
		}
	}

	return tuning;
}

void LuaParser::pushSection(lua_State *L, const Section &s)
{
	Section **ptr;
	Section *copy = new Section(s);

	ptr = (Section **)lua_newuserdata(L, sizeof (Section *));
	luaL_setmetatable(L, SECTION_TYPE);
	*ptr = copy;
}

// {{{ "Static" functions

namespace functions {

static int open(lua_State *L)
{
	Parser **ptr, *p;
	string path;
	int tuning = 0, ch = Parser::DEFAULT_COMMENT_CHAR;

	if (lua_gettop(L) < 1)
		return luaL_error(L, "open() requires at least 1 argument");

	// Get the flags
	path = luaL_checkstring(L, 1);
	if (lua_gettop(L) >= 2) {
		luaL_checktype(L, 2, LUA_TTABLE);
		tuning = LuaParser::readTuning(L, 2);
	}
	if (lua_gettop(L) >= 3)
		ch = luaL_checkstring(L, 1)[0];
		
	p = new Parser(path, tuning);
	if (!p->open()) {
		lua_pushnil(L);
		lua_pushstring(L, p->getError().c_str());

		delete p;
		return 2;
	}

	ptr = (Parser **)lua_newuserdata(L, sizeof (Parser *));
	luaL_setmetatable(L, PARSER_TYPE);
	*ptr = p;

	return 1;
}

} // }}} !functions

const luaL_Reg functionList[] = {
	{ "open",		functions::open		},
	{ nullptr,		nullptr			}
};

// "Parser" methods {{{

namespace config {

static int sectionIterator(lua_State *L)
{
	vector<Section> *array = *(vector<Section> **)lua_touserdata(L, lua_upvalueindex(1));
	int idx = lua_tonumber(L, lua_upvalueindex(2));

	if ((size_t)idx >= array->size()) {
		delete array;
		return 0;
	}

	// Push the current section.
	LuaParser::pushSection(L, (*array)[idx]);

	// Update the index for the next call
	lua_pushinteger(L, ++idx);
	lua_replace(L, lua_upvalueindex(2));

	return 1;
}

static int findSections(lua_State *L)
{
	Parser *p;
	string name;
	vector<Section> **ptr, *list;

	// Get the parameters for that functions
	p = *(Parser **)luaL_checkudata(L, 1, PARSER_TYPE);
	name = luaL_checkstring(L, 2);
	list = new vector<Section>(p->findSections(name));

	// Push the list of array and the current index to iterate one
	ptr = (vector<Section> **)lua_newuserdata(L, sizeof (vector<Section> *));
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, sectionIterator, 2);

	*ptr = list;

	return 1;
}

static int getSection(lua_State *L)
{
	Parser *p = *(Parser **)luaL_checkudata(L, 1, PARSER_TYPE);
	string name = luaL_checkstring(L, 2);
	int ret = 0;

	try {
		Section **ptr;
		Section *s = new Section(p->getSection(name));

		// Copy the section
		ptr = (Section **)lua_newuserdata(L, sizeof (Section *));
		luaL_setmetatable(L, SECTION_TYPE);
		*ptr = s;

		ret = 1;
	} catch (NotFoundException ex) {
		lua_pushnil(L);
		lua_pushfstring(L, "section %s not found", name.c_str());

		ret = 2;
	}

	return ret;
}

static int gc(lua_State *L)
{
	Parser *p = *(Parser **)luaL_checkudata(L, 1, PARSER_TYPE);

	delete p;

	return 0;
}

static int tostring(lua_State *L)
{
	Parser *p = *(Parser **)luaL_checkudata(L, 1, PARSER_TYPE);
	ostringstream oss;

	oss << *p;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

} // }}} !config

const luaL_Reg parserList[] = {
	{ "findSections",	config::findSections	},
	{ "getSection",		config::getSection	},
	{ "__gc",		config::gc		},
	{ "__tostring",		config::tostring	},
	{ nullptr,		nullptr			}
};

// "Section" methods {{{

namespace section {

static int getOptions(lua_State *L)
{
	Section *s = *(Section **)luaL_checkudata(L, 1, SECTION_TYPE);

	lua_createtable(L, s->getOptions().size(), s->getOptions().size());
	for (const Option &o : s->getOptions()) {
		lua_pushstring(L, o.m_key.c_str());
		lua_setfield(L, -2, o.m_value.c_str());
	}

	return 1;
}

static int findOption(lua_State *L)
{
	Section *s = *(Section **)luaL_checkudata(L, 1, SECTION_TYPE);
	string name = luaL_checkstring(L, 2);
	string value;

	if (!s->getOption<string>(name, value)) {
		lua_pushnil(L);
		lua_pushfstring(L, "option %s not found", name.c_str());
		return 2;
	}

	lua_pushstring(L, value.c_str());

	return 1;
}

static int gc(lua_State *L)
{
	Section *s = *(Section **)luaL_checkudata(L, 1, SECTION_TYPE);

	delete s;

	return 0;
}

static int tostring(lua_State *L)
{
	Section *s = *(Section **)luaL_checkudata(L, 1, SECTION_TYPE);
	ostringstream oss;

	oss << *s;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

} // }}} !section

const luaL_Reg sectionList[] = {
	{ "getOptions",		section::getOptions	},
	{ "findOption",		section::findOption	},
	{ "__gc",		section::gc		},
	{ "__tostring",		section::tostring	},
	{ nullptr,		nullptr			}
};

int irccd::luaopen_parser(lua_State *L)
{
	luaL_newlib(L, functionList);

	// Bind tuning enum
	lua_pushinteger(L, Parser::DisableRootSection);
	lua_setfield(L, -2, "DisableRootSection");

	lua_pushinteger(L, Parser::DisableRedefinition);
	lua_setfield(L, -2, "DisableRedefinition");

	lua_pushinteger(L, Parser::DisableVerbosity);
	lua_setfield(L, -2, "DisableVerbosity");

	// Create Parser type
	luaL_newmetatable(L, PARSER_TYPE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, parserList, 0);
	lua_pop(L, 1);

	// Create Section type
	luaL_newmetatable(L, SECTION_TYPE);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");
	luaL_setfuncs(L, sectionList, 0);
	lua_pop(L, 1);

	return 1;
}
