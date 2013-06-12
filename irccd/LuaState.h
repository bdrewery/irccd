/*
 * LuaState.h -- Lua C++ wrapper
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

#ifndef _LUA_STATE_H_
#define _LUA_STATE_H_

#include <memory>
#include <string>

#include <lua.hpp>

namespace irccd {

class LuaDeleter {
public:
	void operator()(lua_State *L) {
		lua_close(L);
		puts("LUA DELETED");
	}
};

typedef std::unique_ptr<lua_State, LuaDeleter> LuaPointer;

class LuaState {
private:
	LuaPointer m_luaState;
	std::string m_error;

public:
	LuaState();

	LuaState(LuaState &&src);

	~LuaState();

	/**
	 * Create a new state.
	 */
	void openState();

	/**
	 * Get the lua state associated with the class.
	 *
	 * @return the state.
	 */
	lua_State * getState();

	/**
	 * Get the error, useful with some function like pcall, dofile.
	 *
	 * @return the error
	 */
	const std::string & getError() const;

	/**
	 * Get a global value.
	 *
	 * @param name the value name
	 * @return the object type (by lua_type())
	 */
	int getglobal(const std::string& name);

	/**
	 * Load a file.
	 *
	 * @param path the file path
	 * @return true on success
	 */
	bool dofile(const std::string& path);

	/**
	 * Push nil.
	 */
	void push();

	/**
	 * Push an integer.
	 *
	 * @param i the integer
	 */
	void push(int i);

	/**
	 * Push a string.
	 *
	 * @param str the string
	 */
	void push(const std::string& str);

	/**
	 * Push a double.
	 *
	 * @param d the double
	 */
	void push(double d);

	/**
	 * Load a library just like it was loaded with require.
	 *
	 * @param name the module name
	 * @param func the function
	 * @param global store as global
	 */
	void require(const std::string& name, lua_CFunction func, bool global);

	/**
	 * Preload a library, it will be added to package.preload so the
	 * user can successfully call require "name". In order to work, you need
	 * to open luaopen_package and luaopen_base first.
	 *
	 * @param name the module name
	 * @param func the opening library
	 * @see require
	 */
	void preload(const std::string& name, lua_CFunction func);

	/**
	 * Call a function.
	 *
	 * @param np the number of parameters
	 * @param nr the number of return value or LUA_MULTRET
	 * @param errorh an optional index of error function handler
	 */
	bool pcall(int np, int nr, int errorh = 0);

	/**
	 * Move assignment operator.
	 *
	 * @param src the source
	 * @return the object
	 */
	LuaState & operator=(LuaState &&src);
};

} // !irccd

void * operator new(size_t size, lua_State *L);

void * operator new(size_t size, lua_State *L, const char *metaname);

#endif // !_LUA_STATE_H_