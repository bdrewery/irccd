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
 * local r = {
 *	enabled = true | false,
 *
 *	match = {
 *		servers	= {
 *			["localhost"] = true,
 *			["malikania"] = true
 *		},
 *
 *		channels = {
 *			["#staff"] = true,
 *		},
 *
 *		plugins	= {
 *			["a"] = true
 *		},
 *	},
 *
 *	set = {
 *		recode = { "ISO-8859-15", "UTF-8" }
 *		plugins = {
 *			["x"] = true
 *		},
 *
 *		events = {
 *			["onCommand"] = false
 *		}
 *	}
 * }
 * </pre>
 */
template <>
struct Luae::Convert<Rule> {
private:
	static void setSequence(lua_State *L, const RuleMap &map, const std::string &field)
	{
		LuaeTable::create(L);

		for (const auto &r : map)
			LuaeTable::set(L, -1, r.first, r.second);

		Luae::setfield(L, -2, field);
	}

	template <typename T>
	using AddFunc = void (T::*)(const std::string &, bool);

	template <typename T>
	static void getSequence(lua_State *L, AddFunc<T> add, T &t, const std::string &field)
	{
		Luae::getfield(L, -1, field);
		if (Luae::type(L, -1) == LUA_TTABLE) {
			LuaeTable::read(L, -1, [&] (lua_State *L, int tkey, int tvalue) {
				if (tkey == LUA_TSTRING && tvalue == LUA_TBOOLEAN)
					(t.*add)(Luae::get<std::string>(L, -2), Luae::get<bool>(L, -1));
			});
		}
		Luae::pop(L);
	}

public:
	/**
	 * Push supported.
	 */
	static const bool hasPush = true;

	/**
	 * Check supported.
	 */
	static const bool hasCheck = true;

	/**
	 * Push the rule.
	 *
	 * @param L the Lua state
	 * @param rule the rule
	 */
	static void push(lua_State *L, const Rule &rule)
	{
		const auto &match	= rule.match();
		const auto &properties	= rule.properties();
		const auto &encoding	= properties.encoding();

		// Main table
		LuaeTable::create(L);
		LuaeTable::set(L, -1, "enabled", rule.isEnabled());

		// Match subtable
		LuaeTable::create(L);
		setSequence(L, match.servers(), "servers");
		setSequence(L, match.channels(), "channels");
		setSequence(L, match.plugins(), "plugins");
		Luae::setfield(L, -2, "match");

		// Set subtable
		LuaeTable::create(L);
		setSequence(L, properties.plugins(), "plugins");
		setSequence(L, properties.events(), "events");

		// Encoding
		if (encoding.size() == 0)
			LuaeTable::set(L, -1, "encoding", "default");
		else
			LuaeTable::set(L, -1, "encoding", encoding);

		Luae::setfield(L, -2, "set");
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
		RuleMatch match;
		RuleProperties properties;
		bool enabled = true;

		Luae::checktype(L, index, LUA_TTABLE);

		// General
		if (LuaeTable::type(L, index, "enabled") == LUA_TBOOLEAN)
			enabled = LuaeTable::get<bool>(L, index, "enabled");

		// Match subtable
		if (LuaeTable::type(L, index, "match") == LUA_TTABLE) {
			Luae::getfield(L, index, "match");
			getSequence(L, &RuleMatch::addServer, match, "servers");
			getSequence(L, &RuleMatch::addChannel, match, "channels");
			getSequence(L, &RuleMatch::addPlugin, match, "plugins");
			Luae::pop(L);
		}

		// Set subtable
		if (LuaeTable::type(L, index, "set") == LUA_TTABLE) {
			Luae::getfield(L, index, "set");
			getSequence(L, &RuleProperties::setPlugin, properties, "plugins");
			getSequence(L, &RuleProperties::setEvent, properties, "events");

			if (LuaeTable::type(L, -1, "encoding") == LUA_TSTRING) {
				if (match.plugins().size() > 0) {
					Luae::error(L, "encoding parameter should be set only with servers and channels");
					// NOTREACHED
				}

				properties.setEncoding(LuaeTable::get<std::string>(L, -1, "encoding"));
			}

			Luae::pop(L);
		}

		return Rule(match, properties, enabled);
	}
};

namespace {

int l_add(lua_State *L)
{
	try {
		auto rule = Luae::check<Rule>(L, 1);
		auto index = luaL_optint(L, 2, -1);

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
	Luae::pushfunction(L, [] (lua_State *L) -> int{
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
		} catch (const std::out_of_range &) {
			return 0;
		}

		return 2;
	}, 1);

	return 1;
}

int l_enable(lua_State *L)
{
	try {
		RuleManager::instance().enable(Luae::check<int>(L, 1) - 1);
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
		RuleManager::instance().disable(Luae::check<int>(L, 1) - 1);
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
