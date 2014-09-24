/*
 * TestConverter.cpp -- string conversion for patterns
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

#include <cstdlib>
#include <sstream>

#include <gtest/gtest.h>

#include <common/Parser.h>
#include <common/Util.h>

namespace irccd {

TEST(Basic, useless)
{
	std::string str = "#";
	Util::Args args;

	std::string result = Util::convert(str, args);
	ASSERT_EQ("#", result);
}

TEST(Basic, simple)
{
	std::string str = "#s";
	Util::Args args;

	args.keywords['s'] = "test";
	std::string result = Util::convert(str, args);

	ASSERT_EQ("test", result);
}

TEST(Basic, two)
{
	std::string str = "#s #c";
	Util::Args args;

	args.keywords['s'] = "s";
	args.keywords['c'] = "c";

	std::string result = Util::convert(str, args);

	ASSERT_EQ("s c", result);
}

TEST(Basic, oneAbsent)
{
	std::string str = "#s #x #c";
	Util::Args args;

	args.keywords['s'] = "s";
	args.keywords['c'] = "c";

	std::string result = Util::convert(str, args);

	ASSERT_EQ("s #x c", result);
}

TEST(Basic, replaceByPattern)
{
	std::string str = "#a #b";
	Util::Args args;

	args.keywords['a'] = "#c";
	args.keywords['c'] = "FAIL";
	args.keywords['b'] = "b";

	std::string result = Util::convert(str, args);

	ASSERT_EQ("#c b", result);
}

TEST(Basic, dateFlags)
{
	std::string str = "%h";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertDate);
	ASSERT_NE("%h", result);

	result = Util::convert(str, args);
	ASSERT_EQ("%h", result);
}

TEST(Basic, homeFlags)
{
	auto home = std::getenv("HOME");

	if (home == nullptr)
		return;

	std::string str = "~";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertHome);
	ASSERT_EQ(home, result);

	result = Util::convert(str, args);
	ASSERT_EQ(str, result);
}

TEST(Basic, envFlags)
{
	auto home = std::getenv("HOME");

	if (home == nullptr)
		return;

	std::string str = "${HOME}";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertEnv);
	ASSERT_EQ(home, result);

	result = Util::convert(str, args);
	ASSERT_EQ(str, result);
}

} // !irccd

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
