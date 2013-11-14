/*
 * LuaThread.cpp -- Lua bindings for threads
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

#include <string>

#include "Irccd.h"
#include "Logger.h"
#include "Luae.h"
#include "LuaThread.h"
#include "Plugin.h"

namespace irccd
{

namespace
{

/* ---------------------------------------------------------
 * Buffer management for loading threads
 * --------------------------------------------------------- */

struct Buffer
{
	std::vector<char>	array;
	bool			given;

	Buffer()
		: given(false)
	{
	}
};

int writer(lua_State *, const char *data, size_t size, Buffer *buffer)
{
	for (size_t i = 0; i < size; ++i)
		buffer->array.push_back(data[i]);

	return 0;
}

const char *loader(lua_State *, Buffer *buffer, size_t *size)
{
	if (buffer->given)
	{
		*size = 0;
		return nullptr;
	}

	buffer->given = true;
	*size = buffer->array.size();

	return buffer->array.data();
}

/* ---------------------------------------------------------
 * Thread management
 * --------------------------------------------------------- */

const char *THREAD_TYPE = "Thread";

/* ---------------------------------------------------------
 * Functions and metamethods
 * --------------------------------------------------------- */

void threadCallback(lua_State *threadState, lua_State *L, int nparams)
{
	if (lua_pcall(threadState, nparams, 0, 0) != LUA_OK)
	{
		Logger::warn("plugin %s: %s",
		    Irccd::getInstance().findPlugin(L)->getName().c_str(),
		    lua_tostring(threadState, -1));
		lua_pop(threadState, 1);
	}

	Irccd::getInstance().unregisterThread(threadState);
}

int l_threadNew(lua_State *L)
{
	Buffer chunk;
	LuaState threadState;
	int np;

	luaL_checktype(L, 1, LUA_TFUNCTION);

	// Dump the function
	lua_pushvalue(L, 1);
	lua_dump(L, reinterpret_cast<lua_Writer>(writer), &chunk);
	lua_pop(L, 1);

	/*
	 * Load the same libs as a new Plugin.
	 */
	for (auto l : Plugin::luaLibs)
		Luae::require(threadState, l.first, l.second, true);
	for (auto l : Plugin::irccdLibs)
		Luae::preload(threadState, l.first, l.second);

	lua_load(threadState, reinterpret_cast<lua_Reader>(loader), &chunk, "thread", nullptr);

	np = 0;
	for (int i = 2; i <= lua_gettop(L); ++i)
	{
		LuaValue v = LuaValue::copy(L, i);
		LuaValue::push(threadState, v);
		++ np;
	}

	try
	{
		Plugin::Ptr self = Irccd::getInstance().findPlugin(L);
		Thread::Ptr thread = Thread::create();

		Irccd::getInstance().registerThread(threadState, self);
	
		std::thread threadFunc(threadCallback,
		    static_cast<lua_State *>(threadState),
		    static_cast<lua_State *>(L),
		    np);

		thread->setState(std::move(threadState));
		thread->setHandle(std::move(threadFunc));

		new (L, THREAD_TYPE) Thread::Ptr(thread);

	}
	catch (std::out_of_range)
	{
		Logger::fatal(1, "irccd: could not find plugin from Lua state %p", L);
	}

	return 1;
}

int l_threadWait(lua_State *L)
{
	Thread::Ptr *t = Luae::toType<Thread::Ptr *>(L, 1, THREAD_TYPE);

	(*t)->wait();

	return 0;
}

int l_threadGc(lua_State *L)
{
	Thread::Ptr *t = Luae::toType<Thread::Ptr *>(L, 1, THREAD_TYPE);

	(*t)->detach();
	(*t).~shared_ptr<Thread>();

	return 0;
}

int l_threadToString(lua_State *L)
{
	Thread *t = Luae::toType<Thread *>(L, 1, THREAD_TYPE);

	lua_pushfstring(L, "thread %p", t);

	return 1;
}

const luaL_Reg functions[] = {
	{ "new",		l_threadNew		},
	{ nullptr,		nullptr			}
};

const luaL_Reg threadMethods[] = {
	{ "wait",		l_threadWait		},
	{ nullptr,		nullptr			}
};

const luaL_Reg threadMeta[] = {
	{ "__gc",		l_threadGc		},
	{ "__tostring",		l_threadToString	},	
	{ nullptr,		nullptr			}
};

}

int luaopen_thread(lua_State *L)
{
	luaL_newlib(L, functions);

	// Create thread object
	luaL_newmetatable(L, THREAD_TYPE);
	luaL_setfuncs(L, threadMeta, 0);
	luaL_newlib(L, threadMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd
