/*
 * TestSplit.cpp -- test Util::split
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

#include "TestSplit.h"

namespace irccd {

using String	= std::string;
using List	= std::vector<std::string>;

std::ostream &operator<<(std::ostream &stream, List list)
{
	stream << "[ ";

	for (size_t i = 0; i < list.size(); ++i) {
		stream << list[i];
		if (i != list.size() - 1)
			stream << ", ";
	}

	stream << " ]";

	return stream;
}

std::string message(const List &expected, const List &result)
{
	std::ostringstream oss;

	oss << "expected: " << expected << std::endl;
	oss << "actual: " << result << std::endl;

	return oss.str();
}

#define EXPECT(e, r)							\
	CPPUNIT_ASSERT_MESSAGE(message(e, r), e == r)

void TestSplit::simple()
{
	List expected { "a", "b" };
	List result = Util::split("a;b", ";");

	EXPECT(expected, result);
}

void TestSplit::cut()
{
	List expected { "msg", "#staff", "foo bar baz" };
	List result = Util::split("msg;#staff;foo bar baz", ";", 3);

	EXPECT(expected, result);
}

} // !irccd

int main()
{
	using namespace irccd;

	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(TestSplit::suite());

	return runnerText.run("", false) == 1 ? 0 : 1;
}
