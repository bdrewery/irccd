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

#include <cassert>
#include <memory>
#include <string>

#include <lua.hpp>

namespace irccd {

class LuaDeleter {
public:
	void operator()(lua_State *L) {
		lua_close(L);
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
	 * Get the stack size.
	 *
	 * @return the current stack size
	 */
	int gettop();

	/**
	 * Load a file.
	 *
	 * @param path the file path
	 * @return true on success
	 */
	bool dofile(const std::string& path);

	/**
	 * Create a new table and push it onto the stack.
	 *
	 * @param narr optional elements hint
	 * @param nrec optional elements hint
	 */
	void createtable(int narr = 0, int nrec = 0);

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
	 * Pop some values.
	 *
	 * @param count the number to pop
	 */
	void pop(int count);

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
	 * Creates a reference of object on the top of the stack into the
	 * table t.
	 *
	 * @param t the table to save to
	 * @return the new reference
	 */
	int ref(int t);

	/**
	 * Returns the Lua object type.
	 *
	 * @param idx the index
	 * @return the type
	 */
	int type(int idx);

	/**
	 * Returns the Lua object type name.
	 *
	 * @param type the Lua type
	 * @return the name
	 */
	std::string typeName(int type);

	/**
	 * Remove the reference ref from the table t.
	 *
	 * @param t the table
	 * @param ref the reference
	 */
	void unref(int t, int ref);

	/**
	 * Get the element n from table at index t.
	 *
	 * @param t the table index
	 * @param n the t[n] value
	 */
	void rawget(int t, int n);

	/**
	 * Set the table field where the value is at the top.
	 *
	 * @param t the table index
	 */
	void rawset(int t, int n);

	/**
	 * Move assignment operator.
	 *
	 * @param src the source
	 * @return the object
	 */
	LuaState & operator=(LuaState &&src);
};

#if !defined(NDEBUG)

#define LUA_STACK_CHECKBEGIN(L)						\
	int __topstack = lua_gettop((L))

#define LUA_STACK_CHECKEQUALS(L)					\
	assert(lua_gettop((L)) == __topstack)

#define LUA_STACK_CHECKEND(L, cond)					\
	assert(lua_gettop((L)) cond == __topstack)

#else

#define LUA_STACK_CHECKBEGIN(L)
#define LUA_STACK_CHECKEQUALS(L)
#define LUA_STACK_CHECKEND(L, cond)

#endif

} // !irccd

void * operator new(size_t size, lua_State *L);

void * operator new(size_t size, lua_State *L, const char *metaname);

/*
 * Delete operators are just present to avoid some warnings, Lua does garbage
 * collection so these functions just do nothing.
 */

void operator delete(void *ptr, lua_State *L);

void operator delete(void *ptr, lua_State *L, const char *metaname);

#endif // !_LUA_STATE_H_
