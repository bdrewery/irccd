/*
 * Test.cpp -- test plugins
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

#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"
#include "Server.h"
#include "Test.h"

namespace irccd {

namespace {

using HelpFunction	= std::function<void ()>;
using TestFunction	= std::function<void (std::shared_ptr<Plugin>, std::shared_ptr<Server>, int, char **)>;

class FakeServer : public Server {
public:
	FakeServer(const Info &info, const Identity &identity)
		: Server(info, identity, RetryInfo())
	{
	}

	void cnotice(const std::string &channel, const std::string &message)
	{
		Logger::log("test: notice: (%s) %s", channel.c_str(), message.c_str());
	}

	void invite(const std::string &target, const std::string &channel)
	{
		Logger::log("test: invite: %s invited to channel %s",
		    target.c_str(), channel.c_str());
	}

	void join(const std::string &name, const std::string &password)
	{
		Logger::log("test: join: joining channel %s with password \"%s\"",
		    name.c_str(), password.c_str());
	}

	void kick(const std::string &name, const std::string &channel, const std::string &reason)
	{
		Logger::log("test: kick: kicking %s from channel %s reason \"%s\"",
		    name.c_str(), channel.c_str(), reason.c_str());
	}

	void me(const std::string &target, const std::string &message)
	{
		Logger::log("test: me: * %s: %s", target.c_str(), message.c_str());
	}

	void mode(const std::string &channel, const std::string &mode)
	{
		Logger::log("test: mode: %s mode %s", channel.c_str(), mode.c_str());
	}

	void names(const std::string &channel)
	{
		Logger::log("test: names: gettings names from %s", channel.c_str());
	}

	void nick(const std::string &nick)
	{
		Logger::log("test: nick: changing nick to %s", nick.c_str());
	}

	void notice(const std::string &nickname, const std::string &message)
	{
		Logger::log("test: notice: from %s: %s", nickname.c_str(), message.c_str());
	}

	void part(const std::string &channel, const std::string &reason)
	{
		Logger::log("test: part: leaving channel %s [%s]", channel.c_str(), reason.c_str());
	}

	void query(const std::string &who, const std::string &message)
	{
		Logger::log("test: query: private message from %s: %s",
		    who.c_str(), message.c_str());
	}

	void say(const std::string &target, const std::string &message)
	{
		Logger::log("test: say: said %s to %s", message.c_str(),
		    target.c_str());
	}

	void topic(const std::string &channel, const std::string &topic)
	{
		Logger::log("test: topic: changing %s topic to %s",
		    channel.c_str(), topic.c_str());
	}
};

// {{{ Help functions

static void helpCommand()
{
	Logger::warn("usage: %s test file onCommand channel who message\n", getprogname());
	Logger::warn("Do a fake onCommand function call. This command does not");
	Logger::warn("require to specify a plugin name, it will use the tested one.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onCommand #staff markand \"will I be rich?\"", getprogname());
}

static void helpConnect()
{
	Logger::warn("usage: %s test file onConnect\n", getprogname());
	Logger::warn("Do a fake successful connection.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onConnect");
}

static void helpChannelNotice()
{
	Logger::warn("usage: %s test file onChannelNotice nick channel notice\n", getprogname());
	Logger::warn("Send a notice to the specified channel.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNotice mick #staff \"#staff is not #offtopic\"", getprogname());
}

static void helpInvite()
{
	Logger::warn("usage: %s test file onInvite channel who\n", getprogname());
	Logger::warn("Do a fake inviation from who to a specific channel.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onInvite #staff john", getprogname());
}

static void helpJoin()
{
	Logger::warn("usage: %s test file onJoin channel who\n", getprogname());
	Logger::warn("Join the channel. The parameter who is the person");
	Logger::warn("nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onJoin #staff francis", getprogname());
}

static void helpKick()
{
	Logger::warn("usage: %s test file onKick channel who kicked reason\n", getprogname());
	Logger::warn("Fake a kick from a specific channel, the reason may be empty.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onKick #staff markand julia");
	Logger::warn("\t%s test file onKick #staff francis markand \"You're not nice with her\"");
}

static void helpMe()
{
	Logger::warn("usage: %s test file onMe channel who message\n", getprogname());
	Logger::warn("Send a CTCP Action to the channel. It is usually rendered like this:\n");
	Logger::warn("\t* jean is eating an apple.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onMe #staff francis \"like that\"", getprogname());
}

static void helpMessage()
{
	Logger::warn("usage: %s test file onMessage channel who message\n", getprogname());
	Logger::warn("Send a message to the specific channel.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onMessage #staff francis \"Hello All\"", getprogname());
}

static void helpMode()
{
	Logger::warn("usage: %s test file onMode channel who mode [modeArg]\n", getprogname());
	Logger::warn("Do a fake channel mode change. The who parameter is the one who");
	Logger::warn("channel mode. An optional mode argument can be specified.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onMode #staff john +t", getprogname());
	Logger::warn("\t%s test file onMode #staff john +k #overflow", getprogname());
}

static void helpNick()
{
	Logger::warn("usage: %s test file onNick oldnick newnick\n", getprogname());
	Logger::warn("Do a fake nick change.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNick john_ john");
}

static void helpNotice()
{
	Logger::warn("usage: %s test file onNotice who target notice\n", getprogname());
	Logger::warn("Send a private notice to the target nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNotice john mick \"Please stop flooding\"", getprogname());
}

static void helpPart()
{
	Logger::warn("usage: %s test file onPart channel who reason\n", getprogname());
	Logger::warn("Simulate a target departure specified by foo on the channel. The");
	Logger::warn("reason may be empty.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onPart #staff john \"Do not like that channel\"", getprogname());
}

static void helpQuery()
{
	Logger::warn("usage: %s test file onQuery who message\n", getprogname());
	Logger::warn("Simulate a private query, who is the sender.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onQuery john \"Do you want some?\"", getprogname());
}

static void helpTopic()
{
	Logger::warn("usage: %s test file onTopic channel who topic\n", getprogname());
	Logger::warn("Change the topic on a fake server. Topic may be empty so that");
	Logger::warn("clear the old one.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onTopic #staff markand \"I'm your new god little girls\"", getprogname());
}

static void helpUserMode()
{
	Logger::warn("usage: %s test file onUserMode who mode\n", getprogname());
	Logger::warn("Fake a user mode change, remember that who is the one that changed");
	Logger::warn("your mode, so you may check the `server' Lua API if you want your");
	Logger::warn("own nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onUserMode john +i");
}

std::unordered_map<std::string, HelpFunction> helpCommands {
	{ "onCommand",		helpCommand		},
	{ "onConnect",		helpConnect		},
	{ "onChannelNotice",	helpChannelNotice	},
	{ "onInvite",		helpInvite		},
	{ "onJoin",		helpJoin		},
	{ "onKick",		helpKick		},
	{ "onMe",		helpMe			},
	{ "onMessage",		helpMessage		},
	{ "onMode",		helpMode		},
	{ "onNick",		helpNick		},
	{ "onNotice",		helpNotice		},
	{ "onPart",		helpPart		},
	{ "onQuery",		helpQuery		},
	{ "onTopic",		helpTopic		},
	{ "onUserMode",		helpUserMode		}
};

// }}}

// {{{ Test functions

void testCommand(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onCommand requires 3 arguments");
	else {
		/*
		 * Fake a !<plugin> command
		 */
		auto command = "!" + Process::info(p->getState()).name + " " + std::string(argv[2]);

		p->onMessage(s, argv[0], argv[1], command);
	}
}

