/*
 * TestRules.cpp -- test irccd rules
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

#include <RuleManager.h>
#include <Rule.h>

#include "TestRules.h"

namespace irccd {

/*
 * Simulate the following rules configuration:
 *
 * #
 * # On all servers, each channel #staff can't use the onCommand event,
 * # everything else is allowed.
 * #
 * [rule]	#1
 * servers	= ""
 * channels	= "#staff"
 * set-events	= "!onCommand"
 *
 * #
 * # However, the same onCommand on #staff is allowed on server "unsafe"
 * #
 * [rule]	#2
 * servers	= "unsafe"
 * channels	= "#staff"
 * set-events	= "onCommand"
 *
 * #
 * # Plugin game is only allowed on server "malikania" and "localhost",
 * # channel "#games" and events "onMessage, onCommand".
 * #
 * # The first rule #3-1 disable the plugin game for every server, it is
 * # reenabled again with the #3-2.
 * #
 * [rule]	#3-1
 * set-plugins	= "!game"
 *
 * [rule]	#3-2
 * servers	= "malikania"
 * channels	= "#games"
 * plugins	= "game"
 * set-plugins	= "game"
 * set-events	= "onMessage onCommand"
 */
void TestRules::setUp()
{
	RuleManager &manager = RuleManager::instance();	

	// #1
	{
		RuleMatch match;
		RuleProperties properties;

		match.addChannel("#staff");
		properties.setEvent("onCommand", false);

		manager.add(Rule(match, properties));
	}

	// #2
	{
		RuleMatch match;
		RuleProperties properties;

		match.addServer("unsafe");
		match.addChannel("#staff");
		properties.setEvent("onCommand");

		manager.add(Rule(match, properties));
	}

	// #3-1
	{
		RuleProperties properties;

		properties.setPlugin("game", false);

		manager.add(Rule(RuleMatch(), properties));
	}

	// #3-2
	{
		RuleMatch match;
		RuleProperties properties;

		match.addServer("malikania");
		match.addServer("localhost");
		match.addChannel("#games");
		match.addPlugin("game");

		properties.setPlugin("game");
		properties.setEvent("onCommand");
		properties.setEvent("onMessage");

		manager.add(Rule(match, properties));
	}
}

void TestRules::tearDown()
{
	RuleManager::instance().clear();
}

void TestRules::basicMatch1()
{
	RuleMatch m;

	/*
	 * [rule]
	 */
	CPPUNIT_ASSERT(m.match("freenode", "#test", "a"));
	CPPUNIT_ASSERT(m.match("", "", ""));
}

void TestRules::basicMatch2()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "freenode"
	 */
	m.addServer("freenode");

	CPPUNIT_ASSERT(m.match("freenode", "#test", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "#test", "a"));
}

void TestRules::basicMatch3()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "freenode"
	 * channels	= "#staff"
	 */
	m.addServer("freenode");
	m.addChannel("#staff");

	CPPUNIT_ASSERT(m.match("freenode", "#staff", "a"));
	CPPUNIT_ASSERT(!m.match("freenode", "#test", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "#staff", "a"));
}

void TestRules::basicMatch4()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania"
	 * channels	= "#staff"
	 * plugins	= "a"
	 */
	m.addServer("malikania");
	m.addChannel("#staff");
	m.addPlugin("a");

	CPPUNIT_ASSERT(m.match("malikania", "#staff", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "#staff", "b"));
	CPPUNIT_ASSERT(!m.match("freenode", "#staff", "a"));
}

void TestRules::complexMatch1()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania freenode"
	 */
	m.addServer("malikania");
	m.addServer("freenode");

	CPPUNIT_ASSERT(m.match("malikania", "", ""));
	CPPUNIT_ASSERT(m.match("freenode", "", ""));
	CPPUNIT_ASSERT(!m.match("no", "", ""));
}

void TestRules::complexMatch2()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania !freenode"
	 */
	m.addServer("malikania");
	m.addServer("freenode", false);

	CPPUNIT_ASSERT(m.match("malikania", "", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "", ""));
	CPPUNIT_ASSERT(!m.match("no", "", ""));
}

void TestRules::complexMatch3()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania !freenode localhost"
	 * channels	= "#staff !#test"
	 */
	m.addServer("malikania");
	m.addServer("freenode", false);
	m.addServer("localhost");
	m.addChannel("#staff");
	m.addChannel("#test", false);

	CPPUNIT_ASSERT(!m.match("", "", ""));
	CPPUNIT_ASSERT(!m.match("", "#games", ""));
	CPPUNIT_ASSERT(!m.match("", "#test", ""));

	CPPUNIT_ASSERT(m.match("malikania", "#staff", ""));
	CPPUNIT_ASSERT(m.match("localhost", "#staff", ""));
	CPPUNIT_ASSERT(!m.match("malikania", "#test", ""));
	CPPUNIT_ASSERT(!m.match("localhost", "#test", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "#staff", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "#test", ""));
	CPPUNIT_ASSERT(!m.match("no", "", ""));
	CPPUNIT_ASSERT(!m.match("no", "#test", ""));
	CPPUNIT_ASSERT(!m.match("no", "#staff", ""));
}

