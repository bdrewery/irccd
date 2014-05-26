/*
 * TestStrip.h -- test Util::strip
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

#ifndef _TEST_STRIP_H_
#define _TEST_STRIP_H_

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

namespace irccd {

class TestStrip : public CppUnit::TestFixture
{
private:
	CPPUNIT_TEST_SUITE(TestStrip);
	CPPUNIT_TEST(left);
	CPPUNIT_TEST(right);
	CPPUNIT_TEST(both);
	CPPUNIT_TEST(none);
	CPPUNIT_TEST(betweenEmpty);
	CPPUNIT_TEST(betweenLeft);
	CPPUNIT_TEST(betweenRight);
	CPPUNIT_TEST(betweenBoth);
	CPPUNIT_TEST(empty);
	CPPUNIT_TEST_SUITE_END();

public:
	void left();
	void right();
	void both();
	void none();
	void betweenEmpty();
	void betweenLeft();
	void betweenRight();
	void betweenBoth();
	void empty();
};

} // !irccd

#endif // !_TEST_STRIP_H_
