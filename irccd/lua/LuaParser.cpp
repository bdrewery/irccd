/*
 * LuaParser.cpp -- Lua bindings for class Parser
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

#include <sstream>

#include <irccd/Irccd.h>
#include <irccd/Luae.h>

#include "LuaParser.h"

namespace irccd {

namespace {

/* --------------------------------------------------------
 * Helpers
 * -------------------------------------------------------- */

const char *SectionType		= "Section";
const char *ParserType		= "Parser";

}

/* --------------------------------------------------------
 * LuaParser implementation
 * -------------------------------------------------------- */

int LuaParser::readTuning(lua_State *L, int idx)
{
	int tuning = 0;
	int rel = idx;

	if (Luae::type(L, rel) == LUA_TTABLE) {
		Luae::push(L, nullptr);

		if (rel < 0)
			--rel;

		while (Luae::next(L, rel)) {
			if (Luae::type(L, -1) == LUA_TNUMBER)
				tuning |= Luae::get<int>(L, -1);

			Luae::pop(L);
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
	: m_state(nullptr)
	, m_logRef(LUA_NOREF)
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
		Luae::rawget(m_state, LUA_REGISTRYINDEX, m_logRef);
		Luae::push(m_state, number);
		Luae::push(m_state, section);
		Luae::push(m_state, message);
		Luae::call(m_state, 3, 0);
	}
}

/* --------------------------------------------------------
 * ParserWrapper
 * -------------------------------------------------------- */

/**
 * @class ParserWrapper
 * @brief Backward compatibility
 *
 * This class is used to wrap the parser and its parameters
 * because the class Parser now throw an exception in the constructor
 * but the Lua API expect that .new return and object and .open
 * returns the error. So we only create the parser at .open method.
 */
class ParserWrapper {
public:
	LuaParser	m_parser;	//!< the parser object
	std::string	m_path;		//!< the path
	int		m_flags;	//!< the flags
	char		m_comment;	//!< the comment character

	/**
	 * Create the wrapper.
	 *
	 * @param path the path
	 * @param flags the flags
	 * @param comment the comment character
	 */
	ParserWrapper(const std::string &path, int flags, char comment)
		: m_path(path)
		, m_flags(flags)
		, m_comment(comment)
	{
	}
};

/**
 * @brief Overload for parser
 */
template <>
struct Luae::IsUserdata<ParserWrapper> : std::true_type {
	/**
	 * The metatable name.
	 */
	static const char *MetatableName;
};

const char *Luae::IsUserdata<ParserWrapper>::MetatableName = ParserType;

/**
 * @brief Overload for parser Section
 */
template <>
struct Luae::IsUserdata<Section> : std::true_type {
	/**
	 * The metatable name.
	 */
	static const char *MetatableName;
};

const char *Luae::IsUserdata<Section>::MetatableName = SectionType;

/* --------------------------------------------------------
 * Parser functions
 * -------------------------------------------------------- */

namespace {

int l_new(lua_State *L)
{
	int tuning = 0, ch = LuaParser::DEFAULT_COMMENT_CHAR;

	// Get the flags
	auto path = Luae::check<std::string>(L, 1);
	if (Luae::gettop(L) >= 2) {
		Luae::checktype(L, 2, LUA_TTABLE);
		tuning = LuaParser::readTuning(L, 2);
	}

	if (Luae::gettop(L) >= 3) {
		auto str = Luae::check<std::string>(L, 3);

		if (str.size() > 0)
			ch = str[0];
	}
		
	try {
		auto p = new (L, ParserType) ParserWrapper(path, tuning, ch);

		p->m_parser.setState(L);
	} catch (std::runtime_error er) {
		Luae::push(L, nullptr);
		Luae::push(L, er.what());

		return 2;
	}

	return 1;
}

const Luae::Reg functionList {
	{ "new",		l_new			}
};

int l_parserOpen(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);

	try {
		p->m_parser = LuaParser(p->m_path, p->m_flags, p->m_comment);
	} catch (std::runtime_error error) {
		Luae::push(L, nullptr);
		Luae::push(L, error.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_parserIterator(lua_State *L)
{
	auto array = Luae::toType<std::vector<Section> *>(L, Luae::upvalueindex(1));
	auto idx = Luae::get<int>(L, Luae::upvalueindex(2));

	if ((size_t)idx >= array->size())
		return 0;

	// Push the current section.
	Luae::push(L, Section((*array)[idx]));

	// Update the index for the next call
	Luae::push(L, ++idx);
	Luae::replace(L, Luae::upvalueindex(2));

	return 1;
}

int l_parserFindSections(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);
	auto name = Luae::check<std::string>(L, 2);

	std::vector<Section> sections;

	p->m_parser.findSections(name, [&] (const Section &s) {
		sections.push_back(s);
	});

	// Push the list of array and the current index to iterate one
	new (L) std::vector<Section>(sections);

	// Add a special __gc function for deleting the table
	LuaeTable::create(L);
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		Luae::toType<std::vector<Section> *>(L, 1)->~vector<Section>();

		return 0;
	});
	Luae::setfield(L, -2, "__gc");
	Luae::setmetatable(L, -2);

	// Push upvalue and the iterator function
	Luae::push(L, 0);
	Luae::pushfunction(L, l_parserIterator, 2);

	return 1;
}

int l_parserHasSection(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);
	auto name = Luae::check<std::string>(L, 2);

	Luae::push(L, p->m_parser.hasSection(name));

	return 1;
}