void TestRules::complexMatch4()
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania !freenode localhost"
	 * channels	= "#games !#test"
	 * plugins	= "a !b"
	 */
	m.addServer("malikania");
	m.addServer("freenode", false);
	m.addServer("localhost");
	m.addChannel("#games");
	m.addChannel("#test", false);
	m.addPlugin("a");
	m.addPlugin("b", false);

	CPPUNIT_ASSERT(!m.match("", "", ""));
	CPPUNIT_ASSERT(!m.match("", "", "a"));
	CPPUNIT_ASSERT(!m.match("", "", "b"));
	CPPUNIT_ASSERT(!m.match("", "#games", ""));
	CPPUNIT_ASSERT(!m.match("", "#test", ""));
	CPPUNIT_ASSERT(!m.match("", "#games", "a"));
	CPPUNIT_ASSERT(!m.match("", "#test", "b"));

	CPPUNIT_ASSERT(!m.match("malikania", "", ""));
	CPPUNIT_ASSERT(!m.match("malikania", "#games", ""));
	CPPUNIT_ASSERT(!m.match("malikania", "#test", ""));
	CPPUNIT_ASSERT(!m.match("malikania", "", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "", "b"));
	CPPUNIT_ASSERT(m.match("malikania", "#games", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "#games", "b"));
	CPPUNIT_ASSERT(!m.match("malikania", "#test", "a"));
	CPPUNIT_ASSERT(!m.match("malikania", "#test", "b"));

	CPPUNIT_ASSERT(!m.match("localhost", "", ""));
	CPPUNIT_ASSERT(!m.match("localhost", "#games", ""));
	CPPUNIT_ASSERT(!m.match("localhost", "#test", ""));
	CPPUNIT_ASSERT(!m.match("localhost", "", "a"));
	CPPUNIT_ASSERT(!m.match("localhost", "", "b"));
	CPPUNIT_ASSERT(m.match("localhost", "#games", "a"));
	CPPUNIT_ASSERT(!m.match("localhost", "#games", "b"));
	CPPUNIT_ASSERT(!m.match("localhost", "#test", "a"));
	CPPUNIT_ASSERT(!m.match("localhost", "#test", "b"));

	CPPUNIT_ASSERT(!m.match("freenode", "", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "#games", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "#test", ""));
	CPPUNIT_ASSERT(!m.match("freenode", "#games", "a"));
	CPPUNIT_ASSERT(!m.match("freenode", "#games", "b"));
	CPPUNIT_ASSERT(!m.match("freenode", "#test", "a"));
	CPPUNIT_ASSERT(!m.match("freenode", "#test", "b"));
}

void TestRules::basic()
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	auto result = manager.solve("malikania", "#staff", "onMessage", "a");
	CPPUNIT_ASSERT(result.enabled);

	/* Allowed */
	result = manager.solve("freenode", "#staff", "onTopic", "b");
	CPPUNIT_ASSERT(result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#staff", "onCommand", "c");
	CPPUNIT_ASSERT(!result.enabled);

	/* Not allowed */
	result = manager.solve("freenode", "#staff", "onCommand", "c");
	CPPUNIT_ASSERT(!result.enabled);

	/* Allowed */
	result = manager.solve("unsafe", "#staff", "onCommand", "c");
	CPPUNIT_ASSERT(result.enabled);
}

void TestRules::games()
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	auto result = manager.solve("malikania", "#games", "onMessage", "game");
	CPPUNIT_ASSERT(result.enabled);

	/* Allowed */
	result = manager.solve("localhost", "#games", "onMessage", "game");
	CPPUNIT_ASSERT(result.enabled);

	/* Allowed */
	result = manager.solve("malikania", "#games", "onCommand", "game");
	CPPUNIT_ASSERT(result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#games", "onQuery", "game");
	CPPUNIT_ASSERT(!result.enabled);

	/* Not allowed */
	result = manager.solve("freenode", "#no", "onMessage", "game");
	CPPUNIT_ASSERT(!result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#test", "onMessage", "game");
	CPPUNIT_ASSERT(!result.enabled);
}

} // !irccd

int main()
{
	using namespace irccd;

	CppUnit::TextTestRunner runnerText;

	runnerText.addTest(TestRules::suite());

	return runnerText.run("", false) == 1 ? 0 : 1;
}
