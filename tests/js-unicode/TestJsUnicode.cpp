/*
 * TestJsUnicode.cpp -- test irccd unicode JS API
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

/*
 * /!\ Be sure that this file is kept saved in UTF-8 /!\
 */

#include <gtest/gtest.h>

#include <IrccdConfig.h>
#include <LibtestUtil.h>
#include <Unicode.h>

using namespace irccd;

class TestJsUnicode : public LibtestUtil {
public:
	TestJsUnicode()
		: LibtestUtil("unicode", "irccd.unicode")
	{
	}
};

TEST_F(TestJsUnicode, symbols)
{
	checkSymbol("unicode.Unicode.forEach", "function");
	checkSymbol("unicode.Unicode.isDigit", "function");
	checkSymbol("unicode.Unicode.isLetter", "function");
	checkSymbol("unicode.Unicode.isLower", "function");
	checkSymbol("unicode.Unicode.isSpace", "function");
	checkSymbol("unicode.Unicode.isTitle", "function");
	checkSymbol("unicode.Unicode.isUpper", "function");
	checkSymbol("unicode.Unicode.length", "function");
	checkSymbol("unicode.Unicode.toUtf32", "function");
	checkSymbol("unicode.Unicode.toLower", "function");
	checkSymbol("unicode.Unicode.toUtf8", "function");
	checkSymbol("unicode.Unicode.toUpper", "function");
}

TEST_F(TestJsUnicode, forEach)
{
	execute(
		"str = \"aé€\";"
		"result = [ ];"
		"unicode.Unicode.forEach(str, function (code) {"
		"	result.push(code);"
		"});"
	);

	duk_get_global_string(m_ctx, "result");

	ASSERT_EQ(DUK_TYPE_OBJECT, duk_get_type(m_ctx, -1));

	duk_enum(m_ctx, -1, DUK_ENUM_ARRAY_INDICES_ONLY);

	int current = 0;
	while (duk_next(m_ctx, -1, 1)) {
		if (current == 0) {
			ASSERT_EQ(U'a', duk_to_uint(m_ctx, -1));
		} else if (current == 1) {
			ASSERT_EQ(U'é', duk_to_uint(m_ctx, -1));
		} else if (current == 2) {
			ASSERT_EQ(U'€', duk_to_uint(m_ctx, -1));
		}

		current ++;
		duk_pop_2(m_ctx);
	}

	ASSERT_EQ(3, current);
}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