void testConnect(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int, char **)
{
	p->onConnect(s);
}

void testChannelNotice(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onChannelNotice requires 3 arguments");
	else
		p->onChannelNotice(s, argv[0], argv[1], argv[2]);
}

void testInvite(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 2)
		Logger::warn("test: onInvite requires 2 arguments");
	else
		p->onInvite(s, argv[0], argv[1]);
}

void testJoin(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 2)
		Logger::warn("test: onJoin requires 2 arguments");
	else
		p->onJoin(s, argv[0], argv[1]);
}

void testKick(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	std::string reason;

	if (argc < 3)
		Logger::warn("test: onKick requires at least 3 arguments");
	else {
		if (argc >= 4)
			reason = argv[3];

		p->onKick(s, argv[0], argv[1], argv[2], reason);
	}
}

void testMe(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onMessage requires 3 arguments");
	else
		p->onMe(s, argv[0], argv[1], argv[2]);
}

void testMessage(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onMessage requires 3 arguments");
	else
		p->onMessage(s, argv[0], argv[1], argv[2]);
}

void testMode(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	std::string modeArg;

	if (argc < 3)
		Logger::warn("test: onMode requires at least 3 arguments");
	else {
		if (argc >= 4)
			modeArg = argv[3];

		p->onMode(s, argv[0], argv[1], argv[2], modeArg);
	}
}

void testNick(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 2)
		Logger::warn("test: onNick requires 2 arguments");
	else
		p->onNick(s, argv[0], argv[1]);
}

