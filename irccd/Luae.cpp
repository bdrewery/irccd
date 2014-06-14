/*
 * Luae.cpp -- Lua helpers and such
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

#include <sstream>

#include <Logger.h>

#include "Luae.h"
#include "Process.h"

namespace irccd {

/* --------------------------------------------------------
 * LuaeState
 * -------------------------------------------------------- */

const char *LuaeState::FieldRefs	= "__refs";

void LuaeState::initRegistry()
{
	if (LuaeTable::type(*this, LUA_REGISTRYINDEX, FieldRefs) == LUA_TNIL) {
		lua_createtable(*this, 0, 0);
		lua_createtable(*this, 0, 1);
		lua_pushstring(*this, "v");
		lua_setfield(*this, -2, "__mode");
		lua_setmetatable(*this, -2);
		lua_setfield(*this, LUA_REGISTRYINDEX, FieldRefs);
	}
}

LuaeState::LuaeState()
{
	m_state = Ptr(luaL_newstate());

	initRegistry();
}

LuaeState::LuaeState(lua_State *L)
{
	m_state = Ptr(L);

	initRegistry();
}

LuaeState::LuaeState(LuaeState &&state)
{
	m_state = std::move(state.m_state);

	initRegistry();
}

LuaeState &LuaeState::operator=(LuaeState &&state)
{
	m_state = std::move(state.m_state);

	initRegistry();

	return *this;
}

LuaeState::operator lua_State*()
{
	return m_state.get();
}

/* --------------------------------------------------------
 * LuaeValue
 * -------------------------------------------------------- */

