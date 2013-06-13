/*
 * Test.cpp -- test plugins
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#include <functional>
#include <map>
#include <string>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Plugin.h"
#include "Server.h"
#include "Test.h"

using namespace irccd;
using namespace std;

typedef function<void(void)>					HelpFunction;
typedef function<void(Plugin *, Server *, int, char **)>	TestFunction;

class FakeServer : public Server {
public:
	void cnotice(const string &channel, const string &message)
	{
		Logger::log("[test] notice: (%s) %s", channel.c_str(), message.c_str());
	}

	void invite(const string &target, const string &channel)
	{
		Logger::log("[test] invite: %s invited to channel %s",
		    target.c_str(), channel.c_str());
	}

	void join(const string &name, const string &password)
	{
		Logger::log("[test] join: joining channel %s with password \"%s\"",
		    name.c_str(), password.c_str());
	}

	void kick(const string &name, const string &channel, const string &reason)
	{
		Logger::log("[test] kick: kicking %s from channel %s reason \"%s\"",
		    name.c_str(), channel.c_str(), reason.c_str());
	}

	void me(const string &target, const string &message)
	{
		Logger::log("[test] me: * %s: %s", target.c_str(), message.c_str());
	}

	void mode(const string &channel, const string &mode)
	{
		Logger::log("[test] mode: %s mode %s", channel.c_str(), mode.c_str());
	}

	void names(const std::string &channel)
	{
		Logger::log("[test] names: gettings names from %s", channel.c_str());
	}

	void nick(const string &nick)
	{
		Logger::log("[test] nick: changing nick to %s", nick.c_str());
	}

	void notice(const string &nickname, const string &message)
	{
		Logger::log("[test] notice: from %s: %s", nickname.c_str(), message.c_str());
	}

	void part(const string &channel)
	{
		Logger::log("[test] part: leaving channel %s", channel.c_str());
	}

	void query(const string &who, const string &message)
	{
		Logger::log("[test] query: private message from %s: %s",
		    who.c_str(), message.c_str());
	}

	void say(const string &target, const string &message)
	{
		Logger::log("[test] say: said %s to %s", message.c_str(),
		    target.c_str());
	}

	void topic(const string &channel, const string &topic)
	{
		Logger::log("[test] topic: changing %s topic to %s",
		    channel.c_str(), topic.c_str());
	}
};

// {{{ Help functions

static void helpConnect(void)
{
	Logger::warn("usage: %s test file onConnect\n", getprogname());
	Logger::warn("Do a fake successful connection.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onConnect");
}

static void helpChannelNotice(void)
{
	Logger::warn("usage: %s test file onChannelNotice nick target notice\n", getprogname());
	Logger::warn("Send a private notice to the specified target. Nick parameter");
	Logger::warn("is the one who sends the notice.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNotice mick #staff \"#staff is not #offtopic\"", getprogname());
}

static void helpInvite(void)
{
	Logger::warn("usage: %s test file onInvite channel who\n", getprogname());
	Logger::warn("Do a fake inviation from who to a specific channel.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onInvite #staff john", getprogname());
}

static void helpJoin(void)
{
	Logger::warn("usage: %s test file onJoin channel who\n", getprogname());
	Logger::warn("Join the channel. The parameter who is the person");
	Logger::warn("nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onJoin #staff francis", getprogname());
}

static void helpKick(void)
{
	Logger::warn("usage: %s test file onKick channel who kicked reason\n", getprogname());
	Logger::warn("Fake a kick from a specific channel, the reason may be empty.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onKick #staff markand julia");
	Logger::warn("\t%s test file onKick #staff francis markand \"You're not nice with her\"");
}

static void helpMessage(void)
{
	Logger::warn("usage: %s test file onMessage channel who message\n", getprogname());
	Logger::warn("Send a message to the specific channel.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onMessage #staff francis \"Hello All\"", getprogname());
}

static void helpMode(void)
{
	Logger::warn("usage: %s test file onMode channel who mode [modeArg]\n", getprogname());
	Logger::warn("Do a fake channel mode change. The who parameter is the one who");
	Logger::warn("channel mode. An optional mode argument can be specified.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onMode #staff john +t", getprogname());
	Logger::warn("\t%s test file onMode #staff john +k #overflow", getprogname());
}

static void helpNick(void)
{
	Logger::warn("usage: %s test file onNick oldnick newnick\n", getprogname());
	Logger::warn("Do a fake nick change.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNick john_ john");
}

static void helpNotice(void)
{
	Logger::warn("usage: %s test file onNotice who target notice\n", getprogname());
	Logger::warn("Send a private notice to the target nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onNotice john mick \"Please stop flooding\"", getprogname());
}

static void helpPart(void)
{
	Logger::warn("usage: %s test file onPart channel who reason\n", getprogname());
	Logger::warn("Simulate a target departure specified by foo on the channel. The");
	Logger::warn("reason may be empty.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onPart #staff john \"Do not like that channel\"", getprogname());
}

static void helpQuery(void)
{
	Logger::warn("usage: %s test file onQuery who message\n", getprogname());
	Logger::warn("Simulate a private query, who is the sender.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onQuery john \"Do you want some?\"", getprogname());
}

static void helpTopic(void)
{
	Logger::warn("usage: %s test file onTopic channel who topic\n", getprogname());
	Logger::warn("Change the topic on a fake server. Topic may be empty so that");
	Logger::warn("clear the old one.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onTopic #staff markand \"I'm your new god little girls\"", getprogname());
}

static void helpUserMode(void)
{
	Logger::warn("usage: %s test file onUserMode who mode\n", getprogname());
	Logger::warn("Fake a user mode change, remember that who is the one that changed");
	Logger::warn("your mode, so you may check the `server' Lua API if you want your");
	Logger::warn("own nickname.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s test file onUserMode john +i");
}

static map<string, HelpFunction> createHelpCommands(void)
{
	map<string, HelpFunction> commands;

	commands["onConnect"]		= helpConnect;
	commands["onChannelNotice"]	= helpChannelNotice;
	commands["onInvite"]		= helpInvite;
	commands["onJoin"]		= helpJoin;
	commands["onKick"]		= helpKick;
	commands["onMessage"]		= helpMessage;
	commands["onMode"]		= helpMode;
	commands["onNick"]		= helpNick;
	commands["onNotice"]		= helpNotice;
	commands["onPart"]		= helpPart;
	commands["onQuery"]		= helpQuery;
	commands["onTopic"]		= helpTopic;
	commands["onUserMode"]		= helpUserMode;

	return commands;
}

static map<string, HelpFunction> helpCommands = createHelpCommands();

// }}}

// {{{ Test functions
#if 0

static void testConnect(Plugin *p, Server *s, int argc, char **argv)
{
	p->onConnect(s);

	(void)argc;
	(void)argv;
}

static void testChannelNotice(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 3) {
		Logger::warn("[test] onChannelNotice requires 2 arguments");
	} else {
		p->onChannelNotice(s, argv[0], argv[1], argv[2]);
	}
}

static void testInvite(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 2) {
		Logger::warn("[test] onInvite requires 2 arguments");
	} else {
		p->onInvite(s, argv[0], argv[1]);
	}
}

static void testJoin(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 2) {
		Logger::warn("[test] onJoin requires 2 arguments");
	} else {
		p->onJoin(s, argv[0], argv[1]);
	}
}

static void testKick(Plugin *p, Server *s, int argc, char **argv)
{
	string reason;

	if (argc < 3) {
		Logger::warn("[test] onKick requires at least 3 arguments");
	} else {
		if (argc > 4)
			reason = argv[3];

		p->onKick(s, argv[0], argv[1], argv[2], reason);
	}
}

static void testMessage(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 3) {
		Logger::warn("[test] onMessage requires 3 arguments");
	} else {
		p->onMessage(s, argv[0], argv[1], argv[2]);
	}
}

static void testMode(Plugin *p, Server *s, int argc, char **argv)
{
	string modeArg;

	if (argc < 3) {
		Logger::warn("[test] onMode requires at least 3 arguments");
	} else {
		if (argc >= 4)
			modeArg = argv[3];

		p->onMode(s, argv[0], argv[1], argv[2], modeArg);
	}
}

static void testNick(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 2) {
		Logger::warn("[test] onNick requires 2 arguments");
	} else {
		p->onNick(s, argv[0], argv[1]);
	}
}

static void testNotice(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 3) {
		Logger::warn("[test] onNotice requires 3 arguments");
	} else {
		p->onNotice(s, argv[0], argv[1], argv[2]);
	}
}

static void testPart(Plugin *p, Server *s, int argc, char **argv)
{
	string reason;

	if (argc < 3) {
		Logger::warn("[test] onPart requires at least 2 argument");
	} else {
		if (argc >= 3)
			reason = argv[2];

		p->onPart(s, argv[0], argv[1], reason);
	}
}

static void testQuery(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 2) {
		Logger::warn("[test] onQuery requires 2 arguments");
	} else {
		p->onQuery(s, argv[0], argv[1]);
	}
}

static void testTopic(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 3) {
		Logger::warn("[test] onTopic requires 3 arguments");
	} else {
		p->onTopic(s, argv[0], argv[1], argv[2]);
	}
}

static void testUserMode(Plugin *p, Server *s, int argc, char **argv)
{
	if (argc < 2) {
		Logger::warn("[test] onUserMode requires 2 arguments");
	} else {
		p->onUserMode(s, argv[0], argv[1]);
	}
}
#endif

static map<string, TestFunction> createCommands(void)
{
	map<string, TestFunction> commands;
#if 0
	commands["onConnect"]		= testConnect;
	commands["onChannelNotice"]	= testChannelNotice;
	commands["onInvite"]		= testInvite;
	commands["onJoin"]		= testJoin;
	commands["onKick"]		= testKick;
	commands["onMessage"]		= testMessage;
	commands["onMode"]		= testMode;
	commands["onNick"]		= testNick;
	commands["onNotice"]		= testNotice;
	commands["onPart"]		= testPart;
	commands["onQuery"]		= testQuery;
	commands["onTopic"]		= testTopic;
	commands["onUserMode"]		= testUserMode;
#endif

	return commands;
}

static map<string, TestFunction> testCommands = createCommands();

// }}}

static void testPlugin(const char *file, int argc, char **argv)
{
	FakeServer server;
	Identity ident;

	if (strcmp(argv[0], "help") == 0) {
		if (argc > 1) {
			try {
				helpCommands.at(argv[1])();
				exit(0);
			} catch (out_of_range ex) {
				Logger::warn("There is no subject named %s", argv[1]);
				exit(1);
			}
		} else {
			Logger::warn("[test] help requires 1 argument");
			exit(1);
		}
	}

	server.setConnection("local", "local", 6667);

	// Always push before calling it
	string name = Util::basename(string(file));
	size_t epos = name.find(".lua");
	if (epos != string::npos)
		name = name.erase(epos);

	Irccd::getInstance()->getPlugins().push_back(shared_ptr<Plugin>(new Plugin(name)));

	shared_ptr<Plugin> plugin = Irccd::getInstance()->getPlugins().back();
	if (!plugin->open(file)) {
		Logger::warn("Failed to open plugin: %s", plugin->getError().c_str());
	}

	// Simulate handler is optional
#if 0
	if (argc > 1) {
		try {
			testCommands.at(argv[1])(&plugin, &server, argc - 2, argv + 2);
		} catch (out_of_range ex) {
			Logger::warn("Unknown test command named %s", argv[1]);
		}
	}
#endif
}

void irccd::test(int argc, char **argv)
{
	Logger::setVerbose(true);

	if (argc < 2) {
		Logger::warn("usage: %s test plugin.lua [command] [parameters...]", getprogname());

		Logger::warn("Commands supported:");
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
		Logger::warn("\tonTopic\t\t\tTest a topic channel change");
		Logger::warn("\tonUserMode\t\tTest a user mode change");
	} else {
		testPlugin(argv[1], argc - 1, argv + 1);
	}

	exit(1);
}
