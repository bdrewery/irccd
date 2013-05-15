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
using namespace parser;
using namespace std;

/* {{{ help messages */

typedef function<void(void)> HelpHandler;

static void helpJoin(void)
{
	Logger::warn("usage: %s join server channel [password]\n", getprogname());
	Logger::warn("Join a channel on a specific server registered in irccd. The server");
	Logger::warn(" is referenced by the parameter server. Parameter channel is the channel");
	Logger::warn(" to join. An optional password may be set as password parameter.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s join freenode #staff", getprogname());
}

static void helpKick(void)
{
	Logger::warn("usage: %s kick server nick channel [reason]\n", getprogname());
	Logger::warn("Kick someone from a channel. The parameter reason is optional and");
	Logger::warn(" may be ommited but when specified it must be unclosed between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s kick freenode jean #staff \"Stop flooding\"", getprogname());
}

static void helpMe(void)
{
	Logger::warn("usage: %s me server target message\n", getprogname());
	Logger::warn("Send a CTCP ACTION message. It is exactly the same syntax as %s message.\n");

	Logger::warn("Example:");
	Logger::warn("\t%sme freenode #staff \"going back soon\"", getprogname());
}

static void helpMessage(void)
{
	Logger::warn("usage: %s message server target message\n", getprogname());
	Logger::warn("Send a message to someone or a channel. The server parameter is one registered");
	Logger::warn(" in irccd config. The target may be a channel or a real person");
	Logger::warn(" a real person. If the message contains more than one word it must be enclosed between");
	Logger::warn(" between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s message freenode #staff \"Hello from irccd\"", getprogname());
}

static void helpNick(void)
{
	Logger::warn("usage: %s nick server nickname\n", getprogname());
	Logger::warn("Change your nickname. The parameter nickname is the new nickname\n");

	Logger::warn("Example:");
	Logger::warn("\t%snick freenode david", getprogname());
}

static void helpPart(void)
{
	Logger::warn("usage: %s part server channel\n", getprogname());
	Logger::warn("Leave a channel. Parameter server is one registered in irccd config.");
	Logger::warn("Parameter channel is the channel to leave.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s part freenode #staff", getprogname());
}

static map<string, HelpHandler> createHelpHandlers(void)
{
	map<string, HelpHandler> helpHandlers;

	helpHandlers["join"] = helpJoin;
	helpHandlers["kick"] = helpKick;
	helpHandlers["me"] = helpMe;
	helpHandlers["message"] = helpMessage;
	helpHandlers["nick"] = helpNick;
	helpHandlers["part"] = helpPart;

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

static void handleJoin(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2)
		Logger::warn("join requires 2 arguments");
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

static map<string, Handler> createHandlers(void)
{
	map<string, Handler> handlers;

	handlers["help"] = handleHelp;
	handlers["join"] = handleJoin;
	handlers["kick"] = handleKick;
	handlers["me"] = handleMe;
	handlers["message"] = handleMessage;
	handlers["nick"] = handleNick;
	handlers["part"] = handlePart;

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

	section.getOption<string>("path", path);

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

	section.getOption<string>("host", host);
	section.getOption<int>("port", port);
	section.getOption<string>("family", inet);

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

		sectionSocket.getOption<string>("type", type);

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
	Logger::warn("\tjoin\t\tJoin a channel");
	Logger::warn("\tmessage\t\tSend a message to someone or a channel");
	Logger::warn("\tpart\t\tLeave a channel");

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
