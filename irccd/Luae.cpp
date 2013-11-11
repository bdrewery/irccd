/*
 * Lua.cpp -- Lua helpers and such
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

#include "Luae.h"

namespace irccd {

LuaState::LuaState()
{
	m_state = Ptr(luaL_newstate());
}

LuaState::LuaState(lua_State *L)
{
	m_state = Ptr(L);
}

LuaState::LuaState(LuaState &&state)
{
	m_state = std::move(state.m_state);
}

LuaState &LuaState::operator=(LuaState &&state)
{
	m_state = std::move(state.m_state);

	return *this;
}

LuaState::operator lua_State*()
{
	return m_state.get();
}

LuaValue LuaValue::copy(lua_State *L, int index)
{
	LuaValue v;

	v.type = lua_type(L, index);

	switch (v.type) {
	case LUA_TBOOLEAN:
		v.boolean = lua_toboolean(L, index);
		break;
	case LUA_TNUMBER:
		v.number = lua_tonumber(L, index);
		break;
	case LUA_TSTRING:
		v.str = lua_tostring(L, index);
		break;
	case LUA_TTABLE:
	{
		LuaValue k;

		if (index < 0)
			-- index;

		lua_pushnil(L);
		while (lua_next(L, index)) {
			v.table.push_back(std::make_pair(copy(L, -2), copy(L, -1)));
			lua_pop(L, 1);
		}

		break;
	}
	default:
		v.type = LUA_TNIL;
		break;
	}

	return v;
}

void LuaValue::push(lua_State *L, const LuaValue &value)
{
	switch (value.type) {
	case LUA_TBOOLEAN:
		lua_pushboolean(L, value.boolean);
		break;
	case LUA_TSTRING:
		lua_pushlstring(L,  value.str.c_str(), value.str.size());
		break;
	case LUA_TNUMBER:
		lua_pushnumber(L, value.number);
		break;
	case LUA_TTABLE:
	{
		lua_createtable(L, 0, 0);

		for (auto p : value.table) {
			LuaValue::push(L, p.first);
			LuaValue::push(L, p.second);

			lua_settable(L, -3);
		}
		break;
	}
	default:
		lua_pushnil(L);
		break;
	}
}

LuaValue::LuaValue()
	: type(LUA_TNIL)
{
}

template <>
bool Luae::getField(lua_State *L, int idx, const std::string &name)
{
	bool value = false;

	lua_getfield(L, idx, name.c_str());
	if (lua_type(L, -1) == LUA_TBOOLEAN)
		value = lua_toboolean(L, -1) == 1;
	lua_pop(L, 1);

	return value;
}

template <>
double Luae::getField(lua_State *L, int idx, const std::string &name)
{
	double value = 0;

	lua_getfield(L, idx, name.c_str());
	if (lua_type(L, -1) == LUA_TNUMBER)
		value = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return value;
}

template <>
int Luae::getField(lua_State *L, int idx, const std::string &name)
{
	int value = 0;

	lua_getfield(L, idx, name.c_str());
	if (lua_type(L, -1) == LUA_TNUMBER)
		value = lua_tointeger(L, -1);
	lua_pop(L, 1);

	return value;
}

template <>
std::string Luae::getField(lua_State *L, int idx, const std::string &name)
{
	std::string value;

	lua_getfield(L, idx, name.c_str());
	if (lua_type(L, -1) == LUA_TSTRING)
		value = lua_tostring(L, -1);
	lua_pop(L, 1);

	return value;
}

int Luae::typeField(lua_State *L, int idx, const std::string &name)
{
	int type;

	LUA_STACK_CHECKBEGIN(L);

	lua_getfield(L, idx, name.c_str());
	type = lua_type(L, -1);
	lua_pop(L, 1);

	LUA_STACK_CHECKEQUALS(L);

	return type;
}

void Luae::preload(lua_State *L, const std::string &name, lua_CFunction func)
{
	LUA_STACK_CHECKBEGIN(L);

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, name.c_str());
	lua_pop(L, 2);

	LUA_STACK_CHECKEQUALS(L);
}

void Luae::readTable(lua_State *L, int idx, ReadFunction func)
{
	lua_pushnil(L);

	if (idx < 0)
		--idx;

	while (lua_next(L, idx)) {
		func(L, lua_type(L, -2), lua_type(L, -1));
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
}

int Luae::referenceField(lua_State *L, int idx, int type, const std::string &name)
{
	int ref = LUA_REFNIL;

	lua_getfield(L, idx, name.c_str());

	if (lua_type(L, -1) == type) {
		lua_pushvalue(L, -1);
		ref = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	lua_pop(L, 1);

	return ref;
}

void Luae::require(lua_State *L, const std::string &name, lua_CFunction func, bool global)
{
	LUA_STACK_CHECKBEGIN(L);

	luaL_requiref(L, name.c_str(), func, global);
	lua_pop(L, 1);

	LUA_STACK_CHECKEQUALS(L);
}

} // !irccd

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

void operator delete(void *, lua_State *)
{
}

void operator delete(void *, lua_State *L, const char *)
{
	lua_pushnil(L);
	lua_setmetatable(L, -2);
}
