/*
 * LuaState.cpp -- Lua C++ wrapper
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

#include "LuaState.h"

using namespace irccd;
using namespace std;

LuaState::LuaState()
{
}

LuaState::LuaState(LuaState &&src)
{
	m_luaState = std::move(src.m_luaState);
	m_error = std::move(src.m_error);
}

LuaState::~LuaState()
{
}

void LuaState::openState()
{
	m_luaState = LuaPointer(luaL_newstate());
}

lua_State * LuaState::getState()
{
	return m_luaState.get();
}

const string & LuaState::getError() const
{
	return m_error;
}

int LuaState::getglobal(const string& name)
{
	lua_getglobal(getState(), name.c_str());

	return lua_type(getState(), -1);
}

bool LuaState::dofile(const string& path)
{
	if (luaL_dofile(getState(), path.c_str()) != LUA_OK) {
		m_error = lua_tostring(getState(), -1);
		return false;
	}

	return true;
}

void LuaState::createtable(int narr, int nrec)
{
	lua_createtable(getState(), narr, nrec);
}

void LuaState::push()
{
	lua_pushnil(getState());
}

void LuaState::push(int i)
{
	lua_pushinteger(getState(), i);
}

void LuaState::push(const string& str)
{
	lua_pushlstring(getState(), str.c_str(), str.length());
}

void LuaState::push(double d)
{
	lua_pushnumber(getState(), d);
}

void LuaState::require(const string& name, lua_CFunction func, bool global)
{
	luaL_requiref(getState(), name.c_str(), func, global);
}

void LuaState::preload(const string& name, lua_CFunction func)
{
	lua_getglobal(getState(), "package");
	lua_getfield(getState(), -1, "preload");
	lua_pushcfunction(getState(), func);
	lua_setfield(getState(), -2, name.c_str());
	lua_pop(getState(), 2);
}

bool LuaState::pcall(int np, int nr, int errorh)
{
	bool success;

	success = lua_pcall(getState(), np, nr, errorh) == LUA_OK;
	if (!success && errorh == 0) {
		m_error = lua_tostring(getState(), -1);
		lua_pop(getState(), 1);
	}

	return success;
}

int LuaState::ref(int t)
{
	return luaL_ref(getState(), t);
}

void LuaState::unref(int t, int ref)
{
	return luaL_unref(getState(), t, ref);
}

void LuaState::rawget(int t, int n)
{
	lua_rawgeti(getState(), t, n);
}

void LuaState::rawset(int t, int n)
{
	lua_rawseti(getState(), t, n);
}

LuaState & LuaState::operator=(LuaState &&src)
{
	m_luaState = std::move(src.m_luaState);
	m_error = std::move(src.m_error);

	return *this;
}

void * operator new(size_t size, lua_State *L)
{
	return lua_newuserdata(L, size);
}

void * operator new(size_t size, lua_State *L, const char *metaname)
{
	void *object;

	object = lua_newuserdata(L, size);
	luaL_setmetatable(L, metaname);

	return object;
}