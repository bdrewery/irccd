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

#include <cppunit/TextTestRunner.h>

#include <common/Parser.h>
#include <common/Util.h>

#include "TestConverter.h"

namespace irccd
{

std::string TestConverter::message(const std::string &expected,
				   const std::string &result)
{
	std::ostringstream oss;

	oss << "'" << expected << "' expected, ";
	oss << "got '" << result << "'";

	return oss.str();
}

void TestConverter::useless()
{
	std::string str = "#";
	Util::Args args;

	std::string result = Util::convert(str, args);
	CPPUNIT_ASSERT_MESSAGE(message("#", result), result == "#");
}

void TestConverter::simple()
{
	std::string str = "#s";
	Util::Args args;

	args.keywords['s'] = "test";
	std::string result = Util::convert(str, args);

	CPPUNIT_ASSERT_MESSAGE(message("test", result), result == "test");
}

void TestConverter::two()
{
	std::string str = "#s #c";
	Util::Args args;

	args.keywords['s'] = "s";
	args.keywords['c'] = "c";

	std::string result = Util::convert(str, args);

	CPPUNIT_ASSERT_MESSAGE(message("s c", result), result == "s c");
}

void TestConverter::oneAbsent()
{
	std::string str = "#s #x #c";
	Util::Args args;

	args.keywords['s'] = "s";
	args.keywords['c'] = "c";

	std::string result = Util::convert(str, args);

	CPPUNIT_ASSERT_MESSAGE(message("s #x c", result), result == "s #x c");
}

void TestConverter::replaceByPattern()
{
	std::string str = "#a #b";
	Util::Args args;

	args.keywords['a'] = "#c";
	args.keywords['c'] = "FAIL";
	args.keywords['b'] = "b";

	std::string result = Util::convert(str, args);

	CPPUNIT_ASSERT_MESSAGE(message("#c b", result), result == "#c b");
}

void TestConverter::dateFlags()
{
	std::string str = "%h";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertDate);
	CPPUNIT_ASSERT_MESSAGE(message("number", result), result != "%h");

	result = Util::convert(str, args);
	CPPUNIT_ASSERT_MESSAGE(message("number", result), result == "%h");
}

void TestConverter::homeFlags()
{
	auto home = std::getenv("HOME");

	if (home == nullptr)
		return;

	std::string str = "~";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertHome);
	CPPUNIT_ASSERT_MESSAGE(message(home, result), home == result);

	result = Util::convert(str, args);
	CPPUNIT_ASSERT_MESSAGE(message(result, str), result == str);
}

void TestConverter::envFlags()
{
	auto home = std::getenv("HOME");

	if (home == nullptr)
		return;

	std::string str = "${HOME}";
	Util::Args args;

	std::string result = Util::convert(str, args, Util::ConvertEnv);
	CPPUNIT_ASSERT_MESSAGE(message(home, result), home == result);

	result = Util::convert(str, args);
	CPPUNIT_ASSERT_MESSAGE(message(result, str), result == str);
}

} // !irccd

int main()
{
	using namespace irccd;

	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(TestConverter::suite());

	return runnerText.run("", false) == 1 ? 0 : 1;
}
