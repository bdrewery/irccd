/*
 * LuaUtf8.cpp -- Lua bindings for class Utf8
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
#include "LuaUtf8.h"
#include "Utf8.h"

namespace irccd {

namespace {

int convert(lua_State *L, bool toupper)
{
	try {
		if (Luae::type(L, 1) == LUA_TTABLE) {
			if (toupper)
				Luae::push(L, Utf8::toupper(Luae::check<std::u32string>(L, 1)));
			else
				Luae::push(L, Utf8::tolower(Luae::check<std::u32string>(L, 1)));
		} else {
			if (toupper)
				Luae::push(L, Utf8::toupper(Luae::check<std::string>(L, 1)));
			else
				Luae::push(L, Utf8::tolower(Luae::check<std::string>(L, 1)));
		}
	} catch (const std::invalid_argument &argument) {
		Luae::push(L, nullptr);
		Luae::push(L, argument.what());

		return 2;
	}

	return 1;
}

int iterator(lua_State *L)
{
	auto i = Luae::get<int>(L, Luae::upvalueindex(2));
	auto length = Luae::rawlen(L, Luae::upvalueindex(1));

	if (i - 1 == static_cast<int>(length))
		return 0;

	Luae::rawget(L, Luae::upvalueindex(1), i);
	auto value = Luae::get<int>(L, -1);
	Luae::pop(L, 1);

	Luae::push(L, ++i);
	Luae::replace(L, Luae::upvalueindex(2));
	Luae::push(L, value);

	return 1;
}

int l_isdigit(lua_State *L)
{
	Luae::push(L, Utf8::isdigit(Luae::check<int>(L, 1)));

	return 1;
}

int l_isletter(lua_State *L)
{
	Luae::push(L, Utf8::isletter(Luae::check<int>(L, 1)));

	return 1;
}

int l_islower(lua_State *L)
{
	Luae::push(L, Utf8::islower(Luae::check<int>(L, 1)));

	return 1;
}

int l_isspace(lua_State *L)
{
	Luae::push(L, Utf8::isspace(Luae::check<int>(L, 1)));

	return 1;
}

int l_istitle(lua_State *L)
{
	Luae::push(L, Utf8::istitle(Luae::check<int>(L, 1)));

	return 1;
}

int l_isupper(lua_State *L)
{
	Luae::push(L, Utf8::isupper(Luae::check<int>(L, 1)));

	return 1;
}

int l_length(lua_State *L)
{
	auto str = Luae::check<std::string>(L, 1);

	try {
		Luae::push(L, static_cast<int>(Utf8::length(str)));
	} catch (const std::invalid_argument &error) {
		Luae::push(L, nullptr);
		Luae::push(L, error.what());

		return 2;
	}

	return 1;
}

int l_list(lua_State *L)
{
	Luae::push(L, Utf8::toucs(Luae::check<std::string>(L, 1)));
	Luae::push(L, 1);
	Luae::pushfunction(L, iterator, 2);

	return 1;
}

int l_toarray(lua_State *L)
{
	auto array = Luae::check<std::string>(L, 1);

	try {
		Luae::push(L, Utf8::toucs(array));
	} catch (const std::invalid_argument &error) {
		Luae::push(L, nullptr);
		Luae::push(L, error.what());

		return 2;
	}

	return 1;
}

int l_tolower(lua_State *L)
{
	return convert(L, false);
}

int l_tostring(lua_State *L)
{
	auto array = Luae::check<std::u32string>(L, 1);

	try {
		Luae::push(L, Utf8::toutf8(array));
	} catch (const std::invalid_argument &error) {
		Luae::push(L, nullptr);
		Luae::push(L, error.what());

		return 2;
	}

	return 1;
}

int l_toupper(lua_State *L)
{
	return convert(L, true);
}

const Luae::Reg functions {
	{ "isdigit",		l_isdigit	},
	{ "isletter",		l_isletter	},
	{ "islower",		l_islower	},
	{ "isspace",		l_isspace	},
	{ "istitle",		l_istitle	},
	{ "isupper",		l_isupper	},
	{ "length",		l_length	},
	{ "list",		l_list		},
	{ "toarray",		l_toarray	},
	{ "tolower",		l_tolower	},
	{ "tostring",		l_tostring	},
	{ "toupper",		l_toupper	},
};

}

int luaopen_utf8(lua_State *L)
{
	Luae::newlib(L, functions);

	return 1;
}

} // !irccd
