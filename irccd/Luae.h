/*
 * Luae.h -- Lua helpers and such
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#ifndef _LUAE_H_
#define _LUAE_H_

/**
 * @file Luae.h
 * @brief Lua C++ extended API
 */

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

/**
 * Store the current stack size. Should be called at the beginning of a
 * function.
 *
 * @param L the Lua state
 */
#define LUAE_STACK_CHECKBEGIN(L)					\
	int __topstack = lua_gettop((L))

/**
 * Check if the current stack size match the beginning. LUAE_STACK_CHECKBEGIN
 * must have been called.
 *
 * @param L the Lua state
 */
#define LUAE_STACK_CHECKEQUALS(L)					\
	assert(lua_gettop((L)) == __topstack)

/**
 * Check if the current stack size match the condition. LUAE_STACK_CHECKBEGIN
 * must have been called.
 *
 * @param L the Lua state
 * @param cond the condition
 */
#define LUAE_STACK_CHECKEND(L, cond)					\
	assert(lua_gettop((L)) cond == __topstack)

#else

/**
 * Store the current stack size. Should be called at the beginning of a
 * function.
 *
 * @param L the Lua state
 */
#define LUAE_STACK_CHECKBEGIN(L)

/**
 * Check if the current stack size match the beginning. LUAE_STACK_CHECKBEGIN
 * must have been called.
 *
 * @param L the Lua state
 */
#define LUAE_STACK_CHECKEQUALS(L)

/**
 * Check if the current stack size match the condition. LUAE_STACK_CHECKBEGIN
 * must have been called.
 *
 * @param L the Lua state
 * @param cond the condition
 */
#define LUAE_STACK_CHECKEND(L, cond)

#endif

namespace irccd {

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
	/**
	 * The field stored into the registry to avoid recreation of shared
	 * objects.
	 */
	static const char *FieldRefs;

	/**
	 * Deleted copy constructor.
	 */
	LuaeState(const LuaeState &) = delete;

	/**
	 * Deleted copy assignment.
	 */
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
	/**
	 * Methods for a class.
	 */
	using Methods	= std::vector<luaL_Reg>;

	/**
	 * Smart pointer for Luae objects.
	 */
	template <typename T>
	using Ptr	= std::shared_ptr<T>;

	/**
	 * @struct Def
	 * @brief Definition of a class
	 */
	struct Def {
		std::string	name;		//!< metatable name
		Methods		methods;	//!< methods
		Methods		metamethods;	//!< metamethods
		const Def	*parent;	//!< optional parent class
	};

	/**
	 * The field stored in the object metatable about the object metatable
	 * name.
	 */
	static const char *FieldName;

	/**
	 * The field that holds all parent classes. It is used to verify casts.
	 */
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
	 * Check if the object at index is suitable for cast to meta. Calls
	 * luaL_error if not.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @param meta the object name
	 */
	static void testShared(lua_State *L, int index, const char *meta);

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
		testShared(L, index, meta);
		
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

		static_cast<Ptr<T> *>(lua_touserdata(L, index))->~shared_ptr();

		LUAE_STACK_CHECKEQUALS(L);

		return 0;
	}
};

