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

#include <sstream>

#include "Irccd.h"
#include "LuaParser.h"

namespace irccd
{

int LuaParser::readTuning(lua_State *L, int idx)
{
	int tuning = 0;
	int rel = idx;

	if (lua_type(L, rel) == LUA_TTABLE)
	{
		lua_pushnil(L);

		if (rel < 0)
			--rel;

		while (lua_next(L, rel))
		{
			if (lua_isnumber(L, -1))
				tuning |= lua_tointeger(L, -1);

			lua_pop(L, 1);
		}
	}

	return tuning;
}

LuaParser::LuaParser(const std::string &path, int tuning, char commentToken)
	: Parser(path, tuning, commentToken)
	, m_state(nullptr)
	, m_logRef(LUA_NOREF)
{
}

LuaParser::LuaParser()
{
}

LuaParser::~LuaParser()
{
}

void LuaParser::setState(lua_State *L)
{
	m_state = L;
}

int LuaParser::getLogRef() const
{
	return m_logRef;
}

void LuaParser::setLogRef(int logRef)
{
	m_logRef = logRef;
}

void LuaParser::log(int number, const std::string &section, const std::string &message)
{
	if (m_logRef == LUA_NOREF)
		Parser::log(number, section, message);
	else if (m_state != nullptr)
	{
		// Call the Lua ref function
		lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_logRef);
		lua_pushinteger(m_state, number);
		lua_pushstring(m_state, section.c_str());
		lua_pushstring(m_state, message.c_str());

		lua_call(m_state, 3, 0);
	}
}

namespace
{

int create(lua_State *L)
{
	LuaParser *p;
	std::string path;
	int tuning = 0, ch = LuaParser::DEFAULT_COMMENT_CHAR;

	if (lua_gettop(L) < 1)
		return luaL_error(L, "open() requires at least 1 argument");

	// Get the flags
	path = luaL_checkstring(L, 1);
	if (lua_gettop(L) >= 2)
	{
		luaL_checktype(L, 2, LUA_TTABLE);
		tuning = LuaParser::readTuning(L, 2);
	}
	if (lua_gettop(L) >= 3)
		ch = luaL_checkstring(L, 1)[0];
		
	p = new (L, PARSER_TYPE) LuaParser(path, tuning, ch);

	// Copy the state so I can call log() function
	p->setState(L);

	return 1;
}

const luaL_Reg functionList[] = {
	{ "new",		create			},
	{ nullptr,		nullptr			}
};

int sectionIterator(lua_State *L)
{
	std::vector<Section> *array = toType<std::vector<Section> *>(L, lua_upvalueindex(1));
	int idx = lua_tointeger(L, lua_upvalueindex(2));

	if ((size_t)idx >= array->size())
		return 0;

	// Push the current section.
	new (L, SECTION_TYPE) Section((*array)[idx]);

	// Update the index for the next call
	lua_pushinteger(L, ++idx);
	lua_replace(L, lua_upvalueindex(2));

	return 1;
}

int parserOpen(lua_State *L)
{
	LuaParser *p = toType<LuaParser *>(L, 1, PARSER_TYPE);

	if (!p->open())
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, p->getError().c_str());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int parserFindSections(lua_State *L)
{
	LuaParser *p;
	std::string name;

	// Get the parameters for that functions
	p = toType<LuaParser *>(L, 1, PARSER_TYPE);
	name = luaL_checkstring(L, 2);

	// Push the list of array and the current index to iterate one
	new (L) std::vector<Section>(p->findSections(name));
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, sectionIterator, 2);

	return 1;
}

int parserHasSection(lua_State *L)
{
	LuaParser *p;
	std::string name;

	p = toType<LuaParser *>(L, 1, PARSER_TYPE);
	name = luaL_checkstring(L, 2);

	lua_pushboolean(L, p->hasSection(name));

	return 1;
}

int parserGetSection(lua_State *L)
{
	LuaParser *p = toType<LuaParser *>(L, 1, PARSER_TYPE);
	std::string name = luaL_checkstring(L, 2);
	int ret = 0;

	try
	{
		new (L, SECTION_TYPE) Section(p->getSection(name));

		ret = 1;
	}
	catch (NotFoundException ex)
	{
		lua_pushnil(L);
		lua_pushfstring(L, "section %s not found", name.c_str());

		ret = 2;
	}

	return ret;
}

int parserRequireSection(lua_State *L)
{
	LuaParser *p;
	std::string name;

	p = toType<LuaParser *>(L, 1, PARSER_TYPE);
	name = luaL_checkstring(L, 2);

	if (!p->hasSection(name))
		return luaL_error(L, "Section %s not found", name.c_str());

	// Copy the section
	new (L, SECTION_TYPE) Section(p->getSection(name));

	return 1;
}

