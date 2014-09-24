/*
 * TestStrip.cpp -- test Util::strip
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

#include <common/Util.h>

namespace irccd {

TEST(Basic, left)
{
	std::string value = "   123";
	std::string result = Util::strip(value);

	ASSERT_EQ("123", result);
}

TEST(Basic, right)
{
	std::string value = "123   ";
	std::string result = Util::strip(value);

	ASSERT_EQ("123", result);
}

TEST(Basic, both)
{
	std::string value = "   123   ";
	std::string result = Util::strip(value);

	ASSERT_EQ("123", result);
}

TEST(Basic, none)
{
	std::string value = "without";
	std::string result = Util::strip(value);

	ASSERT_EQ("without", result);
}

TEST(Basic, betweenEmpty)
{
	std::string value = "one list";
	std::string result = Util::strip(value);

	ASSERT_EQ("one list", result);
}

TEST(Basic, betweenLeft)
{
	std::string value = "  space at left";
	std::string result = Util::strip(value);

	ASSERT_EQ("space at left", result);
}

TEST(Basic, betweenRight)
{
	std::string value = "space at right  ";
	std::string result = Util::strip(value);

	ASSERT_EQ("space at right", result);
}

TEST(Basic, betweenBoth)
{
	std::string value = "  space at both  ";
	std::string result = Util::strip(value);

	ASSERT_EQ("space at both", result);
}

TEST(Basic, empty)
{
	std::string value = "    ";
	std::string result = Util::strip(value);

	ASSERT_EQ("", result);
}

} // !irccd

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
