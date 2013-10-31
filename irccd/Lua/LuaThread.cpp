/*
 * LuaThread.cpp -- Lua bindings for threads
 *
 * Copyright 2013 David Demelier <markand@malikania.fr>
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
#include <thread>

#include "Irccd.h"
#include "Logger.h"
#include "Luae.h"
#include "LuaThread.h"
#include "Plugin.h"

namespace irccd
{

namespace {

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

int l_threadNew(lua_State *L)
{
	Buffer chunk;
	lua_State *newL;
	int np;

	luaL_checktype(L, 1, LUA_TFUNCTION);

	// Dump the function
	lua_pushvalue(L, 1);
	lua_dump(L, reinterpret_cast<lua_Writer>(writer), &chunk);
	lua_pop(L, 1);

	// Create a new state and load the function in it
	newL = luaL_newstate();

	/*
	 * Load the same libs as a new Plugin.
	 */
	for (const Plugin::Library &l : Plugin::luaLibs)
		Luae::require(newL, l.m_name, l.m_func, true);
	for (const Plugin::Library &l : Plugin::irccdLibs)
		Luae::preload(newL, l.m_name, l.m_func);

	lua_load(newL, reinterpret_cast<lua_Reader>(loader), &chunk, "thread", nullptr);

	np = 0;
	for (int i = 2; i <= lua_gettop(L); ++i) {
		LuaValue v = LuaValue::copy(L, i);
		LuaValue::push(newL, v);
		++ np;
	}

	// Register it so logger can use the name
	auto myself = Irccd::getInstance()->findPlugin(L);

	Irccd::getInstance()->registerPluginThread(myself, newL);

	std::thread th = std::thread([&] () {
		if (lua_pcall(newL, np, 0, 0) != LUA_OK) {
			Logger::warn("plugin %s: %s",
			    Irccd::getInstance()->findPlugin(L)->getName().c_str(),
			    lua_tostring(newL, -1));
			lua_pop(L, 1);
		}
	});

	th.detach();

	return 0;
}

const luaL_Reg functions[] = {
	{ "new",	l_threadNew	},
	{ nullptr,	nullptr		}
};

}

int luaopen_thread(lua_State *L)
{
	luaL_newlib(L, functions);

	return 1;
}

} // !irccd
