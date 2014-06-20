/*
 * TestRules.h -- test irccd rules
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

#ifndef _TEST_RULES_H_
#define _TEST_RULES_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

namespace irccd {

class TestRules : public CppUnit::TestFixture
{
private:
	CPPUNIT_TEST_SUITE(TestRules);
	/*
	 * RuleMatch class test
	 */
	CPPUNIT_TEST(basicMatch1);
	CPPUNIT_TEST(basicMatch2);
	CPPUNIT_TEST(basicMatch3);
	CPPUNIT_TEST(basicMatch4);
	CPPUNIT_TEST(complexMatch1);
	CPPUNIT_TEST(complexMatch2);
	CPPUNIT_TEST(complexMatch3);
	CPPUNIT_TEST(complexMatch4);

	CPPUNIT_TEST(basic);
	CPPUNIT_TEST(games);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() override;
	void tearDown() override;

	/*
	 * RuleMatch class test
	 */
	void basicMatch1();
	void basicMatch2();
	void basicMatch3();
	void basicMatch4();
	void complexMatch1();
	void complexMatch2();
	void complexMatch3();
	void complexMatch4();

	void basic();
	void games();
};

} // !irccd

#endif // !_TEST_RULES_H_
