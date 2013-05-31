/*
 * Irccdctl.cpp -- irccd controller class
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

#include <Logger.h>
#include <Parser.h>
#include <Util.h>

#include "Irccdctl.h"

using namespace irccd;
using namespace std;

/* {{{ help messages */

typedef function<void(void)> HelpHandler;

static void helpChannelNotice()
{
	Logger::warn("usage: %s cnotice server channel message\n", getprogname());
	Logger::warn("Send a notice to a public channel. This is a notice that everyone");
	Logger::warn("will be notified by.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s cnotice freenode #staff \"Don't flood\"", getprogname());
}

static void helpInvite()
{
	Logger::warn("usage: %s invite server nickname channel\n", getprogname());
	Logger::warn("Invite someone to a channel, needed for channel with mode +i\n");

	Logger::warn("Example:");
	Logger::warn("\t%s invite freenode xorg62 #staff", getprogname());
}

static void helpJoin()
{
	Logger::warn("usage: %s join server channel [password]\n", getprogname());
	Logger::warn("Join a channel on a specific server registered in irccd. The server");
	Logger::warn("is referenced by the parameter server. Parameter channel is the channel");
	Logger::warn("to join. An optional password may be set as password parameter.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s join freenode #staff", getprogname());
}

static void helpKick()
{
	Logger::warn("usage: %s kick server nick channel [reason]\n", getprogname());
	Logger::warn("Kick someone from a channel. The parameter reason is optional and");
	Logger::warn("may be ommited but when specified it must be unclosed between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s kick freenode jean #staff \"Stop flooding\"", getprogname());
}

static void helpLoad()
{
	Logger::warn("usage: %s load name\n", getprogname());
	Logger::warn("Load a plugin into the irccd instance.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s load logger");
}

static void helpMe()
{
	Logger::warn("usage: %s me server target message\n", getprogname());
	Logger::warn("Send a CTCP ACTION message. It is exactly the same syntax as %s message.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s me freenode #staff \"going back soon\"", getprogname());
}

static void helpMessage()
{
	Logger::warn("usage: %s message server target message\n", getprogname());
	Logger::warn("Send a message to someone or a channel. The server parameter is one registered");
	Logger::warn("in irccd config. The target may be a channel or a real person");
	Logger::warn("a real person. If the message contains more than one word it must be enclosed between");
	Logger::warn("between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s message freenode #staff \"Hello from irccd\"", getprogname());
}

static void helpMode()
{
	Logger::warn("usage: %s mode server channel mode\n", getprogname());
	Logger::warn("Change the mode of the specified channel. The mode contains full parameters");
	Logger::warn("like \"+b\" or \"+k secret\".\n");

	Logger::warn("Example:");
	Logger::warn("\t%s mode freenode #staff +t");
}

static void helpNick()
{
	Logger::warn("usage: %s nick server nickname\n", getprogname());
	Logger::warn("Change your nickname. The parameter nickname is the new nickname\n");

	Logger::warn("Example:");
	Logger::warn("\t%snick freenode david", getprogname());
}

static void helpNotice()
{
	Logger::warn("usage: %s notice server target message\n", getprogname());
	Logger::warn("Send a private notice to a target user.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s notice freenode jean \"Private notice\"", getprogname());
}

static void helpPart()
{
	Logger::warn("usage: %s part server channel\n", getprogname());
	Logger::warn("Leave a channel. Parameter server is one registered in irccd config.");
	Logger::warn("Parameter channel is the channel to leave.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s part freenode #staff", getprogname());
}

static void helpReload()
{
	Logger::warn("usage: %s reload name\n", getprogname());
	Logger::warn("Reload a plugin, parameter name is the plugin to reload.");
	Logger::warn("The plugin needs to be loaded.\n");

	Logger::warn("Example:");
	Logger::warn("\t %s reload logger", getprogname());
}

static void helpTopic()
{
	Logger::warn("usage: %s topic server channel topic\n", getprogname());
	Logger::warn("Set the new topic of a channel. Topic must be enclosed between");
	Logger::warn("quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s topic freenode #wmfs \"This is the best channel\"", getprogname());
}

