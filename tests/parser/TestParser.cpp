/*
 * TestParser.cpp -- test the config file parser
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include <cppunit/TextTestRunner.h>

#include <common/Parser.h>

#include "TestParser.h"

namespace irccd
{

void TestParser::openCorrect()
{
	Parser config("parser/configs/Correct.conf");
	Section s;

	CPPUNIT_ASSERT_EQUAL(config.open(), true);
	CPPUNIT_ASSERT_MESSAGE("Required section general not found", config.hasSection("general"));
	CPPUNIT_ASSERT_MESSAGE("Required section server not found", config.hasSection("server"));
	CPPUNIT_ASSERT_MESSAGE("Function hasSection said there is foo section but it should not", !config.hasSection("foo"));

	try
	{
		s = config.requireSection("general");

		CPPUNIT_ASSERT_MESSAGE("Required option verbose not found", s.hasOption("verbose"));
		CPPUNIT_ASSERT_EQUAL(s.requireOption<bool>("verbose"), true);

		s = config.requireSection("server");

		CPPUNIT_ASSERT_MESSAGE("Required option name not found", s.hasOption("name"));
		CPPUNIT_ASSERT_EQUAL(s.requireOption<std::string>("name"), std::string("localhost"));
	}
	catch (NotFoundException ex)
	{
		CPPUNIT_ASSERT_MESSAGE("Require thrown exception on correct section / option", false);
	}
}

void TestParser::openMultiples()
{
	Parser config("parser/configs/Multiple.conf");

	CPPUNIT_ASSERT_EQUAL(config.open(), true);

	/*
	 * The real number if 3, don't forget there is a default root section.
	 */
	CPPUNIT_ASSERT_EQUAL(3, static_cast<int>(config.getSections().size()));

	int count = 0;
	for (auto s : config.findSections("server"))
	{
		++ count;

		try
		{
			CPPUNIT_ASSERT_EQUAL(s.requireOption<std::string>("name"), std::to_string(count));
		}
		catch (NotFoundException ex)
		{
			CPPUNIT_ASSERT_MESSAGE("Require thrown exception on correct section / option", false);
		}
	}

	CPPUNIT_ASSERT_EQUAL(count, 2);
}

} // !irccd

int main()
{
	using namespace irccd;

	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(TestParser::suite());

	return runnerText.run("", false) == 1 ? 0 : 1;
}
