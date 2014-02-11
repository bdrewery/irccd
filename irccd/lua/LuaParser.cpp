/*
 * LuaParser.cpp -- Lua bindings for class Parser
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include <sstream>

#include "Irccd.h"
#include "LuaParser.h"

namespace irccd {

/* --------------------------------------------------------
 * LuaParser implementation
 * -------------------------------------------------------- */

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
	else if (m_state != nullptr) {
		// Call the Lua ref function
		lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_logRef);
		lua_pushinteger(m_state, number);
		lua_pushstring(m_state, section.c_str());
		lua_pushstring(m_state, message.c_str());

		lua_call(m_state, 3, 0);
	}
}

/* --------------------------------------------------------
 * Helpers
 * -------------------------------------------------------- */

/**
 * @struct Iterator
 * @brief Class used for iterating containers in Lua
 */
template <typename T>
struct Iterator {
	T begin;
	T end;
	T current;	

	Iterator(T begin, T end)
		: begin(begin)
		, end(end)
		, current(begin)
	{
	}
};

using ParserIterator		= Iterator<Parser::List::iterator>;
using SectionIterator		= Iterator<Section::Map::iterator>;

const char *SectionType		= "Section";
const char *ParserType		= "Parser";

/* --------------------------------------------------------
 * Parser functions
 * -------------------------------------------------------- */

namespace {

int l_new(lua_State *L)
{
	LuaParser *p;
	std::string path;
	int tuning = 0, ch = LuaParser::DEFAULT_COMMENT_CHAR;

	// Get the flags
	path = luaL_checkstring(L, 1);
	if (lua_gettop(L) >= 2) {
		luaL_checktype(L, 2, LUA_TTABLE);
		tuning = LuaParser::readTuning(L, 2);
	}

	if (lua_gettop(L) >= 3)
		ch = luaL_checkstring(L, 1)[0];
		
	try {
		p = new (L, ParserType) LuaParser(path, tuning, ch);
	} catch (std::runtime_error er) {
		lua_pushnil(L);
		lua_pushstring(L, er.what());

		return 2;
	}

	// Copy the state so I can call log() function
	p->setState(L);

	return 1;
}

const luaL_Reg functionList[] = {
	{ "new",		l_new			},
	{ nullptr,		nullptr			}
};

int l_parserIterator(lua_State *L)
{
	std::vector<Section> *array = Luae::toType<std::vector<Section> *>(L, lua_upvalueindex(1));
	int idx = lua_tointeger(L, lua_upvalueindex(2));

	if ((size_t)idx >= array->size())
		return 0;

	// Push the current section.
	new (L, SectionType) Section((*array)[idx]);

	// Update the index for the next call
	lua_pushinteger(L, ++idx);
	lua_replace(L, lua_upvalueindex(2));

	return 1;
}

int l_parserFindSections(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto name = luaL_checkstring(L, 2);
	std::vector<Section> sections;

	p->findSections(name, [&] (const Section &s) {
		sections.push_back(s);
	});

	// Push the list of array and the current index to iterate one
	new (L) std::vector<Section>(sections);

	// Add a special __gc function for deleting the table
	lua_createtable(L, 0, 0);
	lua_pushcfunction(L, [] (lua_State *L) -> int {
		Luae::toType<std::vector<Section> *>(L, 1)->~vector<Section>();

		return 0;
	});
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);

	// Push upvalue and the iterator function
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, l_parserIterator, 2);

	return 1;
}

int l_parserHasSection(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto name = luaL_checkstring(L, 2);

	lua_pushboolean(L, p->hasSection(name));

	return 1;
}

int l_parserGetSection(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto name = luaL_checkstring(L, 2);
	int ret = 0;

	try {
		new (L, SectionType) Section(p->getSection(name));

		ret = 1;
	} catch (std::out_of_range ex) {
		lua_pushnil(L);
		lua_pushfstring(L, ex.what());

		ret = 2;
	}

	return ret;
}

int l_parserRequireSection(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto name = luaL_checkstring(L, 2);

	if (!p->hasSection(name))
		return luaL_error(L, "section %s not found", name);

	// Copy the section
	new (L, SectionType) Section(p->getSection(name));

	return 1;
}

int l_parserOnLog(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);

	// Must be a function
	luaL_checktype(L, 2, LUA_TFUNCTION);

	// luaL_ref needs at the top of the stack so put a copy
	lua_pushvalue(L, 2);
	p->setLogRef(luaL_ref(L, LUA_REGISTRYINDEX));

	return 0;
}

int l_parserEq(lua_State *L)
{
	auto p1 = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto p2 = Luae::toType<LuaParser *>(L, 2, ParserType);

	lua_pushboolean(L, *p1 == *p2);

	return 1;
}

int l_parserGc(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);

	luaL_unref(L, LUA_REGISTRYINDEX, p->getLogRef());

	p->~LuaParser();

	return 0;
}

int l_parserPairs(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	auto begin = p->begin();
	auto end = p->end();

	new (L) ParserIterator(p->begin(), p->end());
	lua_pushcclosure(L, [] (lua_State *L) -> int {
		auto it = Luae::toType<ParserIterator *>(L, lua_upvalueindex(1));

		if (it->current == it->end)
			return 0;

		new (L, SectionType) Section(*(it->current++));

		return 1;
	}, 1);

	return 1;
}