void testNotice(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onNotice requires 3 arguments");
	else
		p->onNotice(s, argv[0], argv[1], argv[2]);
}

void testPart(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	std::string reason;

	if (argc < 3)
		Logger::warn("test: onPart requires at least 2 argument");
	else {
		if (argc >= 3)
			reason = argv[2];

		p->onPart(s, argv[0], argv[1], reason);
	}
}

void testQuery(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 2)
		Logger::warn("test: onQuery requires 2 arguments");
	else
		p->onQuery(s, argv[0], argv[1]);
}

void testTopic(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 3)
		Logger::warn("test: onTopic requires 3 arguments");
	else
		p->onTopic(s, argv[0], argv[1], argv[2]);
}

void testUserMode(std::shared_ptr<Plugin> p, std::shared_ptr<Server> s, int argc, char **argv)
{
	if (argc < 2)
		Logger::warn("test: onUserMode requires 2 arguments");
	else
		p->onUserMode(s, argv[0], argv[1]);
}

std::unordered_map<std::string, TestFunction> testCommands {
	{ "onCommand",		testCommand		},
	{ "onConnect",		testConnect		},
	{ "onChannelNotice",	testChannelNotice	},
	{ "onInvite",		testInvite		},
	{ "onJoin",		testJoin		},
	{ "onKick",		testKick		},
	{ "onMe",		testMe			},
	{ "onMessage",		testMessage		},
	{ "onMode",		testMode		},
	{ "onNick",		testNick		},
	{ "onNotice",		testNotice		},
	{ "onPart",		testPart		},
	{ "onQuery",		testQuery		},
	{ "onTopic",		testTopic		},
	{ "onUserMode",		testUserMode		}
};

// }}}

void testPlugin(const char *file, int argc, char **argv)
{
	Server::Info info;
	Server::Identity ident;

	info.name = "local";
	info.host = "local";
	info.port = 6667;

	auto server = std::make_shared<FakeServer>(info, ident);

	if (strcmp(argv[0], "help") == 0) {
		if (argc > 1) {
			try {
				helpCommands.at(argv[1])();
				exit(0);
			} catch (std::out_of_range ex) {
				Logger::fatal(1, "There is no subject named %s", argv[1]);
			}
		} else
			Logger::fatal(1, "test: help requires 1 argument");
	}

	// Always push before calling it
	auto name = Util::baseName(std::string(file));
	auto epos = name.find(".lua");
	if (epos != std::string::npos)
		name = name.erase(epos);

	auto plugin = std::make_shared<Plugin>(name, file);

	try {
		plugin->open();
	} catch (Plugin::ErrorException ex) {
		Logger::fatal(1, "Failed to open plugin: %s", ex.what());
	}

	// Simulate handler is optional
	if (argc > 1) {
		try {
			testCommands.at(argv[1])(plugin, server, argc - 2, argv + 2);
		} catch (std::out_of_range ex) {
			Logger::fatal(1, "Unknown test command named %s", argv[1]);
		} catch (Plugin::ErrorException ex) {
			Logger::fatal(1, "Error in script %s", ex.what());
		}
	}
}

}

void test(int argc, char **argv)
{
	Logger::setVerbose(true);

	if (argc < 2) {
		Logger::warn("usage: %s test plugin.lua [command] [parameters...]", getprogname());
		Logger::warn("       %s test help <command>", getprogname());

		Logger::warn("Commands supported:");
		Logger::warn("\tonCommand\t\tDo a fake special command");
		Logger::warn("\tonConnect\t\tSimulate a connection");
		Logger::warn("\tonChannelNotice\t\tTest a public notice");
		Logger::warn("\tonInvite\t\tInvite someone to a channel");
		Logger::warn("\tonJoin\t\t\tJoin a channel");
		Logger::warn("\tonKick\t\t\tKick someone from a channel");
		Logger::warn("\tonMe\t\t\tSend a CTCP Action (same as /me)");
		Logger::warn("\tonMessage\t\tSend a message to someone or a channel");
		Logger::warn("\tonMode\t\t\tTest a public channel change");
		Logger::warn("\tonNick\t\t\tChange your nickname");
		Logger::warn("\tonPart\t\t\tLeave a channel");
		Logger::warn("\tonQuery\t\t\tSend a private message");
		Logger::warn("\tonTopic\t\t\tTest a topic channel change");
		Logger::fatal(1, "\tonUserMode\t\tTest a user mode change");
	}

	testPlugin(argv[1], argc - 1, argv + 1);
	std::exit(0);
}

} // !irccd