/* }}} */

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
	 * Map from string to function.
	 */
	using Reg	= std::unordered_map<const char *, lua_CFunction>;

	/**
	 * @struct Convert
	 * @brief Push or get values
	 */
	template <typename T>
	struct Convert {
		static const bool hasPush	= false;	//!< has push function
		static const bool hasGet	= false;	//!< has get function
		static const bool hasCheck	= false;	//!< has check function
	};

	/**
	 * @struct Iterator
	 * @brief Wrap STL containers
	 */
	template <typename Type>
	struct Iterator {
		Type	begin;				//!< the beginning
		Type	end;				//!< the end
		Type	current;			//!< the current index

		/**
		 * Construct the iterator.
		 *
		 * @param begin the beginning
		 * @param end the end
		 */
		Iterator(Type begin, Type end)
			: begin(begin)
			, end(end)
			, current(begin)
		{
		}
	};

	/**
	 * Test if the object can be pushed as a userdata. If the object can
	 * be pushed as a userdata, it must match the following requirements:
	 *
	 *	- Copy constructible
	 *	- IsUserdata overload must have const char *MetatableName
	 *
	 * ## Userdata object
	 *
	 * The following example code allows the class Object to be pushed
	 * and get as Lua userdata.
	 *
	 * @code
	 * struct Object { };
	 *
	 * template <>
	 * struct Luae::IsUserdata<Object> : std::true_type {
	 * 	static const char *MetatableName;
	 * };
	 *
	 * const char *Luae::IsUserdata<Object>::MetatableName = "Object";
	 *
	 * int l_push(lua_State *L)
	 * {
	 * 	Luae::push(L, Object());
	 * }
	 *
	 * int l_get(lua_State *L)
	 * {
	 * 	Object *o = Luae::get<Object>(L, 1);
	 * }
	 * @endcode
	 * @note You don't need to add the pointer type to the get template parameter
	 *
	 * ## Custom object
	 *
	 * This other example can be used to push custom objects but not as
	 * userdata. You can use this to push and read tables for instance.
	 *
	 * @code
	 * struct Point {
	 * 	int x, y;
	 * };
	 *
	 * template <>
	 * struct Luae::Convert<Point> {
	 * 	static const bool hasPush = true;
	 * 	static const bool hasGet = true;
	 * 	static const bool hasCheck = true;
	 *
	 * 	static void push(lua_State *L, const Point &p)
	 * 	{
	 * 		lua_createtable(L, 0, 0);
	 * 		lua_pushinteger(L, p.x);
	 * 		lua_setfield(L, -2, "x");
	 * 		lua_pushinteger(L, p.y);
	 * 		lua_setfield(L, -2, "y");
	 * 	}
	 *
	 * 	static Point get(lua_State *L, int index)
	 * 	{
	 * 		Point p;
	 *
	 * 		if (lua_type(L, index) == LUA_TTABLE) {
	 * 			lua_getfield(L, index, "x");
	 * 			p.x = lua_tonumber(L, -1);
	 * 			lua_pop(L, 1);
	 * 			lua_getfield(L, index, "y");
	 * 			p.y = lua_tonumber(L, -1);
	 * 			lua_pop(L, 1);
	 * 		}
	 *
	 * 		return p;
	 * 	}
	 *
	 * 	static Point check(lua_State *L, int index)
	 * 	{
	 * 		// Do your check
	 *
	 * 		return get(L, index);
	 * 	}
	 * };
	 *
	 * int l_push(lua_State *L)
	 * {
	 * 	Luae::push<Point>(L, Point {1, 2});
	 * }
	 *
	 * int l_get(lua_State *L)
	 * {
	 * 	Point p = Luae::get<Point>(L, 1);
	 * }
	 * @endcode
	 * @note Here you get a T and not a T *
	 */
	template <typename T>
	struct IsUserdata : std::false_type { };

private:
	template <typename T>
	struct IsSharedUserdata : std::false_type { };

	template <typename T>
	struct IsSharedUserdata<std::shared_ptr<T>> {
		static const bool value = IsUserdata<T>::value;
	};

