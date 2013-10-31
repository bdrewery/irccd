/*
 * Lua.h -- Lua helpers and such
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

#ifndef _LUA_H_
#define _LUA_H_

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <lua.hpp>

namespace irccd
{

class LuaDeleter
{
public:
	void operator()(lua_State *L)
	{
		lua_close(L);
	}
};

typedef std::unique_ptr<lua_State, LuaDeleter> LuaState;

class LuaValue
{
private:
	union
	{
		lua_Number	 number;
		bool		 boolean;
	};

	int type;
	std::string str;
	std::vector<std::pair<LuaValue, LuaValue>> table;

public:
	/**
	 * Dump a value at the specific index.
	 *
	 * @param L the Lua state
	 * @param index the value
	 * @return a tree of values
	 */
	static LuaValue copy(lua_State *L, int index);

	/**
	 * Push a value to a state.
	 *
	 * @param L the Lua state
	 * @param value the value to push
	 */
	static void push(lua_State *L, const LuaValue &value);

	/**
	 * Default constructor (type nil)
	 */
	LuaValue();
};

class Luae
{
public:
	typedef std::function<void(lua_State *L, int tkey, int tvalue)> ReadFunction;

	/**
	 * Get a field of a specific type from a table. Specialized for the
	 * types: int, double, bool and string.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @return the converted type.
	 */
	template <typename T>
	static T getField(lua_State *L, int idx, const std::string &name);

	/**
	 * Require a field from a table.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @return the value or call luaL_error
	 */
	template <typename T>
	static T requireField(lua_State *L, int idx, const std::string &name)
	{
		lua_getfield(L, idx, name.c_str());

		if (lua_type(L, -1) == LUA_TNIL)
			luaL_error(L, "missing field `%s'", name.c_str());
			// NOT REACHED

		lua_pop(L, 1);

		return getField<T>(L, idx, name);
	}

	/**
	 * Check a table field.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @return the type
	 */
	static int typeField(lua_State *L, int idx, const std::string &name);

	/**
	 * Read a table, the function func is called for each element in the
	 * table. Parameter tkey is the Lua type of the key, parameter tvalue is
	 * the Lua type of the value. The key is available at index -2 and the
	 * value at index -1.
	 *
	 * <strong>Do not pop anything within the function.</strong>
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param func the function to call
	 */
	static void readTable(lua_State *L, int idx, ReadFunction func);

	/**
	 * Preload a library, it will be added to package.preload so the
	 * user can successfully call require "name". In order to work, you need
	 * to open luaopen_package and luaopen_base first.
	 *
	 * @param L the Lua state
	 * @param name the module name
	 * @param func the opening library
	 * @see require
	 */
	static void preload(lua_State *L,
			    const std::string &name,
			    lua_CFunction func);

	/**
	 * Reference a field from a table at the index. The reference is created in
	 * the registry only if type matches.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param type the type requested
	 * @param name the field name
	 * @return the reference or LUA_REFNIL on problem
	 */
	static int referenceField(lua_State *L,
				  int idx,
				  int type,
				  const std::string &name);

	/**
	 * Load a library just like it was loaded with require.
	 *
	 * @param L the Lua state
	 * @param name the module name
	 * @param func the function
	 * @param global store as global
	 */
	static void require(lua_State *L,
			    const std::string &name,
			    lua_CFunction func,
			    bool global);

	/**
	 * Convert a class created with new placement.
	 *
	 * @param L the Lua state
	 * @param idx the value index
	 * @param metaname the metatable name
	 * @return the converted object
	 */
	template <typename T>
	static T toType(lua_State *L, int idx, const char *metaname)
	{
		return reinterpret_cast<T>(luaL_checkudata(L, idx, metaname));
	}
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

#endif // !_LUA_H_
