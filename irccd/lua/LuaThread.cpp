/*
 * LuaThread.cpp -- Lua bindings for threads
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

#include <string>

#include <common/Logger.h>

#include <irccd/Irccd.h>
#include <irccd/Luae.h>
#include <irccd/Plugin.h>
#include <irccd/Thread.h>

namespace irccd {

namespace {

/* ---------------------------------------------------------
 * Buffer management for loading threads
 * --------------------------------------------------------- */

struct Buffer {
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
	if (buffer->given) {
		*size = 0;
		return nullptr;
	}

	buffer->given = true;
	*size = buffer->array.size();

	return buffer->array.data();
}

void loadfunction(std::shared_ptr<Thread> &thread, lua_State *owner)
{
	Buffer chunk;

	lua_pushvalue(owner, 1);
	lua_dump(owner, reinterpret_cast<lua_Writer>(writer), &chunk, true);
	lua_pop(owner, 1);
	lua_load(*thread, reinterpret_cast<lua_Reader>(loader), &chunk, "thread", nullptr);
}

void loadfile(std::shared_ptr<Thread> &thread, const char *path)
{
	if (luaL_loadfile(*thread, path) != LUA_OK) {
		auto error = lua_tostring(*thread, -1);
		lua_pop(*thread, 1);

		throw std::runtime_error(error);
	}
}

/* ---------------------------------------------------------
 * Thread management
 * --------------------------------------------------------- */

const char *ThreadType = "Thread";

/* ---------------------------------------------------------
 * Functions and metamethods
 * --------------------------------------------------------- */

int l_threadNew(lua_State *L)
{
	auto thread = std::make_shared<Thread>();
	int np;

	for (const auto &l : Process::luaLibs)
		Luae::require(*thread, l.first, l.second, true);
	for (const auto &l : Process::irccdLibs)
		Luae::preload(*thread, l.first, l.second);

	try {
		if (lua_type(L, 1) == LUA_TFUNCTION)
			loadfunction(thread, L);
		else if (lua_type(L, 1) == LUA_TSTRING)
			loadfile(thread, lua_tostring(L, 1));
		else
			return luaL_error(L, "expected a function or a file path");
	} catch (std::runtime_error err) {
		lua_pushnil(L);
		lua_pushstring(L, err.what());

		return 2;
	}

	np = 0;
	for (int i = 2; i <= lua_gettop(L); ++i) {
		LuaeValue v = LuaeValue::copy(L, i);
		LuaeValue::push(*thread, v);
		++ np;
	}

	try {
		auto info = Process::info(L);
		auto process = thread->process();

		// Set home and name like the plugin
		Process::initialize(process, info);

		// Create the object to push as return value
		auto *ptr = new (L, ThreadType) std::shared_ptr<Thread>(thread);
		Thread::start(*ptr, np);
	} catch (std::out_of_range) {
		Logger::fatal(1, "irccd: could not find plugin from Lua state %p", L);
	}

	return 1;
}

int l_threadJoin(lua_State *L)
{
	auto *t = Luae::toType<std::shared_ptr<Thread> *>(L, 1, ThreadType);

	try {
		(*t)->join();
	} catch (std::system_error error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_threadDetach(lua_State *L)
{
	auto *t = Luae::toType<std::shared_ptr<Thread> *>(L, 1, ThreadType);

	try {
		(*t)->detach();
	} catch (std::system_error error) {
		lua_pushnil(L);
		lua_pushstring(L, error.what());

		return 2;
	}

	lua_pushboolean(L, true);

	return 1;
}

int l_threadGc(lua_State *L)
{
	auto *t = Luae::toType<std::shared_ptr<Thread> *>(L, 1, ThreadType);

	/*
	 * At this step, the thread is marked for deletion but may have not
	 * been join() or detach(). So we detach it to avoid blocking the
	 * application and throwing an error.
	 */
	if (!(*t)->hasJoined()) {
		Logger::debug("thread: detaching because not joined");

		try {
			(*t)->detach();
		} catch (std::system_error) { }
	}

	(*t).~shared_ptr<Thread>();

	return 0;
}

int l_threadToString(lua_State *L)
{
	Thread *t = Luae::toType<Thread *>(L, 1, ThreadType);

	Luae::pushfstring(L, "thread %p", t);

	return 1;
}

const luaL_Reg functions[] = {
	{ "new",		l_threadNew		},
	{ nullptr,		nullptr			}
};

const luaL_Reg threadMethods[] = {
	{ "join",		l_threadJoin		},
	{ "detach",		l_threadDetach		},
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
	luaL_newmetatable(L, ThreadType);
	luaL_setfuncs(L, threadMeta, 0);
	luaL_newlib(L, threadMethods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	return 1;
}

} // !irccd
