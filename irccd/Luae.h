/*
 * Luae.h -- Lua helpers and such
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
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <lua.hpp>

#if !defined(NDEBUG)

#define LUAE_STACK_CHECKBEGIN(L)					\
	int __topstack = lua_gettop((L))

#define LUAE_STACK_CHECKEQUALS(L)					\
	assert(lua_gettop((L)) == __topstack)

#define LUAE_STACK_CHECKEND(L, cond)					\
	assert(lua_gettop((L)) cond == __topstack)

#else

#define LUAE_STACK_CHECKBEGIN(L)
#define LUAE_STACK_CHECKEQUALS(L)
#define LUAE_STACK_CHECKEND(L, cond)

#endif

namespace irccd {

/* {{{ Luae */

/**
 * @class Luae
 * @brief Add lot of convenience for Lua
 *
 * This class adds lot of functions for Lua and C++.
 */
class Luae {
private:
	/*
	 * Wrapper for dofile and dostring.
	 */
	static void doexecute(lua_State *L, int status);

public:
	/**
	 * @struct Convert
	 * @brief Push or get values
	 */
	template <typename T>
	struct Convert {
		static const bool supported = false;
	};

	/**
	 * @struct Iterator
	 * @brief Wrap STL containers
	 */
	template <typename Type>
	struct Iterator {
		Type	begin;
		Type	end;
		Type	current;

		Iterator(Type begin, Type end)
			: begin(begin)
			, end(end)
			, current(begin)
		{
		}
	};

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
	 * Get a global value from Lua.
	 *
	 * @param L the Lua state
	 * @param name the value name
	 */
	static inline void getglobal(lua_State *L, const std::string &name)
	{
		lua_getglobal(L, name.c_str());
	}

	/**
	 * Load all Lua libraries.
	 *
	 * @param L the Lua state
	 */
	static inline void openlibs(lua_State *L)
	{
		luaL_openlibs(L);
	}

	/**
	 * Set a global.
	 *
	 * @param L the Lua state
	 * @param name the name
	 */
	static inline void setglobal(lua_State *L, const std::string &name)
	{
		lua_setglobal(L, name.c_str());
	}

	/**
	 * Push a function with an optional number of upvalues.
	 *
	 * @param L the Lua state
	 * @param func the function
	 * @param nup the number of up values
	 */
	static inline void pushfunction(lua_State *L, lua_CFunction func, int nup = 0)
	{
		lua_pushcclosure(L, func, nup);
	}

	/**
	 * Move the top element at the given index.
	 *
	 * @param L the Lua state
	 * @param index the new index
	 */
	static inline void insert(lua_State *L, int index)
	{
		lua_insert(L, index);
	}

	/**
	 * Replace the element at the given index by the one at the top.
	 *
	 * @param L the Lua state
	 * @param index the new index
	 */
	static inline void replace(lua_State *L, int index)
	{
		lua_replace(L, index);
	}

	/**
	 * Remove the element at the given index.
	 *
	 * @param L the Lua state
	 * @param index the table index
	 */
	static inline void remove(lua_State *L, int index)
	{
		lua_remove(L, index);
	}

	/**
	 * Wrapper around pcall, throw instead of returning an error code.
	 *
	 * @param L the Lua state
	 * @param np the number of parameters
	 * @param nr the number of return value
	 * @param error the message handler
	 */
	static inline void pcall(lua_State *L, int np, int nr, int error = 0)
	{
		if (lua_pcall(L, np, nr, error) != LUA_OK) {
			auto error = lua_tostring(L, -1);
			lua_pop(L, 1);

			throw std::runtime_error(error);
		}
	}

	/**
	 * Create a unique reference to the table at the given index.
	 *
	 * @param L the Lua state
	 * @param index the table index
	 * @return the reference
	 */
	static inline int ref(lua_State *L, int index)
	{
		return luaL_ref(L, index);
	}

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
	 * Load and execute a file.
	 *
	 * @param L the Lua state
	 * @param path the the path
	 * @throw std::runtime_error on error
	 */
	static inline void dofile(lua_State *L, const std::string &path)
	{
		doexecute(L, luaL_dofile(L, path.c_str()));
	}