int l_parserGetSection(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);
	auto name = Luae::check<std::string>(L, 2);
	int ret = 0;

	try {
		Luae::push(L, Section(p->m_parser.getSection(name)));

		ret = 1;
	} catch (std::out_of_range ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		ret = 2;
	}

	return ret;
}

int l_parserRequireSection(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);
	auto name = Luae::check<std::string>(L, 2);

	if (!p->m_parser.hasSection(name))
		return Luae::error(L, "section %s not found", name.c_str());

	// Copy the section
	Luae::push(L, Section(p->m_parser.getSection(name)));

	return 1;
}

int l_parserOnLog(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);

	// Must be a function
	Luae::checktype(L, 2, LUA_TFUNCTION);

	// luaL_ref needs at the top of the stack so put a copy
	Luae::pushvalue(L, 2);
	p->m_parser.setLogRef(Luae::ref(L, LUA_REGISTRYINDEX));

	return 0;
}

int l_parserEq(lua_State *L)
{
	auto p1 = Luae::check<ParserWrapper>(L, 1);
	auto p2 = Luae::check<ParserWrapper>(L, 2);

	Luae::push(L, p1->m_parser == p2->m_parser);

	return 1;
}

int l_parserGc(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);

	if (p->m_parser.getLogRef() != LUA_REFNIL)
		Luae::unref(L, LUA_REGISTRYINDEX, p->m_parser.getLogRef());

	p->m_parser.~LuaParser();

	return 0;
}

int l_parserPairs(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);

	Luae::pushIterator(L, p->m_parser);

	return 1;
}

int l_parserTostring(lua_State *L)
{
	auto p = Luae::check<ParserWrapper>(L, 1);
	std::ostringstream oss;

	oss << "sections: [";
	for (const auto &s : p->m_parser) {
		// We don't add root, it always added
		if (s.getName().length() >= 1)
			oss << " " << s.getName();
	}
	oss << " ]";

	Luae::push(L, oss.str());

	return 1;
}

const Luae::Reg parserMethods {
	{ "open",		l_parserOpen			},
	{ "findSections",	l_parserFindSections		},
	{ "hasSection",		l_parserHasSection		},
	{ "getSection",		l_parserGetSection		},
	{ "requireSection",	l_parserRequireSection		},
	{ "onLog",		l_parserOnLog			}
};

const Luae::Reg parserMeta {
	{ "__eq",		l_parserEq			},
	{ "__gc",		l_parserGc			},
	{ "__pairs",		l_parserPairs			},
	{ "__tostring",		l_parserTostring		}
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
		Luae::push(L, s.getOption<std::string>(name).c_str());
	else if (type == "number")
		Luae::push(L, s.getOption<double>(name));
	else if (type == "boolean")
		Luae::push(L, s.getOption<bool>(name));
	else {
		Luae::push(L, nullptr);
		Luae::push(L, "invalid type requested");

		return 2;
	}

	return 1;
}

int l_sectionName(lua_State *L)
{
	auto s = Luae::check<Section>(L, 1);

	Luae::push(L, s->getName());

	return 1;
}

int l_sectionHasOption(lua_State *L)
{
	auto s = Luae::check<Section>(L, 1);
	auto name = Luae::check<std::string>(L, 2);

	Luae::push(L, s->hasOption(name));

	return 1;
}

