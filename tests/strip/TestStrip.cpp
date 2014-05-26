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

#include <cppunit/TextTestRunner.h>

#include <common/Util.h>

#include "TestStrip.h"

namespace irccd {

void TestStrip::left()
{
	std::string value = "   123";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("123"), result);
}

void TestStrip::right()
{
	std::string value = "123   ";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("123"), result);
}

void TestStrip::both()
{
	std::string value = "   123   ";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("123"), result);
}

void TestStrip::none()
{
	std::string value = "without";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("without"), result);
}

void TestStrip::betweenEmpty()
{
	std::string value = "one list";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("one list"), result);
}

void TestStrip::betweenLeft()
{
	std::string value = "  space at left";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("space at left"), result);
}

void TestStrip::betweenRight()
{
	std::string value = "space at right  ";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("space at right"), result);
}

void TestStrip::betweenBoth()
{
	std::string value = "  space at both  ";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string("space at both"), result);
}

void TestStrip::empty()
{
	std::string value = "    ";
	std::string result = Util::strip(value);

	CPPUNIT_ASSERT_EQUAL(std::string(""), result);
}

} // !irccd

int main()
{
	using namespace irccd;

	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(TestStrip::suite());

	return runnerText.run("", false) == 1 ? 0 : 1;
}