	/**
	 * Load and execute a string.
	 *
	 * @param L the Lua state
	 * @param data the data
	 * @throw std::runtime_error on error
	 */
	static inline void dostring(lua_State *L, const std::string &data)
	{
		doexecute(L, luaL_dostring(L, data.c_str()));
	}

	/**
	 * Get the type at the given index.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the type
	 */
	static inline int type(lua_State *L, int index)
	{
		return lua_type(L, index);
	}

	/**
	 * Push a value to Lua.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	template <typename T>
	static void push(lua_State *L, const T &value)
	{
		static_assert(Convert<T>::supported, "type not supported");

		Convert<T>::push(L, value);
	}

	/**
	 * Overload for string literals.
	 *
	 * @param L the Lua state
	 * @param s the string
	 */
	template <size_t N>
	static void push(lua_State *L, const char (&s)[N])
	{
		push<const char *>(L, s);
	}

	/**
	 * Check the type at the given index. Calls luaL_error on bad
	 * type.
	 *
	 * @param L the Lua state
	 * @param index the the index
	 * @param type the type to check
	 */
	static inline void checktype(lua_State *L, int index, int type)
	{
		luaL_checktype(L, index, type);
	}

	/**
	 * Get a value from Lua. The type are not checked
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the value
	 */
	template <typename T>
	static T get(lua_State *L, int index)
	{
		static_assert(Convert<T>::supported, "type not supported");

		return Convert<T>::get(L, index);
	}

	/**
	 * Get the value at the index, raise a Lua error on failures.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the value
	 */
	template <typename T>
	static T check(lua_State *L, int index)
	{
		return Convert<T>::check(L, index);
	}

	/**
	 * Push an iterator function onto the stack. May be used by functions
	 * calls or directly with __pairs.
	 *
	 * The container must at least have:
	 *	value_type,
	 *	const_iterator
	 *
	 * The value of the container must also be pushable with Luae::Convert.
	 *
	 * @param L the Lua state
	 * @param container the container
	 * @return 1
	 */
	template <typename Container,
		  typename Converter = Convert<typename Container::value_type>>
	static int pushIterator(lua_State *L, const Container &container)
	{
		using ValueType		= typename Container::value_type;
		using IteratorType	= typename Container::const_iterator;

		static_assert(Convert<ValueType>::supported, "type not supported");

		LUAE_STACK_CHECKBEGIN(L);

		new (L) Iterator<IteratorType>(container.cbegin(), container.cend());

		lua_createtable(L, 0, 0);
		lua_pushcfunction(L, [] (lua_State *L) -> int {
			toType<Iterator<IteratorType> *>(L, 1)->~Iterator<IteratorType>();

			return 0;
		});
		lua_setfield(L, -2, "__gc");
		lua_setmetatable(L, -2);

		// Push the iterator function
		lua_pushcclosure(L, [] (lua_State *L) -> int {
			auto it = toType<Iterator<IteratorType> *>(L, lua_upvalueindex(1));

			if (it->current == it->end)
				return 0;

				Converter::push(L, *(it->current++));

			return 1;
		}, 1);

		LUAE_STACK_CHECKEND(L, -1);
		
		return 1;
	}

	/**
	 * Write a warning about a deprecated feature.
	 *
	 * @param L the Lua state
	 * @param name the name
	 * @param repl the optional replacement
	 */
	static void deprecate(lua_State *L, const std::string &name, const std::string &repl = "");

	/**
	 * Convert a new placement made object, without testing if its a real
	 * object.
	 *
	 * @param L the Lua state
	 * @param idx the object index
	 * @return the converted object
	 */
	template<class T>
	static inline T toType(lua_State *L, int idx)
	{
		return reinterpret_cast<T>(lua_touserdata(L, idx));
	}

	/**
	 * Check for a userdata from the stack but without checking if it's a real
	 * LuaeClass one.
	 *
	 * @param L the Lua state
	 * @param idx the index
	 * @param meta the metatable name
	 * @return the object
	 */
	template <class T>
	static inline T toType(lua_State *L, int idx, const char *meta)
	{
		return reinterpret_cast<T>(luaL_checkudata(L, idx, meta));
	}

	/**
	 * Pop arguments from the stack.
	 *
	 * @param L the Lua state
	 * @param count the number of values to pop
	 */
	static inline void pop(lua_State *L, int count)
	{
		lua_pop(L, count);
	}
};