int l_sectionGetOption(lua_State *L)
{
	auto s = Luae::check<Section>(L, 1);
	auto name = Luae::check<std::string>(L, 2);
	std::string type = "string";

	if (Luae::gettop(L) >= 3)
		type = Luae::check<std::string>(L, 3);

	if (!s->hasOption(name)) {
		Luae::push(L, nullptr);
		Luae::pushfstring(L, "option %s not found", name.c_str());

		return 2;
	}

	return pushOption(L, *s, name, type);
}

int l_sectionRequireOption(lua_State *L)
{
	auto s = Luae::check<Section>(L, 1);
	auto name = Luae::check<std::string>(L, 2);
	std::string type = "string";

	if (Luae::gettop(L) >= 3)
		type = Luae::check<std::string>(L, 3);

	if (!s->hasOption(name))
		return Luae::error(L, "option %s not found", name.c_str());

	return pushOption(L, *s, name, type);
}

#if defined(COMPAT_1_1)

int l_sectionGetOptions(lua_State *L)
{
	Luae::deprecate(L, "getOptions", "pairs");

	auto s = Luae::check<Section>(L, 1);

	LuaeTable::create(L);

	for (const auto &o : *s)
		LuaeTable::set(L, -1, o.first, o.second);

	return 1;
}

#endif

int l_sectionEquals(lua_State *L)
{
	auto s1 = Luae::check<Section>(L, 1);
	auto s2 = Luae::check<Section>(L, 2);

	Luae::push(L, *s1 == *s2);

	return 1;
}

int l_sectionGc(lua_State *L)
{
	Luae::check<Section>(L, 1)->~Section();

	return 0;
}

int l_sectionPairs(lua_State *L)
{
	using SectionIterator = Luae::Iterator<Section::const_iterator>;

	auto s = Luae::check<Section>(L, 1);

	/*
	 * Do not use Luae::pushPairs because we want to push the value
	 * and the key directly.
	 */
	new (L) SectionIterator(s->begin(), s->end());
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		auto it = Luae::toType<SectionIterator *>(L, Luae::upvalueindex(1));

		if (it->current == it->end)
			return 0;

		auto value = it->current++;

		Luae::push(L, value->first);
		Luae::push(L, value->second);

		return 2;
	}, 1);

	return 1;
}

int l_sectionTostring(lua_State *L)
{
	auto s = Luae::check<Section>(L, 1);
	std::ostringstream oss;
	std::string str;

	oss << s->getName() << ": [";
	for (const auto &o : *s)
		oss << " " << o.first << " = " << o.second;
	oss << " ]";

	str = oss.str();
	Luae::push(L, str);

	return 1;
}

const Luae::Reg sectionMethods {
	{ "getName",			l_sectionName			},
	{ "hasOption",			l_sectionHasOption		},
	{ "getOption",			l_sectionGetOption		},
/*
 * DEPRECATION:	1.2-001
 *
 * All the following functions have been moved to the irccd.system.
 */
#if defined(COMPAT_1_1)
	{ "getOptions",			l_sectionGetOptions		},
#endif
	{ "requireOption",		l_sectionRequireOption		}
};

const Luae::Reg sectionMeta {
	{ "__eq",			l_sectionEquals			},
	{ "__gc",			l_sectionGc			},
	{ "__pairs",			l_sectionPairs			},
	{ "__tostring",			l_sectionTostring		}
};

const LuaeEnum::Def tuning {
	{ "DisableRootSection",		static_cast<int>(Parser::DisableRootSection)	},
	{ "DisableRedefinition",	static_cast<int>(Parser::DisableRedefinition)	},
	{ "DisableVerbosity",		static_cast<int>(Parser::DisableVerbosity)	},
};

}

int luaopen_parser(lua_State *L)
{
	Luae::newlib(L, functionList);

	// Bind tuning enum
	LuaeEnum::create(L, tuning, -1);

	// Create Parser type
	Luae::newmetatable(L, ParserType);
	Luae::setfuncs(L, parserMeta);
	Luae::newlib(L, parserMethods);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	// Create Section type
	Luae::newmetatable(L, SectionType);
	Luae::setfuncs(L, sectionMeta);
	Luae::newlib(L, sectionMethods);
	Luae::setfield(L, -2, "__index");
	Luae::pop(L, 1);

	return 1;
}

} // !irccd