static void helpUnload()
{
	Logger::warn("usage: %s unload name\n", getprogname());
	Logger::warn("Unload a loaded plugin from the irccd instance.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s unload logger");
}

static void helpUserMode()
{
	Logger::warn("usage: %s mode server mode\n", getprogname());
	Logger::warn("Change your own user mode.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s mode freenode +i");
}

static map<string, HelpHandler> createHelpHandlers()
{
	map<string, HelpHandler> helpHandlers;

	helpHandlers["cnotice"]	= helpChannelNotice;
	helpHandlers["invite"]	= helpInvite;
	helpHandlers["join"]	= helpJoin;
	helpHandlers["kick"]	= helpKick;
	helpHandlers["load"]	= helpLoad;
	helpHandlers["me"]	= helpMe;
	helpHandlers["message"]	= helpMessage;
	helpHandlers["mode"]	= helpMode;
	helpHandlers["notice"]	= helpNotice;
	helpHandlers["nick"]	= helpNick;
	helpHandlers["part"]	= helpPart;
	helpHandlers["reload"]	= helpReload;
	helpHandlers["topic"]	= helpTopic;
	helpHandlers["umode"]	= helpUserMode;
	helpHandlers["unload"]	= helpUnload;

	return helpHandlers;
}

static map<string, HelpHandler> helpHandlers = createHelpHandlers();

/* }}} */

/* {{{ handlers */

typedef function<void(Irccdctl *, int, char **)> Handler;

static void handleHelp(Irccdctl *ctl, int argc, char **argv)
{
	(void)ctl;

	if (argc < 1) {
		Logger::warn("help requires 1 argument");
		exit(1);
	}

	try {
		helpHandlers.at(argv[0])();
	} catch (out_of_range ex) {
		Logger::warn("There is no subject named %s", argv[0]);
	}

	exit(1);
}

