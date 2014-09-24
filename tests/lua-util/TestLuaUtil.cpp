/*
 * TestLuaUtil.cpp -- test irccd.util API
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

#include <gtest/gtest.h>
#include <lua.hpp>

#include <lua/LuaUtil.h>

namespace irccd {

TEST(Basic, split)
{
	try {
		LuaeState L;

		Luae::openlibs(L);
		Luae::require(L, "irccd.util", luaopen_util, false);
		Luae::dofile(L, "scripts/test-util.lua");
		Luae::getglobal(L, "split");
		Luae::pcall(L, 0, 0);
	} catch (const std::runtime_error &error) {
		FAIL() << error.what();
	}
}

TEST(Basic, strip)
{
	try {
		LuaeState L;

		Luae::openlibs(L);
		Luae::require(L, "irccd.util", luaopen_util, false);
		Luae::dofile(L, "scripts/test-util.lua");
		Luae::getglobal(L, "strip");
		Luae::pcall(L, 0, 0);
	} catch (const std::runtime_error &error) {
		FAIL() << error.what();
	}
}

TEST(Basic, convert)
{
	try {
		LuaeState L;

		Luae::openlibs(L);
		Luae::require(L, "irccd.util", luaopen_util, false);
		Luae::dofile(L, "scripts/test-util.lua");
		Luae::getglobal(L, "convert");
		Luae::pcall(L, 0, 0);
	} catch (const std::runtime_error &error) {
		FAIL() << error.what();
	}
}

} // !irccd

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
