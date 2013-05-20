/*
 * LuaParser.h -- Lua bindings for class Parser
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

#ifndef _LUA_PARSER_H_
#define _LUA_PARSER_H_

#include <lua.hpp>

#include <Parser.h>

namespace irccd {

class LuaParser : public parser::Parser {
private:
	lua_State *m_state;	//! back pointer for log
	int m_logRef;		//! reference for log()

public:
	static int readTuning(lua_State *L, int idx);

	/**
	 * Add a section as a userdata at the top of the stack.
	 *
	 * @param L the Lua state
	 * @param s the section to push
	 */
	static void pushSection(lua_State *L, const parser::Section &s);

	/**
	 * Wrapper for super constructor.
	 *
	 * @param path the file path
	 * @param tuning optional tuning flags
	 * @param commentToken an optional comment delimiter
	 * @see Parser
	 */
	LuaParser(const std::string &path, int tuning = 0, char commentToken = Parser::DEFAULT_COMMENT_CHAR);

	/**
	 * Default constructor.
	 */
	LuaParser(void);

	/**
	 * Default destructor.
	 */
	~LuaParser(void);

	/**
	 * Set the Lua state, used for logging.
	 *
	 * @param L the Lua state.
	 */
	void setState(lua_State *L);

	/**
	 * Set the Lua function to call for logging.
	 *
	 * @param logRef the function reference
	 */
	void setLogRef(int logRef);

	virtual void log(int number, const std::string &section, const std::string &message);
};

int luaopen_parser(lua_State *L);

} // !irccd

#endif // !_LUA_PARSER_H_