template <>
struct Luae::Convert<bool> {
	static const bool supported = true;

	static void push(lua_State *L, const bool &value)
	{
		lua_pushboolean(L, value);
	}

	static bool get(lua_State *L, int index)
	{
		return lua_toboolean(L, index);
	}

	static bool check(lua_State *L, int index)
	{
		return lua_toboolean(L, index);
	}
};

template <>
struct Luae::Convert<int> {
	static const bool supported = true;

	static void push(lua_State *L, const int &value)
	{
		lua_pushinteger(L, value);
	}

	static int get(lua_State *L, int index)
	{
		return lua_tointeger(L, index);
	}

	static int check(lua_State *L, int index)
	{
		return luaL_checkinteger(L, index);
	}
};

template <>
struct Luae::Convert<double> {
	static const bool supported = true;

	static void push(lua_State *L, const double &value)
	{
		lua_pushnumber(L, value);
	}

	static double get(lua_State *L, int index)
	{
		return lua_tonumber(L, index);
	}

	static double check(lua_State *L, int index)
	{
		return luaL_checknumber(L, index);
	}
};

template <>
struct Luae::Convert<std::string> {
	static const bool supported = true;

	static void push(lua_State *L, const std::string &value)
	{
		lua_pushlstring(L, value.c_str(), value.length());
	}

	static std::string get(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	static std::string check(lua_State *L, int index)
	{
		return luaL_checkstring(L, index);
	}
};

template <>
struct Luae::Convert<const char *> {
	static const bool supported = true;

	static void push(lua_State *L, const char *value)
	{
		lua_pushstring(L, value);
	}

	static const char *get(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	static const char *check(lua_State *L, int index)
	{
		return luaL_checkstring(L, index);
	}
};

/* }}} */

/* {{{ LuaeState */

/**
 * @class LuaeState
 * @brief Wrapper for lua_State
 *
 * This class automatically create a new Lua state and add implicit
 * cast operator plus RAII destruction.
 */
class LuaeState {
private:
	struct Deleter {
		void operator()(lua_State *L)
		{
			lua_close(L);
		}
	};

	using Ptr = std::unique_ptr<lua_State, Deleter>;

	Ptr m_state;

	void initRegistry();

public:
	/*
	 * FieldRefs:		The field stored into the registry to avoid
	 *			recreation of shared objects.
	 */
	static const char *FieldRefs;

	LuaeState(const LuaeState &) = delete;
	LuaeState &operator=(const LuaeState &) = delete;

	/**
	 * Default constructor. Create a new state.
	 */
	LuaeState();

	/**
	 * Use the already created state.
	 *
	 * @param L the state to use
	 */
	LuaeState(lua_State *L);

	/**
	 * Move constructor.
	 *
	 * @param state the Lua state to move
	 */
	LuaeState(LuaeState &&state);

	/**
	 * Move assignment operator.
	 *
	 * @param state the Lua state to move
	 */
	LuaeState &operator=(LuaeState &&state);

	/**
	 * Implicit cast operator for convenient usage to C Lua API.
	 *
	 * @return the state as lua_State *
	 */
	operator lua_State*();
};

/* }}} */

/* {{{ LuaeValue */

/**
 * @class LuaeValue
 * @brief A fake variant for Lua values
 *
 * This class is primarly used for copying Lua values without checking
 * the types, useful to pass data.
 */
class LuaeValue {
private:
	union {
		lua_Number	 number;
		bool		 boolean;
	};

	int type;
	std::string str;
	std::vector<std::pair<LuaeValue, LuaeValue>> table;

public:
	/**
	 * Dump a value at the specific index.
	 *
	 * @param L the Lua state
	 * @param index the value
	 * @return a tree of values
	 */
	static LuaeValue copy(lua_State *L, int index);

	/**
	 * Push a value to a state.
	 *
	 * @param L the Lua state
	 * @param value the value to push
	 */
	static void push(lua_State *L, const LuaeValue &value);

	/**
	 * Default constructor (type nil)
	 */
	LuaeValue();
};

/* }}} */

/* {{{ LuaeClass */

/**
 * @class LuaeClass
 * @brief Support for object oriented programming between C++ and Lua
 *
 * This class provides functions for passing and retrieving objects from C++ and
 * Lua.
 */
class LuaeClass {
public:
	using Methods	= std::vector<luaL_Reg>;

