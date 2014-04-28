/*
 * LuaRule.cpp -- Lua bindings for class RuleManager
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

#include "Luae.h"
#include "RuleManager.h"

namespace irccd {

template <>
struct Luae::Convert<Rule> {
	static const bool hasPush = true;
	static const bool hasCheck = true;

	static void push(lua_State *, const Rule &)
	{
#if 0
		auto match	= rule.match();
		auto properties	= rule.properties();

		// Main table
		LuaeTable::create(L);
		LuaeTable::set(L, -1, "enabled", rule.isEnabled());

		// Match subtable
		LuaeTable::create(L);
		Luae::setfield(L, -2, "match");
#endif
	}

	static Rule check(lua_State *, int)
	{
#if 0
		RuleMatch match;
		RuleProperties properties;
		bool enabled = true;

		Luae::checktype(L, index, LUA_TTABLE);

		// General
		if (LuaeTable::type(L, index, "enabled") == LUA_TBOOLEAN)
			enabled = LuaeTable::get<bool>(L, index, "enabled");

		// Match subtable
		if (LuaeTable::type(L, index, "match") == LUA_TTABLE) {
		}

#endif
		return Rule(RuleMatch(), RuleProperties());
	}
};

namespace {

int l_add(lua_State *L)
{
	try {
		Luae::push(L, RuleManager::instance().add(Luae::check<Rule>(L, 1)));
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	return 1;
}

int l_get(lua_State *L)
{
	try {
		Luae::push(L, RuleManager::instance().get(Luae::check<int>(L, 1)));
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	return 1;
}

int l_remove(lua_State *L)
{
	try {
		RuleManager::instance().remove(Luae::check<int>(L, 1));
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	return 0;
}

int l_list(lua_State *)
{
	/* XXX */
	return 0;
}

int l_enable(lua_State *L)
{
	try {
		RuleManager::instance().enable(Luae::check<int>(L, 1));
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	Luae::push(L, true);

	return 0;
}

int l_disable(lua_State *L)
{
	try {
		RuleManager::instance().disable(Luae::check<int>(L, 1));
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_count(lua_State *L)
{
	Luae::push(L, static_cast<long>(RuleManager::instance().count()));

	return 1;
}

int l_clear(lua_State *)
{
	RuleManager::instance().clear();

	return 0;
}

const Luae::Reg functions {
	{ "add",			l_add		},
	{ "get",			l_get		},
	{ "remove",			l_remove	},
	{ "list",			l_list		},
	{ "enable",			l_enable	},
	{ "disable",			l_disable	},
	{ "count",			l_count		},
	{ "clear",			l_clear		}
};

}

int luaopen_rule(lua_State *L)
{
	Luae::newlib(L, functions);

	return 1;
}

} // !irccd