public:
	/* -------------------------------------------------
	 * Standard Lua API wrappers
	 * ------------------------------------------------- */

	/**
	 * Calls a Lua function in non-protected mode.
	 *
	 * @param L the Lua state
	 * @param np the number of parameters
	 * @param nr the number of return values
	 */
	static inline void call(lua_State *L, int np = 0, int nr = 0)
	{
		lua_call(L, np, nr);
	}

	/**
	 * Ensure that there are at least extra free stack slots in the stack.
	 *
	 * @param L the Lua state
	 * @param extra the extra data
	 * @return true if possible
	 */
	static inline int checkstack(lua_State *L, int extra)
	{
		return lua_checkstack(L, extra);
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
	 * Compares two Lua values.
	 *
	 * Operation is one of:
	 *
	 * * LUA_OPEQ,
	 * * LUA_OPLT,
	 * * LUA_OPLE
	 *
	 * @param L the Lua state
	 * @param index1 the first value
	 * @param index2 the second value
	 * @param op the operation
	 * @return true if index1 statisfies op compared to index2
	 */
	static inline bool compare(lua_State *L, int index1, int index2, int op)
	{
		return lua_compare(L, index1, index2, op) == 1;
	}

	/**
	 * Concatenate the n values at the top of the stack and pops them.
	 * Leaves the result at the top of the stack.
	 *
	 * @param L the Lua state
	 * @param n the number to concat
	 */
	static inline void concat(lua_State *L, int n)
	{
		lua_concat(L, n);
	}

	/**
	 * Moves the element at the index from into the valid index to.
	 *
	 * @param L the Lua state
	 * @param from the from index
	 * @param to the destination index
	 */
	static inline void copy(lua_State *L, int from, int to)
	{
		lua_copy(L, from, to);
	}

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
	 * Generate an error with the string at the top of the stack.
	 *
	 * @param L the Lua state
	 * @return nothing
	 */
	static inline int error(lua_State *L)
	{
		return lua_error(L);
	}

	/**
	 * Raises a Lua error, thus calling longjmp.
	 *
	 * @param L the Lua state
	 * @param fmt the format
	 * @param args the arguments
	 * @return nothing
	 */
	template <typename... Args>
	static inline int error(lua_State *L, const char *fmt, Args&&... args)
	{
		return luaL_error(L, fmt, std::forward<Args>(args)...);
	}

	/**
	 * Controls the garbage collector.
	 *
	 * @param L the Lua state
	 * @param what the action
	 * @param data optional GC data
	 */
	static inline int gc(lua_State *L, int what, int data = 0)
	{
		return lua_gc(L, what, data);
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
	 * Get the metatable of the value at the given index. Returns false
	 * if does not have a metatable and pushes nothing.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline bool getmetatable(lua_State *L, int index)
	{
		return lua_getmetatable(L, index) == 1;
	}

	/**
	 * Set the value at the given index. Top value is the value to assign
	 * key is just below the value.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void gettable(lua_State *L, int index)
	{
		lua_gettable(L, index);
	}

	/**
	 * Get the current stack size.
	 *
	 * @param L the Lua state
	 * @return the stack size
	 */
	static inline int gettop(lua_State *L)
	{
		return lua_gettop(L);
	}

	/**
	 * Pushes the Lua value associated with the userdata.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void getuservalue(lua_State *L, int index)
	{
		lua_getuservalue(L, index);
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
	 * Push the result of the operator '#' from the value at the given
	 * index.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void len(lua_State *L, int index)
	{
		lua_len(L, index);
	}

	/**
	 * Create or get a metatable in the registry.
	 *
	 * @param L the Lua state
	 * @param name the metatable name
	 */
	static inline void newmetatable(lua_State *L, const std::string &name)
	{
		luaL_newmetatable(L, name.c_str());
	}

	/**
	 * Create a new table and fill it with functions.
	 *
	 * @param L the Lua state
	 * @param functions the functions
	 */
	static inline void newlib(lua_State *L, const luaL_Reg *functions)
	{
		lua_createtable(L, 0, 0);
		luaL_setfuncs(L, functions, 0);
	}

	/**
	 * Create a new table and fill it with functions.
	 *
	 * @param L the Lua state
	 * @param functions the functions
	 */
	static inline void newlib(lua_State *L, const Reg &functions)
	{
		lua_createtable(L, 0, 0);
		for (auto &p : functions) {
			lua_pushcfunction(L, p.second);
			lua_setfield(L, -2, p.first);
		}
	}

	/**
	 * Pops a key from the stack and pushes a key-value pair from the table
	 * at the given index.
	 *
	 * @param L the Lua state
	 * @param index the table index
	 * @return true if there are still elements
	 */
	static inline bool next(lua_State *L, int index)
	{
		return lua_next(L, index) == 1;
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
	 * Pop arguments from the stack.
	 *
	 * @param L the Lua state
	 * @param count the number of values to pop
	 */
	static inline void pop(lua_State *L, int count = 1)
	{
		lua_pop(L, count);
	}

	/**
	 * Pushes a copy of the value at the given index.
	 *
	 * @param L the Lua state
	 * @param index the value to copy
	 */
	static inline void pushvalue(lua_State *L, int index)
	{
		lua_pushvalue(L, index);
	}

	/**
	 * Returns true if the values are primitively equal.
	 *
	 * @param L the Lua state
	 * @param index1 the first value
	 * @param index2 the second value
	 * @return true if they equals
	 */
	static inline bool rawequal(lua_State *L, int index1, int index2)
	{
		return lua_rawequal(L, index1, index2) == 1;
	}

	/**
	 * Like gettable but with raw access.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void rawget(lua_State *L, int index)
	{
		lua_rawget(L, index);
	}

	/**
	 * Like gettable but with raw access.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @param n the nt
	 */
	static inline void rawget(lua_State *L, int index, int n)
	{
		lua_rawgeti(L, index, n);
	}

	/**
	 * Like rawgeti but with pointer.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @param p the pointer key
	 */
	static inline void rawget(lua_State *L, int index, const void *p)
	{
		lua_rawgetp(L, index, p);
	}

	/**
	 * Get the value length.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the raw length
	 */
	static inline int rawlen(lua_State *L, int index)
	{
		return lua_rawlen(L, index);
	}

	/**
	 * Similar to settable but without raw assignment.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void rawset(lua_State *L, int index)
	{
		lua_rawset(L, index);
	}

	/**
	 * Set the value at the top of stack to the ntn value at the value
	 * at the given index.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @param n the nth index
	 */
	static inline void rawset(lua_State *L, int index, int n)
	{
		lua_rawseti(L, index, n);
	}

	/**
	 * Like rawseti with a void pointer as the key.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @param ptr the pointer key
	 */
	static inline void rawset(lua_State *L, int index, const void *ptr)
	{
		lua_rawsetp(L, index, ptr);
	}

	/**
	 * Push a formatted string like lua_pushfstring. Warning, it is not type
	 * safe and you should for instance not pass std::String to %s.
	 *
	 * @param L the Lua state
	 * @param fmt the format
	 * @param args the arguments
	 * @return the formatted string
	 */
	template <typename... Args>
	static inline const char *pushfstring(lua_State *L, const char *fmt, Args&&... args)
	{
		return lua_pushfstring(L, fmt, std::forward<Args>(args)...);
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
	 * Set the functions to the table at the top of stack.
	 *
	 * @param L the Lua state
	 * @param functions the functions
	 * @param nup the number of upvalues
	 */
	static inline void setfuncs(lua_State *L, const luaL_Reg *functions, int nup = 0)
	{
		luaL_setfuncs(L, functions, nup);
	}

	/**
	 * Set the functions to the table at the top of stack.
	 *
	 * @param L the Lua state
	 * @param functions the functions
	 * @param nup the number of upvalues
	 */
	static inline void setfuncs(lua_State *L, const Reg &functions, int nup = 0)
	{
		luaL_checkversion(L);
		luaL_checkstack(L, nup, "too many upvalues");

		for (auto &l : functions) {
			for (int i = 0; i < nup; i++)
				lua_pushvalue(L, -nup);
			lua_pushcclosure(L, l.second, nup);
			lua_setfield(L, -(nup + 2), l.first);
		}

		lua_pop(L, nup);
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
	 * Pops the table at the top of the stack and sets it as the metatable
	 * of the value at the given index.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void setmetatable(lua_State *L, int index)
	{
		lua_setmetatable(L, index);
	}

	/**
	 * Does t[n] where n is the value at the top of the stack and the key
	 * just below the value.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 */
	static inline void settable(lua_State *L, int index)
	{
		lua_settable(L, index);
	}

	/**
	 * Set the stack size.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static inline void settop(lua_State *L, int index = 0)
	{
		lua_settop(L, index);
	}

	/**
	 * Pops a table or nil from the top stack and set it as the new
	 * associated value to the userdata.
	 *
	 * @param L the Lua state
	 * @param index the userdata index
	 */
	static inline void setuservalue(lua_State *L, int index)
	{
		lua_setuservalue(L, index);
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
	 * Get the type name.
	 *
	 * @param L the Lua state
	 * @param type the type
	 * @return the name
	 * @see type
	 */
	static inline const char *typeName(lua_State *L, int type)
	{
		return lua_typename(L, type);
	}

	/**
	 * Get the type name from a index.
	 *
	 * @param L the Lua state
	 * @param index the value index
	 * @return the name
	 */
	static inline const char *typeNamei(lua_State *L, int index)
	{
		return luaL_typename(L, index);
	}

	/**
	 * Unref the value from the table.
	 *
	 * @param L the Lua state
	 * @param index the table index
	 * @param ref the reference
	 */
	static inline void unref(lua_State *L, int index, int ref)
	{
		luaL_unref(L, index, ref);
	}

	/**
	 * Get the up value index.
	 *
	 * @param index the index
	 * @return the real index
	 */
	static inline int upvalueindex(int index)
	{
		return lua_upvalueindex(index);
	}

	/* -------------------------------------------------
	 * Extended API
	 * ------------------------------------------------- */

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
	 * Push standard objects. These objects are usually primitives types
	 * or objects that does not have LuaName field. They are usually
	 * mapped to the Lua type or tables.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	template <typename T>
	static void push(lua_State *L,
			 const T &value,
			 typename std::enable_if<!IsUserdata<T>::value, T>::type * = 0,
			 typename std::enable_if<!IsSharedUserdata<T>::value, T>::type * = 0)
	{
		static_assert(Convert<T>::hasPush, "type not supported");

		Convert<T>::push(L, value);
	}

	/**
	 * Push objects as userdata. The object must be copy constructible. This
	 * overload is enabled when the object has a LuaName static field.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	template <typename T>
	static void push(lua_State *L,
			 const T &value,
			 typename std::enable_if<IsUserdata<T>::value, T>::type * = 0)
	{
		new (L, IsUserdata<T>::MetatableName) T(value);
	}

	/**
	 * Works like userdata excepts that it use shared pointer objects. It
	 * use LuaeClass::pushShared.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	template <typename T, typename Type = typename T::element_type>
	static void push(lua_State *L,
			 const T &value,
			 typename std::enable_if<IsSharedUserdata<T>::value>::type * = 0)
	{
		LuaeClass::pushShared<Type>(L, value, IsUserdata<Type>::MetatableName);
	}

	/**
	 * Overload for string literals and arrays.
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
	 * Get the data, the type is not checked.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a T value
	 */
	template <typename T>
	static T
	get(lua_State *L,
	    int index,
	    typename std::enable_if<!IsUserdata<T>::value, T>::type * = 0,
	    typename std::enable_if<!IsSharedUserdata<T>::value, T>::type * = 0)
	{
		static_assert(Convert<T>::hasGet, "type not supported");

		return Convert<T>::get(L, index);
	}

	/**
	 * Get a userdata object. The type is not checked and just cast.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a T * value
	 */
	template <typename T>
	static T *
	get(lua_State *L,
	    int index,
	    typename std::enable_if<IsUserdata<T>::value, T *>::type * = 0)
	{
		return Luae::toType<T *>(L, index);
	}

	/**
	 * Get a shared pointer object created from LuaeClass. The type is not
	 * checked.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a std::shared_ptr<>
	 */
	template <typename T, typename Type = typename T::element_type>
	static std::shared_ptr<Type>
	get(lua_State *L,
	   int index,
	   typename std::enable_if<IsSharedUserdata<T>::value, T>::type * = 0)
	{
		return LuaeClass::getShared<Type>(L, index); 
	}

	/**
	 * Check for a data. The type is checked.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a T value
	 */
	template <typename T>
	static T
	check(lua_State *L,
	      int index,
	      typename std::enable_if<!IsUserdata<T>::value, T>::type * = 0,
	      typename std::enable_if<!IsSharedUserdata<T>::value, T>::type * = 0)
	{
		static_assert(Convert<T>::hasCheck, "type not supported");

		return Convert<T>::check(L, index);
	}

	/**
	 * Check for a userdata object.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a T * value
	 */
	template <typename T>
	static T *
	check(lua_State *L,
	      int index,
	      typename std::enable_if<IsUserdata<T>::value, T *>::type * = 0)
	{
		return Luae::toType<T *>(L, index, IsUserdata<T>::MetatableName);
	}

	/**
	 * Check for a shared pointer object created from LuaeClass.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a std::shared_ptr<>
	 */
	template <typename T, typename Type = typename T::element_type>
	static std::shared_ptr<Type>
	check(lua_State *L,
	      int index,
	      typename std::enable_if<IsSharedUserdata<T>::value, T>::type * = 0)
	{
		return LuaeClass::getShared<Type>(L, index, IsUserdata<Type>::MetatableName); 
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

				Luae::push(L, *(it->current++));

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
};

/**
 * @brief Overload for nil.
 */
template <>
struct Luae::Convert<std::nullptr_t> {
	static const bool hasPush	= true;	//!< push supported

	/**
	 * Push nil.
	 *
	 * @param L the Lua state
	 */
	static void push(lua_State *L, const std::nullptr_t &)
	{
		lua_pushnil(L);
	}
};

/**
 * @brief Overload for booleans.
 */
template <>
struct Luae::Convert<bool> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the boolean value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const bool &value)
	{
		lua_pushboolean(L, value);
	}

	/**
	 * Get a boolean.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static bool get(lua_State *L, int index)
	{
		return lua_toboolean(L, index);
	}

	/**
	 * Check for a bool.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static bool check(lua_State *L, int index)
	{
		return lua_toboolean(L, index);
	}
};

/**
 * @brief Overload for integers.
 */
template <>
struct Luae::Convert<int> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the integer value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const int &value)
	{
		lua_pushinteger(L, value);
	}

	/**
	 * Get a integer.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static int get(lua_State *L, int index)
	{
		return lua_tointeger(L, index);
	}

	/**
	 * Check for an integer.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static int check(lua_State *L, int index)
	{
		return luaL_checkinteger(L, index);
	}
};

/**
 * @brief Overload for longs.
 */
template <>
struct Luae::Convert<long> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the integer value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const long &value)
	{
		lua_pushinteger(L, value);
	}

	/**
	 * Get a integer.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static long get(lua_State *L, int index)
	{
		return lua_tointeger(L, index);
	}

	/**
	 * Check for an integer.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static long check(lua_State *L, int index)
	{
		return luaL_checkinteger(L, index);
	}
};

/**
 * @brief Overload for doubles.
 */
template <>
struct Luae::Convert<double> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the double value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const double &value)
	{
		lua_pushnumber(L, value);
	}

	/**
	 * Get a double.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static double get(lua_State *L, int index)
	{
		return lua_tonumber(L, index);
	}

	/**
	 * Check for a double.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static double check(lua_State *L, int index)
	{
		return luaL_checknumber(L, index);
	}
};

/**
 * @brief Overload for std::string.
 */
template <>
struct Luae::Convert<std::string> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the string value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const std::string &value)
	{
		lua_pushlstring(L, value.c_str(), value.length());
	}

	/**
	 * Get a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static std::string get(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	/**
	 * Check for a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static std::string check(lua_State *L, int index)
	{
		return luaL_checkstring(L, index);
	}
};

/**
 * @brief Overload for std::u32string.
 */
template <>
struct Luae::Convert<std::u32string> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the string value.
	 *
	 * @param L the Lua state
	 * @param str the value
	 */
	static void push(lua_State *L, const std::u32string &str)
	{
		lua_createtable(L, str.size(), 0);
		for (size_t i = 0; i < str.size(); ++i) {
			lua_pushinteger(L, str[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	/**
	 * Get a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static std::u32string get(lua_State *L, int index)
	{
		std::u32string result;

		if (lua_type(L, index) == LUA_TNUMBER) {
			result.push_back(lua_tonumber(L, index));
		} else if (lua_type(L, index) == LUA_TTABLE) {
			if (index < 0)
				-- index;

			lua_pushnil(L);
			while (lua_next(L, index)) { 
				if (lua_type(L, -1) == LUA_TNUMBER)
					result.push_back(lua_tonumber(L, -1));

				lua_pop(L, 1);
			}
		}

		return result;
	}

	/**
	 * Check for a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static std::u32string check(lua_State *L, int index)
	{
		if (lua_type(L, index) != LUA_TNUMBER &&
		    lua_type(L, index) != LUA_TTABLE)
			luaL_error(L, "expected table or number");
			// NOTREACHED

		return get(L, index);
	}
};

/**
 * @brief Overload for string list
 */
template <>
struct Luae::Convert<std::vector<std::string>> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true; //!< get supported
	static const bool hasCheck	= true; //!< check supported

	/**
	 * Push a string list.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const std::vector<std::string> &value)
	{
		int i = 0;

		lua_createtable(L, value.size(), 0);
		for (const auto &s : value) {
			lua_pushlstring(L, s.c_str(), s.length());
			lua_rawseti(L, -2, ++i);
		}
	}

	/**
	 * Get a string list.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return the list
	 */
	static std::vector<std::string> get(lua_State *L, int index)
	{
		std::vector<std::string> list;

		if (index < 0)
			-- index;

		lua_pushnil(L);
		while (lua_next(L, index)) { 
			if (lua_type(L, -1) == LUA_TSTRING)
				list.push_back(lua_tostring(L, -1));

			lua_pop(L, 1);
		}

		return list;
	}

	/**
	 * Check for a string list.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return the list
	 */
	static std::vector<std::string> check(lua_State *L, int index)
	{
		luaL_checktype(L, index, LUA_TTABLE);

		return get(L, index);
	}
};

/**
 * @brief Overload for const char *
 */
template <>
struct Luae::Convert<const char *> {
	static const bool hasPush	= true;	//!< push supported
	static const bool hasGet	= true;	//!< get supported
	static const bool hasCheck	= true;	//!< check supported

	/**
	 * Push the string value.
	 *
	 * @param L the Lua state
	 * @param value the value
	 */
	static void push(lua_State *L, const char *value)
	{
		lua_pushstring(L, value);
	}

	/**
	 * Get a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return a boolean
	 */
	static const char *get(lua_State *L, int index)
	{
		return lua_tostring(L, index);
	}

	/**
	 * Check for a string.
	 *
	 * @param L the Lua state
	 * @param index the index
	 */
	static const char *check(lua_State *L, int index)
	{
		return luaL_checkstring(L, index);
	}
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

/* {{{ LuaeTable */

/**
 * @class LuaeTable
 * @brief Some function for table manipulation
 *
 * Read, reference and get fields from tables.
 */
class LuaeTable {
public:
	/**
	 * The map function for \ref read
	 */
	using ReadFunction = std::function<void(lua_State *L, int tkey, int tvalue)>;

	/**
	 * Push a new table onto the stack.
	 *
	 * @param L the Lua state
	 * @param nrec the optional number of entries as record
	 * @param narr the optional number of entries as sequence
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
		Luae::setfield(L, (idx < 0) ? --idx : idx, name);

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
	using Def = std::unordered_map<const char *, int>;

	/**
	 * Bind the enumeration and keep it at the top of the stack.
	 *
	 * @param L the Lua state
	 * @param def the definition
	 */
	static void create(lua_State *L, const Def &def);

	/**
	 * Set the enumeration values to an existing table.
	 *
	 * @param L the Lua state
	 * @param def the definition
	 * @param index the table index
	 */
	static void create(lua_State *L, const Def &def, int index);

	/**
	 * Create the enumeration table and set it to a table field.
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

/**
 * Push a Lua userdata.
 *
 * @param size the object size
 * @param L the Lua state
 * @return the allocated data
 */
void *operator new(size_t size, lua_State *L);

/**
 * Push a Lua userdata with a metatable.
 *
 * @param size the object size
 * @param L the Lua state
 * @param metaname the metatable name
 * @return the allocated data
 */
void *operator new(size_t size, lua_State *L, const char *metaname);

/**
 * Delete the Lua userdata.
 *
 * @param ptr the data
 * @param L the Lua state
 */
void operator delete(void *ptr, lua_State *L);

/**
 * Delete the Lua userdata.
 *
 * @param ptr the data
 * @param L the Lua state
 * @param metaname the metatable name
 */
void operator delete(void *ptr, lua_State *L, const char *metaname);

#endif // !_LUAE_H_