	template <typename T>
	using Ptr	= std::shared_ptr<T>;

	struct Def {
		std::string	name;		//! metatable name
		Methods		methods;	//! methods
		Methods		metamethods;	//! metamethods
		const Def	*parent;	//! optional parent class
	};

	/*
	 * FieldName:		The field stored in the object metatable about 
	 *			the object metatable name.
	 *
	 * FieldParents:	The field that holds all parent classes. It is
	 *			used to verify casts.
	 */
	static const char *FieldName;
	static const char *FieldParents;

	/**
	 * Initialize a new object.
	 *
	 * @param L the Lua state
	 * @param def the definition
	 */
	static void create(lua_State *L, const Def &def);

	/**
	 * Push a shared object to Lua, it also push it to the "__refs"
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
	static void pushShared(lua_State *L, Ptr<T> o, const std::string &name)
	{
		LUAE_STACK_CHECKBEGIN(L);

		lua_getfield(L, LUA_REGISTRYINDEX, LuaeState::FieldRefs);
		assert(lua_type(L, -1) == LUA_TTABLE);

		lua_rawgetp(L, -1, o.get());

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_pop(L, 1);

			new (L, name.c_str()) std::shared_ptr<T>(o);
			
			lua_pushvalue(L, -1);
			lua_rawsetp(L, -3, o.get());
		}

		lua_replace(L, -2);

		LUAE_STACK_CHECKEND(L, -1);
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
	static Ptr<T> getShared(lua_State *L, int index, const char *meta)
	{
		LUAE_STACK_CHECKBEGIN(L);

		luaL_checktype(L, index, LUA_TUSERDATA);
		if (!luaL_getmetafield(L, index, FieldName))
			luaL_error(L, "invalid type cast");

		// Get the class name
		const char *name = lua_tostring(L, -1);
		lua_pop(L, 1);

		bool found(false);

		if (std::string(name) == std::string(meta)) {
			found = true;
		} else {
			if (!luaL_getmetafield(L, index, FieldParents))
				luaL_error(L, "invalid type cast");

			lua_pushnil(L);
			while (lua_next(L, -2) != 0) {
				if (lua_type(L, -2) != LUA_TSTRING) {
					lua_pop(L, 1);
					continue;
				}

				auto tn = lua_tostring(L, -1);
				if (std::string(tn) == std::string(meta))
					found = true;

				lua_pop(L, 1);
			}
	
			lua_pop(L, 1);
		}

		if (!found)
			luaL_error(L, "invalid cast from `%s' to `%s'", name, meta);

		LUAE_STACK_CHECKEQUALS(L);
		
		return *static_cast<Ptr<T> *>(lua_touserdata(L, index));
	}

	/**
	 * Delete the shared pointer at the given index. This function does
	 * not check if the type is valid for performance reason. And because
	 * it's usually called in __gc, there is no reason to check.
	 *
	 * Also return 0 the __gc method can directly call
	 * return LuaeClass::deleteShared(...)
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return 0 for convenience
	 */
	template <typename T>
	static int deleteShared(lua_State *L, int index)
	{
		LUAE_STACK_CHECKBEGIN(L);

		Luae::toType<Ptr<T> *>(L, index)->~shared_ptr<T>();

		LUAE_STACK_CHECKEQUALS(L);

		return 0;
	}
};

/* }}} */

/* {{{ LuaeTable */

/**
 * @class LuaeTable
 * @brief Some function for table manipulation
 *
 * Read, reference and get fields from tables.
 */
class LuaeTable {
public:
	using ReadFunction = std::function<void(lua_State *L, int tkey, int tvalue)>;

	/**
	 * Push a new table onto the stack.
	 *
	 * @param L the Lua state
	 * @param nrec the optional number of entries as record
	 * @param nrec the optional number of entries as sequence
	 */
	static void create(lua_State *L, int nrec = 0, int narr = 0);

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
	static T get(lua_State *L, int idx, const std::string &name)
	{
		LUAE_STACK_CHECKBEGIN(L);

		lua_getfield(L, idx, name.c_str());
		auto t = Luae::get<T>(L, -1);
		lua_pop(L, 1);

		LUAE_STACK_CHECKEQUALS(L);

		return t;
	}

