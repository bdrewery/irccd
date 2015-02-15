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

#include <irccd/Luae.h>
#include <irccd/RuleManager.h>

namespace irccd {

/**
 * @brief Support for Rule
 *
 * Pushes a table with the following fields:
 *
 * <pre>
 * local rule = {
 *     action,
 *     servers = { } or ""
 *     channels = { } or "",
 *     nicknames = { } or "",
 *     plugins = { } or "",
 *     events = { } or ""
 * }
 * </pre>
 */
template <>
struct Luae::Convert<Rule> {
public:
	/**
	 * Push supported.
	 */
	static const bool hasPush = true;

	/**
	 * Check supported.
	 */
	static const bool hasCheck = true;

	static void pushSequence(lua_State *L, const RuleMap &map, const std::string &name)
	{
		lua_createtable(L, 0, 0);

		int i = 1;
		for (const auto &v : map) {
			lua_pushinteger(L, i++);
			lua_pushstring(L, v.c_str());
			lua_settable(L, -3);
		}

		lua_setfield(L, -2, name.c_str());
	}

	/**
	 * Push the rule.
	 *
	 * @param L the Lua state
	 * @param rule the rule
	 */
	static void push(lua_State *L, const Rule &rule)
	{
		lua_createtable(L, 0, 0);
		lua_pushinteger(L, static_cast<int>(rule.action()));
		lua_setfield(L, -2, "action");

		pushSequence(L, rule.servers(), "servers");
		pushSequence(L, rule.channels(), "channels");
		pushSequence(L, rule.nicknames(), "nicknames");
		pushSequence(L, rule.plugins(), "plugins");
		pushSequence(L, rule.events(), "events");
	}

	static RuleMap getSequence(lua_State *L, int index, const std::string &name)
	{
		RuleMap result;

		LUAE_STACK_CHECKBEGIN(L);
		lua_getfield(L, index, name.c_str());

		if (lua_type(L, -1) == LUA_TSTRING) {
			// only one value
			result.insert(lua_tostring(L, -1));
		} else if (lua_type(L, -1) == LUA_TTABLE) {
			// multiple values
			lua_pushnil(L);

			while (lua_next(L, -2) != 0) {
				if (lua_type(L, -1) == LUA_TSTRING)
					result.insert(lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}

		lua_pop(L, 1);
		LUAE_STACK_CHECKEQUALS(L);

		return result;
	}

	/**
	 * Check the rule.
	 *
	 * @param L the Lua state
	 * @param index the index
	 * @return the rule
	 * @throw std::invalid_argument if bad arguments
	 */
	static Rule check(lua_State *L, int index)
	{
		luaL_checktype(L, index, LUA_TTABLE);

		lua_getfield(L, index, "action");
		RuleAction action = static_cast<RuleAction>(luaL_optinteger(L, -1, static_cast<int>(RuleAction::Accept)));
		lua_pop(L, 1);

		return Rule{
			getSequence(L, index, "servers"),
			getSequence(L, index, "channels"),
			getSequence(L, index, "nicknames"),
			getSequence(L, index, "plugins"),
			getSequence(L, index, "events"),
			action,
		};
	}
};

namespace {

int l_add(lua_State *L)
{
	try {
		auto rule = Luae::check<Rule>(L, 1);
		auto index = luaL_optinteger(L, 2, -1);

		Luae::push(L, RuleManager::instance().add(rule, index));
	} catch (const std::exception &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_get(lua_State *L)
{
	try {
		Luae::push(L, RuleManager::instance().get(Luae::check<int>(L, 1) - 1));
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
		RuleManager::instance().remove(Luae::check<int>(L, 1) - 1);
	} catch (const std::out_of_range &ex) {
		Luae::push(L, nullptr);
		Luae::push(L, ex.what());

		return 2;
	}

	Luae::push(L, true);

	return 1;
}

int l_list(lua_State *L)
{
	Luae::push(L, 0);
	Luae::pushfunction(L, [] (lua_State *L) -> int {
		auto i = Luae::get<int>(L, Luae::upvalueindex(1));
		auto l = RuleManager::instance().count();

		if (static_cast<unsigned>(i) >= l)
			return 0;

		/*
		 * Here, an other thread may have deleted a rule already.
		 */
		try {
			Luae::push(L, RuleManager::instance().get(i));
			Luae::push(L, ++i);

			// Replace the counter
			Luae::push(L, i);
			Luae::replace(L, Luae::upvalueindex(1));
		} catch (const std::exception &) {
			return 0;
		}

		return 2;
	}, 1);

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
