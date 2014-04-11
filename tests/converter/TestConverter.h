/*
 * TestConverter.h -- string conversion for patterns
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

#ifndef _TEST_CONVERTER_H_
#define _TEST_CONVERTER_H_

#include <string>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

namespace irccd
{

class TestConverter : public CppUnit::TestFixture
{
private:
	CPPUNIT_TEST_SUITE(TestConverter);
	CPPUNIT_TEST(useless);
	CPPUNIT_TEST(simple);
	CPPUNIT_TEST(two);
	CPPUNIT_TEST(oneAbsent);
	CPPUNIT_TEST(replaceByPattern);
	CPPUNIT_TEST(dateFlags);
	CPPUNIT_TEST(homeFlags);
	CPPUNIT_TEST(envFlags);
	CPPUNIT_TEST_SUITE_END();

	std::string message(const std::string &expected,
			    const std::string &result);

public:
	void useless();
	void simple();
	void two();
	void oneAbsent();
	void replaceByPattern();
	void dateFlags();
	void homeFlags();
	void envFlags();
};

} // !irccd

#endif // !_TEST_PARSER_H_
