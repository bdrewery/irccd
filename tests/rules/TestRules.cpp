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
 * events	= "onCommand"
 * action	= drop
 *
 * #
 * # However, the same onCommand on #staff is allowed on server "unsafe"
 * #
 * [rule]	#2
 * servers	= "unsafe"
 * channels	= "#staff"
 * events	= "onCommand"
 * action	= accept
 *
 * #
 * # Plugin game is only allowed on server "malikania" and "localhost",
 * # channel "#games" and events "onMessage, onCommand".
 * #
 * # The first rule #3-1 disable the plugin game for every server, it is
 * # reenabled again with the #3-2.
 * #
 * [rule]	#3-1
 * plugins	= "game"
 * action	= drop
 *
 * [rule]	#3-2
 * servers	= "malikania localhost"
 * channels	= "#games"
 * plugins	= "game"
 * events	= "onMessage onCommand"
 * action	= accept
 */
class RulesTest : public testing::Test {
protected:
	RulesTest()
	{
		auto &manager = RuleManager::instance();

		// #1
		{
			manager.add({
				RuleMap{		},
				RuleMap{ "#staff"	},
				RuleMap{		},
				RuleMap{		},
				RuleMap{ "onCommand"	},
				RuleAction::Drop
			});
		}

		// #2
		{
			manager.add({
				RuleMap{ "unsafe"	},
				RuleMap{ "#staff"	},
				RuleMap{		},
				RuleMap{		},
				RuleMap{ "onCommand"	},
				RuleAction::Accept
			});
		}

		// #3-1
		{
			manager.add({
				RuleMap{},
				RuleMap{},
				RuleMap{},
				RuleMap{"game"},
				RuleMap{},
				RuleAction::Drop
			});
		}

		// #3-2
		{
			manager.add({
				RuleMap{ "malikania", "localhost"	},
				RuleMap{ "#games"			},
				RuleMap{ 				},
				RuleMap{ "game"				},
				RuleMap{ "onCommand", "onMessage"	},
				RuleAction::Accept
			});
		}
	}

	~RulesTest()
	{
		RuleManager::instance().clear();
	}
};

TEST_F(RulesTest, basicMatch1)
{
	Rule m;

	/*
	 * [rule]
	 */
	ASSERT_TRUE(m.match("freenode", "#test", "a", "", ""));
	ASSERT_TRUE(m.match("", "", "", "", ""));
}

TEST_F(RulesTest, basicMatch2)
{
	Rule m{RuleMap{"freenode"}};

	/*
	 * [rule]
	 * servers	= "freenode"
	 */

	ASSERT_TRUE(m.match("freenode", "#test", "a", "", ""));
	ASSERT_FALSE(m.match("malikania", "#test", "a", "", ""));
	ASSERT_TRUE(m.match("freenode", "", "jean", "", "onMessage"));
}

TEST_F(RulesTest, basicMatch3)
{
	Rule m{RuleMap{"freenode"}, RuleMap{"#staff"}};

	/*
	 * [rule]
	 * servers	= "freenode"
	 * channels	= "#staff"
	 */

	ASSERT_TRUE(m.match("freenode", "#staff", "a", "", ""));
	ASSERT_FALSE(m.match("freenode", "#test", "a", "", ""));
	ASSERT_FALSE(m.match("malikania", "#staff", "a", "", ""));
}

TEST_F(RulesTest, basicMatch4)
{
	Rule m{RuleMap{"malikania"}, RuleMap{"#staff"}, RuleMap{"a"}};

	/*
	 * [rule]
	 * servers	= "malikania"
	 * channels	= "#staff"
	 * plugins	= "a"
	 */

	ASSERT_TRUE(m.match("malikania", "#staff", "a", "",""));
	ASSERT_FALSE(m.match("malikania", "#staff", "b", "", ""));
	ASSERT_FALSE(m.match("freenode", "#staff", "a", "", ""));
}

TEST_F(RulesTest, complexMatch1)
{
	Rule m{RuleMap{"malikania", "freenode"}};

	/*
	 * [rule]
	 * servers	= "malikania freenode"
	 */

	ASSERT_TRUE(m.match("malikania", "", "", "", ""));
	ASSERT_TRUE(m.match("freenode", "", "", "", ""));
	ASSERT_FALSE(m.match("no", "", "", "", ""));
}

TEST_F(RulesTest, basicSolve)
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	ASSERT_TRUE(manager.solve("malikania", "#staff", "", "a", "onMessage"));

	/* Allowed */
	ASSERT_TRUE(manager.solve("freenode", "#staff", "", "b", "onTopic"));

	/* Not allowed */
	ASSERT_FALSE(manager.solve("malikania", "#staff", "", "", "onCommand"));

	/* Not allowed */
	ASSERT_FALSE(manager.solve("freenode", "#staff", "", "c", "onCommand"));

	/* Allowed */
	ASSERT_TRUE(manager.solve("unsafe", "#staff", "", "c", "onCommand"));
}

TEST_F(RulesTest, gamesSolve)
{
	auto &manager = RuleManager::instance();

	/* Allowed */
	ASSERT_TRUE(manager.solve("malikania", "#games", "", "game", "onMessage"));

	/* Allowed */
	ASSERT_TRUE(manager.solve("localhost", "#games", "", "game", "onMessage"));

	/* Allowed */
	ASSERT_TRUE(manager.solve("malikania", "#games", "", "game", "onCommand"));

	/* Not allowed */
	ASSERT_FALSE(manager.solve("malikania", "#games", "", "game", "onQuery"));

	/* Not allowed */
	ASSERT_FALSE(manager.solve("freenode", "#no", "", "game", "onMessage"));

	/* Not allowed */
	ASSERT_FALSE(manager.solve("malikania", "#test", "", "game", "onMessage"));
}

} // !irccd

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