int parserOnLog(lua_State *L)
{
	LuaParser *p = toType<LuaParser *>(L, 1, PARSER_TYPE);

	// Must be a function
	luaL_checktype(L, 2, LUA_TFUNCTION);

	// luaL_ref needs at the top of the stack so put a copy
	lua_pushvalue(L, 2);
	p->setLogRef(luaL_ref(L, LUA_REGISTRYINDEX));

	return 0;
}

const luaL_Reg parserMethodList[] = {
	{ "open",		parserOpen			},
	{ "findSections",	parserFindSections		},
	{ "hasSection",		parserHasSection		},
	{ "getSection",		parserGetSection		},
	{ "requireSection",	parserRequireSection		},
	{ "onLog",		parserOnLog			},
	{ nullptr,		nullptr				}
};

int gc(lua_State *L)
{
	LuaParser *p = toType<LuaParser *>(L, 1, PARSER_TYPE);

	luaL_unref(L, LUA_REGISTRYINDEX, p->getLogRef());

	p->~LuaParser();

	return 0;
}

int tostring(lua_State *L)
{
	LuaParser *p = toType<LuaParser *>(L, 1, PARSER_TYPE);
	std::ostringstream oss;

	oss << *p;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

const luaL_Reg parserMtList[] = {
	{ "__gc",		gc				},
	{ "__tostring",		tostring			},
	{ nullptr,		nullptr				}
};

int sectionHasOption(lua_State *L)
{
	Section *s = toType<Section *>(L, 1, SECTION_TYPE);
	std::string name = luaL_checkstring(L, 2);

	lua_pushboolean(L, s->hasOption(name));

	return 1;
}

int sectionGetOption(lua_State *L)
{
	Section *s = toType<Section *>(L, 1, SECTION_TYPE);
	std::string name = luaL_checkstring(L, 2);

	if (!s->hasOption(name))
	{
		lua_pushnil(L);
		lua_pushfstring(L, "option %s not found", name.c_str());

		return 2;
	}

	lua_pushstring(L, s->getOption<std::string>(name).c_str());

	return 1;
}

int sectionRequireOption(lua_State *L)
{
	Section *s = toType<Section *>(L, 1, SECTION_TYPE);
	std::string name = luaL_checkstring(L, 2);
	std::string value;

	try
	{
		lua_pushstring(L, s->requireOption<std::string>(name).c_str());
	}
	catch (NotFoundException ex)
	{
		return luaL_error(L, "required option %s not found", ex.which().c_str());
	}

	return 1;
}

int sectionGetOptions(lua_State *L)
{
	Section *s = toType<Section *>(L, 1, SECTION_TYPE);

	lua_createtable(L, s->getOptions().size(), s->getOptions().size());
	for (const Option &o : s->getOptions())
	{
		lua_pushstring(L, o.m_key.c_str());
		lua_setfield(L, -2, o.m_value.c_str());
	}

	return 1;
}

const luaL_Reg sectionMethodList[] = {
	{ "hasOption",		sectionHasOption		},
	{ "getOption",		sectionGetOption		},
	{ "requireOption",	sectionRequireOption		},
	{ "getOptions",		sectionGetOptions		},
	{ nullptr,		nullptr				}
};

int sectionEq(lua_State *L)
{
	Section *s1, *s2;

	s1 = toType<Section *>(L, 1, SECTION_TYPE);
	s2 = toType<Section *>(L, 2, SECTION_TYPE);

	lua_pushboolean(L, *s1 == *s2);

	return 1;
}

int sectionGc(lua_State *L)
{
	toType<Section *>(L, 1, SECTION_TYPE)->~Section();

	return 0;
}

int sectionTostring(lua_State *L)
{
	Section *s = toType<Section *>(L, 1, SECTION_TYPE);
	std::ostringstream oss;

	oss << *s;

	lua_pushstring(L, oss.str().c_str());

	return 1;
}

const luaL_Reg sectionMtList[] = {
	{ "__eq",		sectionEq			},
	{ "__gc",		sectionGc			},
	{ "__tostring",		sectionTostring			},
	{ nullptr,		nullptr				}
};

}

int luaopen_parser(lua_State *L)
{
	luaL_newlib(L, functionList);

	// Bind tuning enum
	lua_pushinteger(L, LuaParser::DisableRootSection);
	lua_setfield(L, -2, "DisableRootSection");

	lua_pushinteger(L, LuaParser::DisableRedefinition);
	lua_setfield(L, -2, "DisableRedefinition");

	lua_pushinteger(L, LuaParser::DisableVerbosity);
	lua_setfield(L, -2, "DisableVerbosity");

	// Create Parser type
	luaL_newmetatable(L, PARSER_TYPE);
	luaL_setfuncs(L, parserMtList, 0);
	luaL_newlib(L, parserMethodList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// Create Section type
	luaL_newmetatable(L, SECTION_TYPE);
	luaL_setfuncs(L, sectionMtList, 0);
	luaL_newlib(L, sectionMethodList);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd
