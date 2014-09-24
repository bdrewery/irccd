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

#include <gtest/gtest.h>

#include <RuleManager.h>
#include <Rule.h>

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
class RulesTest : public testing::Test {
protected:
	RulesTest()
	{
		auto &manager = RuleManager::instance();

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

	~RulesTest()
	{
		RuleManager::instance().clear();
	}
};

TEST_F(RulesTest, basicMatch1)
{
	RuleMatch m;

	/*
	 * [rule]
	 */
	ASSERT_TRUE(m.match("freenode", "#test", "a"));
	ASSERT_TRUE(m.match("", "", ""));
}

TEST_F(RulesTest, basicMatch2)
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "freenode"
	 */
	m.addServer("freenode");

	ASSERT_TRUE(m.match("freenode", "#test", "a"));
	ASSERT_FALSE(m.match("malikania", "#test", "a"));
}

TEST_F(RulesTest, basicMatch3)
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "freenode"
	 * channels	= "#staff"
	 */
	m.addServer("freenode");
	m.addChannel("#staff");

	ASSERT_TRUE(m.match("freenode", "#staff", "a"));
	ASSERT_FALSE(m.match("freenode", "#test", "a"));
	ASSERT_FALSE(m.match("malikania", "#staff", "a"));
}

TEST_F(RulesTest, basicMatch4)
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

	ASSERT_TRUE(m.match("malikania", "#staff", "a"));
	ASSERT_FALSE(m.match("malikania", "#staff", "b"));
	ASSERT_FALSE(m.match("freenode", "#staff", "a"));
}

TEST_F(RulesTest, complexMatch1)
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania freenode"
	 */
	m.addServer("malikania");
	m.addServer("freenode");

	ASSERT_TRUE(m.match("malikania", "", ""));
	ASSERT_TRUE(m.match("freenode", "", ""));
	ASSERT_FALSE(m.match("no", "", ""));
}

TEST_F(RulesTest, complexMatch2)
{
	RuleMatch m;

	/*
	 * [rule]
	 * servers	= "malikania !freenode"
	 */
	m.addServer("malikania");
	m.addServer("freenode", false);

	ASSERT_TRUE(m.match("malikania", "", ""));
	ASSERT_FALSE(m.match("freenode", "", ""));
	ASSERT_FALSE(m.match("no", "", ""));
}

TEST_F(RulesTest, complexMatch3)
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

	ASSERT_FALSE(m.match("", "", ""));
	ASSERT_FALSE(m.match("", "#games", ""));
	ASSERT_FALSE(m.match("", "#test", ""));

	ASSERT_TRUE(m.match("malikania", "#staff", ""));
	ASSERT_TRUE(m.match("localhost", "#staff", ""));
	ASSERT_FALSE(m.match("malikania", "#test", ""));
	ASSERT_FALSE(m.match("localhost", "#test", ""));
	ASSERT_FALSE(m.match("freenode", "#staff", ""));
	ASSERT_FALSE(m.match("freenode", "#test", ""));
	ASSERT_FALSE(m.match("no", "", ""));
	ASSERT_FALSE(m.match("no", "#test", ""));
	ASSERT_FALSE(m.match("no", "#staff", ""));
}

TEST_F(RulesTest, complexMatch4)
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

	ASSERT_FALSE(m.match("", "", ""));
	ASSERT_FALSE(m.match("", "", "a"));
	ASSERT_FALSE(m.match("", "", "b"));
	ASSERT_FALSE(m.match("", "#games", ""));
	ASSERT_FALSE(m.match("", "#test", ""));
	ASSERT_FALSE(m.match("", "#games", "a"));
	ASSERT_FALSE(m.match("", "#test", "b"));

	ASSERT_FALSE(m.match("malikania", "", ""));
	ASSERT_FALSE(m.match("malikania", "#games", ""));
	ASSERT_FALSE(m.match("malikania", "#test", ""));
	ASSERT_FALSE(m.match("malikania", "", "a"));
	ASSERT_FALSE(m.match("malikania", "", "b"));
	ASSERT_TRUE(m.match("malikania", "#games", "a"));
	ASSERT_FALSE(m.match("malikania", "#games", "b"));
	ASSERT_FALSE(m.match("malikania", "#test", "a"));
	ASSERT_FALSE(m.match("malikania", "#test", "b"));

	ASSERT_FALSE(m.match("localhost", "", ""));
	ASSERT_FALSE(m.match("localhost", "#games", ""));
	ASSERT_FALSE(m.match("localhost", "#test", ""));
	ASSERT_FALSE(m.match("localhost", "", "a"));
	ASSERT_FALSE(m.match("localhost", "", "b"));
	ASSERT_TRUE(m.match("localhost", "#games", "a"));
	ASSERT_FALSE(m.match("localhost", "#games", "b"));
	ASSERT_FALSE(m.match("localhost", "#test", "a"));
	ASSERT_FALSE(m.match("localhost", "#test", "b"));

	ASSERT_FALSE(m.match("freenode", "", ""));
	ASSERT_FALSE(m.match("freenode", "#games", ""));
	ASSERT_FALSE(m.match("freenode", "#test", ""));
	ASSERT_FALSE(m.match("freenode", "#games", "a"));
	ASSERT_FALSE(m.match("freenode", "#games", "b"));
	ASSERT_FALSE(m.match("freenode", "#test", "a"));
	ASSERT_FALSE(m.match("freenode", "#test", "b"));
}

TEST_F(RulesTest, basicSolve)
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	auto result = manager.solve("malikania", "#staff", "onMessage", "a");
	ASSERT_TRUE(result.enabled);

	/* Allowed */
	result = manager.solve("freenode", "#staff", "onTopic", "b");
	ASSERT_TRUE(result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#staff", "onCommand", "c");
	ASSERT_FALSE(result.enabled);

	/* Not allowed */
	result = manager.solve("freenode", "#staff", "onCommand", "c");
	ASSERT_FALSE(result.enabled);

	/* Allowed */
	result = manager.solve("unsafe", "#staff", "onCommand", "c");
	ASSERT_TRUE(result.enabled);
}

TEST_F(RulesTest, gamesSolve)
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	auto result = manager.solve("malikania", "#games", "onMessage", "game");
	ASSERT_TRUE(result.enabled);

	/* Allowed */
	result = manager.solve("localhost", "#games", "onMessage", "game");
	ASSERT_TRUE(result.enabled);

	/* Allowed */
	result = manager.solve("malikania", "#games", "onCommand", "game");
	ASSERT_TRUE(result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#games", "onQuery", "game");
	ASSERT_FALSE(result.enabled);

	/* Not allowed */
	result = manager.solve("freenode", "#no", "onMessage", "game");
	ASSERT_FALSE(result.enabled);

	/* Not allowed */
	result = manager.solve("malikania", "#test", "onMessage", "game");
	ASSERT_FALSE(result.enabled);
}

} // !irccd

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
