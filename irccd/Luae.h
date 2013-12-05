/*
 * Lua.h -- Lua helpers and such
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

#ifndef _LUA_H_
#define _LUA_H_

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <lua.hpp>

namespace irccd {

/**
 * @class LuaState
 * @brief Wrapper for lua_State
 *
 * This class automatically create a new Lua state and add implicit
 * cast operator plus RAII destruction.
 */
class LuaState {
private:
	struct Deleter {
		void operator()(lua_State *L)
		{
			lua_close(L);
		}
	};

	using Ptr = std::unique_ptr<lua_State, Deleter>;

	std::string test;
	Ptr m_state;

public:
	LuaState(const LuaState &) = delete;
	LuaState &operator=(const LuaState &) = delete;

	/**
	 * Default constructor. Create a new state.
	 */
	LuaState();

	/**
	 * Use the already created state.
	 *
	 * @param L the state to use
	 */
	LuaState(lua_State *L);

	/**
	 * Move constructor.
	 *
	 * @param state the Lua state to move
	 */
	LuaState(LuaState &&state);

	/**
	 * Move assignment operator.
	 *
	 * @param state the Lua state to move
	 */
	LuaState &operator=(LuaState &&state);

	/**
	 * Implicit cast operator for convenient usage to C Lua API.
	 *
	 * @return the state as lua_State *
	 */
	operator lua_State*();
};

/**
 * @class LuaValue
 * @brief A fake variant for Lua values
 *
 * This class is primarly used for copying Lua values without checking
 * the types, useful to pass data.
 */
class LuaValue {
private:
	union {
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

/**
 * @class Luae
 * @brief Add lot of convenience for Lua
 *
 * This class adds lot of functions for Lua and C++.
 */
class Luae
{
public:
	using ReadFunction = std::function<void(lua_State *L, int tkey, int tvalue)>;

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
	 * Initialize the registry for shared objects.
	 *
	 * @param L the Lua state
	 */
	static void initRegistry(lua_State *L);

	/**
	 * Push a shared object to Lua, it also push it to the "refs"
	 * table with __mode = "v". That is if we need to push the object
	 * again we use the same reference so Lua get always the same
	 * userdata and gain the following benefits:
	 *
	 * 1. The user can use the userdata as table key
	 * 2. A performance gain thanks to less allocations
	 *
	 * @param L the Lua state
	 * @param o the object to push
	 * @param name the object metatable name
	 */
	template <typename T>
	static void pushShared(lua_State *L,
			std::shared_ptr<T> o,
			const std::string &name)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, "refs");
		assert(lua_type(L, -1) == LUA_TTABLE);

		lua_rawgetp(L, -1, o.get());

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_pop(L, 1);

			new (L, name.c_str()) std::shared_ptr<T>(o);
			
			lua_pushvalue(L, -1);
			lua_rawsetp(L, -3, o.get());
		}

		lua_replace(L, -2);
	}

	/**
	 * Get an object from Lua that was previously push with pushShared.
	 *
	 * @param L the Lua state
	 * @param index the object index
	 * @param meta the object metatable name
	 * @return the object
	 */
	template <typename T>
	static std::shared_ptr<T> getShared(lua_State *L, int index, const char *meta)
	{
		using Ptr = std::shared_ptr<T>;
		
		Ptr *ptr = static_cast<Ptr *>(luaL_checkudata(L, index, meta));
		
		return *ptr;
	}

	/**
	 * Convert a new placement made object, without testing if its a real
	 * object.
	 *
	 * @param L the Lua state
	 * @param idx the object index
	 * @return the converted object
	 */
	template<class T>
	static T toType(lua_State *L, int idx)
	{
		return reinterpret_cast<T>(lua_touserdata(L, idx));
	}

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

void *operator new(size_t size, lua_State *L);

void *operator new(size_t size, lua_State *L, const char *metaname);

void operator delete(void *ptr, lua_State *L);

void operator delete(void *ptr, lua_State *L, const char *metaname);

#endif // !_LUA_H_