	/**
	 * Set a field to the table at the given index.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @see set
	 */
	static inline void setfield(lua_State *L, int idx, const std::string &name)
	{
		lua_setfield(L, idx, name.c_str());
	}

	/**
	 * Get a field at the given index.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 */
	static inline void getfield(lua_State *L, int idx, const std::string &name)
	{
		lua_getfield(L, idx, name.c_str());
	}

	/**
	 * Set a table field. Specialized for the same fields as get.
	 *
	 * @param L the Lua state
	 * @param idx the index
	 * @param name the field name
	 * @param value the value
	 * @see get
	 */
	template <typename T>
	static void set(lua_State *L, int idx, const std::string &name, const T &value)
	{
		LUAE_STACK_CHECKBEGIN(L);

		Luae::push(L, value);
		LuaeTable::setfield(L, (idx < 0) ? --idx : idx, name);

		LUAE_STACK_CHECKEQUALS(L);
	}

	/**
	 * Overload for string literals.
	 *
	 * @param L the Lua state
	 * @param idx the index
	 * @param name the field name
	 * @param s the string
	 */
	template <size_t N>
	static void set(lua_State *L, int idx, const std::string &name, const char (&s)[N])
	{
		set<const char *>(L, idx, name, s);
	}

	/**
	 * Set a class object to a table.
	 *
	 * @param L the Lua state
	 * @param index the table index
	 * @param name the field name
	 * @param meta the metatable name
	 * @param o the object
	 */
	template <typename T>
	static void setShared(lua_State *L,
			      int index,
			      const std::string &name,
			      const std::string &meta,
			      LuaeClass::Ptr<T> o)
	{
		LUAE_STACK_CHECKBEGIN(L);

		LuaeClass::pushShared(L, o, meta);
		LuaeTable::setfield(L, (index < 0) ? --index : index, name);

		LUAE_STACK_CHECKEQUALS(L);
	}

	/**
	 * Require a field from a table.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @return the value or call luaL_error
	 */
	template <typename T>
	static T require(lua_State *L, int idx, const std::string &name)
	{
		LUAE_STACK_CHECKBEGIN(L);

		lua_getfield(L, idx, name.c_str());

		if (lua_type(L, -1) == LUA_TNIL)
			luaL_error(L, "missing field `%s'", name.c_str());
			// NOT REACHED

		lua_pop(L, 1);
		auto v = get<T>(L, idx, name);

		LUAE_STACK_CHECKEQUALS(L);

		return v;
	}

	/**
	 * Check a table field.
	 *
	 * @param L the Lua state
	 * @param idx the table index
	 * @param name the field name
	 * @return the type
	 */
	static int type(lua_State *L, int idx, const std::string &name);

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
	static void read(lua_State *L, int idx, ReadFunction func);

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
	static int ref(lua_State *L,
		       int idx,
		       int type,
		       const std::string &name);
};

/* }}} */

/* {{{ LuaeEnum */

/**
 * @class LuaeEnum
 * @brief Binds and get enumeration
 *
 * Bind C/C++ enumeration as tables to Lua in the key-value form. Two
 * methods push and get are also available for enumeration flags.
 */
class LuaeEnum {
public:
	/**
	 * The definition of the enumeration
	 */
	using Def = std::unordered_map<std::string, int>;

	/**
	 * Bind the enumeration as a table into an existing table.
	 *
	 * @param L the Lua state
	 * @param def the definition
	 * @param index the table index
	 * @param name the field name to store the enumeration
	 */
	static void create(lua_State *L,
			   const Def &def,
			   int index,
			   const std::string &name);

	/**
	 * Push the value enumeration as a table to Lua. This is used
	 * as the OR replacement.
	 *
	 * @param L the Lua state
	 * @param def the definition
	 * @param value the value
	 */
	static void push(lua_State *L, const Def &def, int value);

	/**
	 * Get the enumeration from Lua. Raises an error if the
	 * value at the index is not a table.
	 *
	 * This is used as the OR replacement.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the value
	 */
	static int get(lua_State *L, int index);
};

/* }}} */

} // !irccd

void *operator new(size_t size, lua_State *L);

void *operator new(size_t size, lua_State *L, const char *metaname);

void operator delete(void *ptr, lua_State *L);

void operator delete(void *ptr, lua_State *L, const char *metaname);

#endif // !_LUA_H_