static void handleChannelNotice(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("cnotice requires 3 arguments");
	else {
		oss << "CNOTICE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleInvite(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("invite requires 3 arguments");
	else {
		oss << "INVITE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";

		ctl->sendRaw(oss.str());
	}
}

static void handleJoin(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2)
		Logger::warn("join requires at least 2 arguments");
	else {
		oss << "JOIN " << argv[0] << " " << argv[1];

		// optional password
		if (argc >= 3)
			oss << " " << argv[2];
		oss << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleKick(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("kick requires at least 3 arguments ");
	else {
		oss << "KICK " << argv[0] << " " << argv[1] << " " << argv[2];

		// optional reason
		if (argc >= 4)
			oss << " " << argv[3];
		oss << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleLoad(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 1)
		Logger::warn("load requires 1 argument");
	else {
		oss << "LOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMe(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("me requires 3 arguments");
	else {
		oss << "ME " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMessage(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("message requires 3 arguments");
	else {
		oss << "MSG " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMode(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("mode requires 3 arguments");
	else {
		oss << "MODE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleNick(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2)
		Logger::warn("nick requires 2 arguments");
	else {
		oss << "NICK " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleNotice(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("notice requires 3 arguments");
	else {
		oss << "NOTICE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handlePart(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2)
		Logger::warn("part requires 2 arguments");
	else {
		oss << "PART " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleReload(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 1)
		Logger::warn("reload requires 1 argument");
	else {
		oss << "RELOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleTopic(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3)
		Logger::warn("topic requires 3 arguments");
	else {
		oss << "TOPIC " << argv[0] << " " << argv[1] << " ";
		oss << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleUnload(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 1)
		Logger::warn("unload requires 1 argument");
	else {
		oss << "UNLOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleUserMode(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2)
		Logger::warn("umode requires 2 arguments");
	else {
		oss << "UMODE " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static map<string, Handler> createHandlers(void)
{
	map<string, Handler> handlers;

	handlers["cnotice"]	= handleChannelNotice;
	handlers["help"]	= handleHelp;
	handlers["invite"]	= handleInvite;
	handlers["join"]	= handleJoin;
	handlers["kick"]	= handleKick;
	handlers["load"]	= handleLoad;
	handlers["me"]		= handleMe;
	handlers["message"]	= handleMessage;
	handlers["mode"]	= handleMode;
	handlers["nick"]	= handleNick;
	handlers["notice"]	= handleNotice;
	handlers["part"]	= handlePart;
	handlers["reload"]	= handleReload;
	handlers["topic"]	= handleTopic;
	handlers["umode"]	= handleUserMode;
	handlers["unload"]	= handleUnload;

	return handlers;
}

static map<string, Handler> handlers = createHandlers();

/* }}} */

Irccdctl::Irccdctl(void)
{
}

Irccdctl::~Irccdctl(void)
{
}

void Irccdctl::connectUnix(const Section &section)
{
	string path;

	path = section.requireOption<string>("path");

	try {
		m_socket = SocketUtil::connectUnix(path);
	} catch (SocketUtil::ErrorException error) {
		Logger::warn("Failed to connect to %s: %s", path.c_str(), error.what());
		exit(1);
	}
}

void Irccdctl::connectInet(const Section &section)
{
	string host, inet;
	int port, family = 0;

	host = section.requireOption<string>("host");
	port = section.requireOption<int>("port");
	inet = section.requireOption<string>("family");

	if (inet == "ipv4")
		family = Socket::Inet4;
	else if (inet == "ipv6")
		family = Socket::Inet6;
	else {
		Logger::warn("parameter family is one of them: ipv4, ipv6");
		exit(1);
	}

	try {
		m_socket = SocketUtil::connectInet(host, port, family);
	} catch (SocketUtil::ErrorException error) {
		Logger::warn("Failed to connect: %s", error.what());
		exit(1);
	}
}

void Irccdctl::readConfig(void)
{
	Parser config(m_configPath);

	if (!config.open()) {
		Logger::warn("Failed to open %s", m_configPath.c_str());
		exit(1);
	}

	try {
		const Section &sectionSocket = config.getSection("socket");
		string type;

		type = sectionSocket.requireOption<string>("type");

		if (type.compare("unix") == 0)
			connectUnix(sectionSocket);
		else if (type.compare("inet") == 0)
			connectInet(sectionSocket);
	} catch (NotFoundException ex) {
		Logger::warn("Config misses %s", ex.which().c_str());
	}
}

void Irccdctl::openConfig(void)
{
	if (m_configPath.length() == 0) {
		m_configPath = Util::configFilePath("irccdctl.conf");
		readConfig();
	} else
		readConfig();
}

void Irccdctl::sendRaw(const std::string &message)
{
	try {
		m_socket->send(message.c_str(), message.length());
	} catch (Socket::Exception ex) {
		Logger::warn("Failed to send message: %s", ex.what());
	}
}

void Irccdctl::usage(void)
{
	Logger::warn("usage: %s [-cv] <commands> [<args>]\n", getprogname());

	Logger::warn("Commands supported:");
	Logger::warn("\thelp\t\tGet this help");
	Logger::warn("\tinvite\t\tInvite someone to a channel");
	Logger::warn("\tjoin\t\tJoin a channel");
	Logger::warn("\tkick\t\tKick someone from a channel");
	Logger::warn("\tme\t\tSend a CTCP Action (same as /me)");
	Logger::warn("\tmessage\t\tSend a message to someone or a channel");
	Logger::warn("\tmode\t\tChange a channel mode");
	Logger::warn("\tnick\t\tChange your nickname");
	Logger::warn("\tpart\t\tLeave a channel");
	Logger::warn("\ttopic\t\tChange a channel topic");
	Logger::warn("\tumode\t\tChange a user mode");

	Logger::warn("\nFor more information on a command, type %s help <command>", getprogname());

	exit(1);
}

void Irccdctl::setConfigPath(const std::string &path)
{
	m_configPath = path;
}

void Irccdctl::setVerbosity(bool verbose)
{
	Logger::setVerbose(verbose);
}

int Irccdctl::run(int argc, char **argv)
{
	if (argc < 1)
		usage();
		/* NOTREACHED */

	// exceptional, do not open for "help" subject
	if (strcmp(argv[0], "help") != 0)
		openConfig();

	try {
		handlers.at(string(argv[0]))(this, --argc, ++argv);
	} catch (out_of_range ex) {
		Logger::warn("Unknown command %s", argv[0]);
		return 1;
	}
	
	return 0;
}