LuaeValue LuaeValue::copy(lua_State *L, int index)
{
	LuaeValue v;

	v.type = lua_type(L, index);

	switch (v.type) {
	case LUA_TBOOLEAN:
		v.boolean = lua_toboolean(L, index);
		break;
	case LUA_TNUMBER:
		v.number = lua_tonumber(L, index);
		break;
	case LUA_TSTRING:
	{
		const char *tmp = lua_tolstring(L, index, &v.stringLength);

		v.string.reserve(v.stringLength);

		// Copy string by hand which avoid '\0'
		for (std::size_t i = 0; i < v.stringLength; ++i)
			v.string.push_back(tmp[i]);
	}
		break;
	case LUA_TTABLE:
	{
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

void LuaeValue::push(lua_State *L, const LuaeValue &value)
{
	switch (value.type) {
	case LUA_TBOOLEAN:
		lua_pushboolean(L, value.boolean);
		break;
	case LUA_TSTRING:
		lua_pushlstring(L,  value.string.data(), value.stringLength);
		break;
	case LUA_TNUMBER:
		lua_pushnumber(L, value.number);
		break;
	case LUA_TTABLE:
	{
		lua_createtable(L, 0, 0);

		for (auto p : value.table) {
			push(L, p.first);
			push(L, p.second);

			lua_settable(L, -3);
		}
		break;
	}
	default:
		lua_pushnil(L);
		break;
	}
}

LuaeValue::LuaeValue()
	: type(LUA_TNIL)
{
}

/* --------------------------------------------------------
 * LuaeTable
 * -------------------------------------------------------- */

void LuaeTable::create(lua_State *L, int nrec, int narr)
{
	LUAE_STACK_CHECKBEGIN(L);

	lua_createtable(L, nrec, narr);

	LUAE_STACK_CHECKEND(L, - 1);
}

int LuaeTable::type(lua_State *L, int idx, const std::string &name)
{
	int type;

	LUAE_STACK_CHECKBEGIN(L);

	lua_getfield(L, idx, name.c_str());
	type = lua_type(L, -1);
	lua_pop(L, 1);

	LUAE_STACK_CHECKEQUALS(L);

	return type;
}

void LuaeTable::read(lua_State *L, int idx, ReadFunction func)
{
	LUAE_STACK_CHECKBEGIN(L);

	lua_pushnil(L);

	if (idx < 0)
		--idx;

	while (lua_next(L, idx)) {
		func(L, lua_type(L, -2), lua_type(L, -1));
		lua_pop(L, 1);
	}

	LUAE_STACK_CHECKEQUALS(L);
}

int LuaeTable::ref(lua_State *L, int idx, int type, const std::string &name)
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

/* --------------------------------------------------------
 * LuaeClass
 * -------------------------------------------------------- */

const char *LuaeClass::FieldName	= "__name";
const char *LuaeClass::FieldParents	= "__parents";

void LuaeClass::create(lua_State *L, const Def &def)
{
	LUAE_STACK_CHECKBEGIN(L);
	luaL_newmetatable(L, def.name.c_str());

	// Store the name of the class
	lua_pushlstring(L, def.name.c_str(), def.name.length());
	lua_setfield(L, -2, FieldName);

	// Store the parents names
	int i = 0;

	lua_createtable(L, 0, 0);
	for (auto d(def.parent); d != nullptr; d = d->parent) {
		lua_pushlstring(L, d->name.c_str(), d->name.length());
		lua_rawseti(L, -2, ++i);
	}
	lua_setfield(L, -2, FieldParents);

	// Metamethods
	if (def.metamethods.size() > 0) {
		for (auto m : def.metamethods) {
			lua_pushcfunction(L, m.func);
			lua_setfield(L, -2, m.name);
		}
	}

	// Methods
	lua_createtable(L, 0, 0);
	for (auto m : def.methods) {
		lua_pushcfunction(L, m.func);
		lua_setfield(L, -2, m.name);
	}

	// Create the inheritance
	if (def.parent != nullptr) {
		luaL_newmetatable(L, def.parent->name.c_str());
		lua_setmetatable(L, -2);
	}
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1);
	LUAE_STACK_CHECKEQUALS(L);
}

void LuaeClass::testShared(lua_State *L, int index, const char *meta)
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
}

/* --------------------------------------------------------
 * LuaeEnum
 * -------------------------------------------------------- */

void LuaeEnum::create(lua_State *L, const Def &def)
{
	LUAE_STACK_CHECKBEGIN(L);

	lua_createtable(L, 0, 0);

	for (auto p : def) {
		lua_pushinteger(L, p.second);
		lua_setfield(L, -2, p.first);
	}

	LUAE_STACK_CHECKEND(L, - 1);
}

void LuaeEnum::create(lua_State *L, const Def &def, int index)
{
	LUAE_STACK_CHECKBEGIN(L);

	if (index < 0)
		-- index;

	for (auto p : def) {
		lua_pushinteger(L, p.second);
		lua_setfield(L, index, p.first);
	}

	LUAE_STACK_CHECKEQUALS(L);
}

void LuaeEnum::create(lua_State *L,
		      const Def &def,
		      int index,
		      const std::string &name)
{
	LUAE_STACK_CHECKBEGIN(L);

	create(L, def);

	if (index < 0)
		-- index;

	lua_setfield(L, index, name.c_str());

	LUAE_STACK_CHECKEQUALS(L);
}

void LuaeEnum::push(lua_State *L, const Def &def, int value)
{
	LUAE_STACK_CHECKBEGIN(L);
	lua_createtable(L, 0, 0);

	for (auto p : def) {
		if (value & p.second) {
			lua_pushinteger(L, p.second);
			lua_setfield(L, -2, p.first);
		}
	}

	LUAE_STACK_CHECKEND(L, - 1);
}

int LuaeEnum::get(lua_State *L, int index)
{
	int value = 0;

	LUAE_STACK_CHECKBEGIN(L);
	luaL_checktype(L, index, LUA_TTABLE);

	if (index < 0)
		-- index;

	lua_pushnil(L);
	while (lua_next(L, index)) {
		if (lua_type(L, -1) == LUA_TNUMBER)
			value |= lua_tointeger(L, -1);

		lua_pop(L, 1);
	}

	LUAE_STACK_CHECKEQUALS(L);

	return value;
}

/* --------------------------------------------------------
 * Luae
 * -------------------------------------------------------- */

void Luae::doexecute(lua_State *L, int status)
{
	if (status != LUA_OK) {
		auto error = lua_tostring(L, -1);
		lua_pop(L, 1);

		throw std::runtime_error(error);
	}
}

std::string Luae::format(lua_State *L, int index)
{
	std::string s;

	lua_getfield(L, LUA_REGISTRYINDEX, "__luae_format");
	if (lua_type(L, -1) == LUA_TNIL) {
		lua_pop(L, 1);
		luaopen_string(L);
		lua_getfield(L, -1, "format");
		lua_remove(L, -2);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, "__luae_format");
	}
	lua_insert(L, index);
	lua_call(L, lua_gettop(L) - index, 1);
	s = lua_tostring(L, -1);
	lua_pop(L, 1);

	return s;
}

void Luae::preload(lua_State *L, const std::string &name, lua_CFunction func)
{
	LUAE_STACK_CHECKBEGIN(L);

	lua_getglobal(L, "package");
	lua_getfield(L, -1, "preload");
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, name.c_str());
	lua_pop(L, 2);

	LUAE_STACK_CHECKEQUALS(L);
}

void Luae::require(lua_State *L, const std::string &name, lua_CFunction func, bool global)
{
	LUAE_STACK_CHECKBEGIN(L);

	luaL_requiref(L, name.c_str(), func, global);
	lua_pop(L, 1);

	LUAE_STACK_CHECKEQUALS(L);
}

void Luae::deprecate(lua_State *, const std::string &name, const std::string &repl)
{
	std::ostringstream oss;

	oss << "warning, usage of deprecated function `" << name << "'";

	if (repl.size() > 0)
		oss << ", please switch to `" << repl << "'";

	Logger::warn(oss.str());
}

} // !irccd

void *operator new(size_t size, lua_State *L)
{
	return lua_newuserdata(L, size);
}

void *operator new(size_t size, lua_State *L, const char *metaname)
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