int l_parserTostring(lua_State *L)
{
	auto p = Luae::toType<LuaParser *>(L, 1, ParserType);
	std::ostringstream oss;
	std::string str;

	oss << "sections: [";
	for (const auto &s : *p) {
		// We don't add root, it always added
		if (s.getName().length() >= 1)
			oss << " " << s.getName();
	}
	oss << " ]";

	str = oss.str();
	lua_pushlstring(L, str.c_str(), str.length());

	return 1;
}

const luaL_Reg parserMethods[] = {
	{ "findSections",	l_parserFindSections		},
	{ "hasSection",		l_parserHasSection		},
	{ "getSection",		l_parserGetSection		},
	{ "requireSection",	l_parserRequireSection		},
	{ "onLog",		l_parserOnLog			},
	{ nullptr,		nullptr				}
};

const luaL_Reg parserMeta[] = {
	{ "__eq",		l_parserEq			},
	{ "__gc",		l_parserGc			},
	{ "__pairs",		l_parserPairs			},
	{ "__tostring",		l_parserTostring		},
	{ nullptr,		nullptr				}
};

/* --------------------------------------------------------
 * Section methods
 * -------------------------------------------------------- */

int pushOption(lua_State *L,
	       const Section &s,
	       const std::string &name,
	       const std::string &type)
{
	if (type == "string")
		lua_pushstring(L, s.getOption<std::string>(name).c_str());
	else if (type == "number")
		lua_pushnumber(L, s.getOption<double>(name));
	else if (type == "boolean")
		lua_pushboolean(L, s.getOption<bool>(name));
	else {
		lua_pushnil(L);
		lua_pushstring(L, "invalid type requested");

		return 2;
	}

	return 1;
}

int l_sectionName(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);
	auto name = s->getName();

	lua_pushlstring(L, name.c_str(), name.length());

	return 1;
}

int l_sectionHasOption(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);
	auto name = luaL_checkstring(L, 2);

	lua_pushboolean(L, s->hasOption(name));

	return 1;
}

int l_sectionGetOption(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);
	auto name = luaL_checkstring(L, 2);
	auto type = "string";

	if (lua_gettop(L) >= 3)
		type = luaL_checkstring(L, 3);

	if (!s->hasOption(name)) {
		lua_pushnil(L);
		lua_pushfstring(L, "option %s not found", name);

		return 2;
	}

	return pushOption(L, *s, name, type);
}

int l_sectionRequireOption(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);
	auto name = luaL_checkstring(L, 2);
	auto type = "string";

	if (lua_gettop(L) >= 3)
		type = luaL_checkstring(L, 3);

	if (!s->hasOption(name))
		return luaL_error(L, "option %s not found", name);

	return pushOption(L, *s, name, type);
}

int l_sectionEquals(lua_State *L)
{
	auto s1 = Luae::toType<Section *>(L, 1, SectionType);
	auto s2 = Luae::toType<Section *>(L, 2, SectionType);

	lua_pushboolean(L, *s1 == *s2);

	return 1;
}

int l_sectionGc(lua_State *L)
{
	Luae::toType<Section *>(L, 1, SectionType)->~Section();

	return 0;
}

int l_sectionPairs(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);

	new (L) SectionIterator(s->begin(), s->end());
	lua_pushcclosure(L, [] (lua_State *L) -> int {
		auto it = Luae::toType<SectionIterator *>(L, lua_upvalueindex(1));

		if (it->current == it->end)
			return 0;

		auto value = it->current++;

		lua_pushlstring(L, value->first.c_str(), value->first.length());
		lua_pushlstring(L, value->second.c_str(), value->second.length());

		return 2;
	}, 1);

	return 1;
}

int l_sectionTostring(lua_State *L)
{
	auto s = Luae::toType<Section *>(L, 1, SectionType);
	std::ostringstream oss;
	std::string str;

	oss << s->getName() << ": [";
	for (const auto &o : *s)
		oss << " " << o.first << " = " << o.second;
	oss << " ]";

	str = oss.str();
	lua_pushlstring(L, str.c_str(), str.length());

	return 1;
}

const luaL_Reg sectionMethods[] = {
	{ "getName",		l_sectionName			},
	{ "hasOption",		l_sectionHasOption		},
	{ "getOption",		l_sectionGetOption		},
	{ "requireOption",	l_sectionRequireOption		},
	{ nullptr,		nullptr				}
};

const luaL_Reg sectionMeta[] = {
	{ "__eq",		l_sectionEquals			},
	{ "__gc",		l_sectionGc			},
	{ "__pairs",		l_sectionPairs			},
	{ "__tostring",		l_sectionTostring		},
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
	luaL_newmetatable(L, ParserType);
	luaL_setfuncs(L, parserMeta, 0);
	luaL_newlib(L, parserMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	// Create Section type
	luaL_newmetatable(L, SectionType);
	luaL_setfuncs(L, sectionMeta, 0);
	luaL_newlib(L, sectionMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd
