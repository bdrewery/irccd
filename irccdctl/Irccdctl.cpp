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
#include <sstream>

#include <Logger.h>
#include <Parser.h>
#include <SocketAddress.h>
#include <SocketListener.h>
#include <Util.h>

#include "Irccdctl.h"

using namespace irccd;
using namespace std;

/* {{{ help messages */

typedef function<void()> HelpHandler;

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
	Logger::warn("\t%s load logger", getprogname());
}

static void helpMe()
{
	Logger::warn("usage: %s me server target message\n", getprogname());
	Logger::warn("Send a CTCP ACTION message. It is exactly the same syntax as message.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s me freenode #staff \"going back soon\"", getprogname());
}

static void helpMessage()
{
	Logger::warn("usage: %s message server target message\n", getprogname());
	Logger::warn("Send a message to someone or a channel. The target may be a channel or a real person");
	Logger::warn("If the message contains more than one word it must be enclosed between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s message freenode #staff \"Hello from irccd\"", getprogname());
}

static void helpMode()
{
	Logger::warn("usage: %s mode server channel mode\n", getprogname());
	Logger::warn("Change the mode of the specified channel. The mode contains full parameters");
	Logger::warn("like \"+b\" or \"+k secret\".\n");

	Logger::warn("Example:");
	Logger::warn("\t%s mode freenode #staff +t", getprogname());
}

static void helpNick()
{
	Logger::warn("usage: %s nick server nickname\n", getprogname());
	Logger::warn("Change your nickname. The parameter nickname is the new nickname\n");

	Logger::warn("Example:");
	Logger::warn("\t%s nick freenode david", getprogname());
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
	Logger::warn("\t%s unload logger", getprogname());
}

static void helpUserMode()
{
	Logger::warn("usage: %s umode server mode\n", getprogname());
	Logger::warn("Change your own user mode.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s umode +i", getprogname());
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

static void handleHelp(Irccdctl *, int argc, char **argv)
{
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

	if (argc < 3) {
		Logger::warn("cnotice requires 3 arguments");
		exit(1);
	} else {
		oss << "CNOTICE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleInvite(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("invite requires 3 arguments");
		exit(1);
	} else {
		oss << "INVITE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";

		ctl->sendRaw(oss.str());
	}
}

static void handleJoin(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2) {
		Logger::warn("join requires at least 2 arguments");
		exit(1);
	} else {
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

	if (argc < 3) {
		Logger::warn("kick requires at least 3 arguments ");
		exit(1);
	} else {
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

	if (argc < 1) {
		Logger::warn("load requires 1 argument");
		exit(1);
	} else {
		oss << "LOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMe(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("me requires 3 arguments");
		exit(1);
	} else {
		oss << "ME " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMessage(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("message requires 3 arguments");
		exit(1);
	} else {
		oss << "MSG " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleMode(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("mode requires 3 arguments");
		exit(1);
	} else {
		oss << "MODE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleNick(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2) {
		Logger::warn("nick requires 2 arguments");
		exit(1);
	} else {
		oss << "NICK " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleNotice(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("notice requires 3 arguments");
		exit(1);
	} else {
		oss << "NOTICE " << argv[0] << " " << argv[1];
		oss << " " << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handlePart(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2) {
		Logger::warn("part requires 2 arguments");
		exit(1);
	} else {
		oss << "PART " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleReload(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 1) {
		Logger::warn("reload requires 1 argument");
		exit(1);
	} else {
		oss << "RELOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleTopic(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 3) {
		Logger::warn("topic requires 3 arguments");
		exit(1);
	} else {
		oss << "TOPIC " << argv[0] << " " << argv[1] << " ";
		oss << argv[2] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleUnload(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 1) {
		Logger::warn("unload requires 1 argument");
		exit(1);
	} else {
		oss << "UNLOAD " << argv[0] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static void handleUserMode(Irccdctl *ctl, int argc, char **argv)
{
	ostringstream oss;

	if (argc < 2) {
		Logger::warn("umode requires 2 arguments");
		exit(1);
	} else {
		oss << "UMODE " << argv[0] << " " << argv[1] << "\n";
		ctl->sendRaw(oss.str());
	}
}

static map<string, Handler> createHandlers()
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

Irccdctl::Irccdctl()
{
	Socket::init();

	Logger::setVerbose(false);
}

Irccdctl::~Irccdctl()
{
	Socket::finish();
}

#if !defined(_WIN32)
void Irccdctl::connectUnix(const Section &section)
{
	string path;

	path = section.requireOption<string>("path");

	try {
		m_socket = Socket(AF_UNIX, SOCK_STREAM, 0);
		m_socket.connect(AddressUnix(path));
	} catch (SocketError error) {
		Logger::warn("irccd: failed to connect to %s: %s", path.c_str(), error.what());
		exit(1);
	}
}
#endif

void Irccdctl::connectInet(const Section &section)
{
	string host, inet;
	int port, family = 0;

	host = section.requireOption<string>("host");
	port = section.requireOption<int>("port");
	inet = section.requireOption<string>("family");

	if (inet == "ipv4")
		family = AF_INET;
	else if (inet == "ipv6")
		family = AF_INET6;
	else {
		Logger::warn("socket: parameter family is one of them: ipv4, ipv6");
		exit(1);
	}

	try {
		m_socket = Socket(family, SOCK_STREAM, 0);
		m_socket.connect(ConnectAddressIP(host, port, family));
	} catch (SocketError error) {
		Logger::warn("irccdctl: failed to connect: %s", error.what());
		exit(1);
	}
}

void Irccdctl::readConfig(Parser &config)
{
	try {
		const Section &sectionSocket = config.getSection("socket");
		string type;

		type = sectionSocket.requireOption<string>("type");

		if (type == "unix") {
#if !defined(_WIN32)
			connectUnix(sectionSocket);
#else
			Logger::warn("socket: unix sockets are not supported on Windows");
#endif
		} else if (type == "internet") {
			connectInet(sectionSocket);
		} else {
			Logger::warn("socket: invalid socket type %s", type.c_str());
			exit(1);
		}
	} catch (NotFoundException ex) {
		Logger::warn("socket: missing parameter %s", ex.which().c_str());
		exit(1);
	}
}

void Irccdctl::openConfig()
{
	Parser config;
	vector<string> tried;

	if (m_configPath.length() == 0) {
		bool found;

		found = Util::findConfig("irccdctl.conf", [&] (const string &path) -> bool {
			config = Parser(path);

			// Keep track of loaded files
			if (!config.open()) {
				tried.push_back(path);
				return false;
			}

			m_configPath = path;

			return (found = true);
		});

		if (!found) {
			Logger::warn("irccdctl: no configuration could be found, exiting");

			for (auto p : tried)
				Logger::warn("irccdctl: tried %s", p.c_str());

			exit(1);
		}
	} else {
		config = Parser(m_configPath);

		if (!config.open()) {
			Logger::warn("irccdctl: could not open %s, exiting", m_configPath.c_str());
			exit(1);
		}
	}

	readConfig(config);
}

void Irccdctl::sendRaw(const std::string &message)
{
	try {
		m_socket.send(message.c_str(), message.length());
	} catch (SocketError ex) {
		Logger::warn("irccdctl: failed to send message: %s", ex.what());
	}
}

int Irccdctl::getResponse()
{
	SocketListener listener;
	ostringstream oss;
	bool finished = false;
	int ret = 0;

	listener.add(m_socket);
	try {
		while (!finished) {
			char data[128];
			unsigned nbread;
			size_t pos;

			listener.select(30);
			nbread = m_socket.recv(data, sizeof (data) - 1);
			if (nbread == 0) {
				finished = true;
			} else {
				string result;

				data[nbread] = '\0';
				oss << data;

				pos = oss.str().find_first_of('\n');
				if (pos == string::npos)
					continue;

				result = oss.str().substr(0, pos);
				if (result != "OK") {
					Logger::warn("irccdctl: error, server said: %s", result.c_str());
					ret = 1;
				}

				finished = true;
			}
		}
	} catch (SocketError ex) {
		Logger::warn("irccdctl: error: %s", ex.what());
		ret = 1;
	} catch (SocketTimeout) {
		Logger::warn("irccdctl: didn't get a response from irccd");
		ret = 1;
	}

	return ret;
}

void Irccdctl::usage()
{
	Logger::warn("usage: %s [-cv] <command> [<args>]\n", getprogname());

	Logger::warn("Commands supported:");
	Logger::warn("\tcnotice\t\tSend a channel notice");
	Logger::warn("\thelp\t\tGet this help");
	Logger::warn("\tinvite\t\tInvite someone to a channel");
	Logger::warn("\tjoin\t\tJoin a channel");
	Logger::warn("\tkick\t\tKick someone from a channel");
	Logger::warn("\tload\t\tLoad a Lua plugin");
	Logger::warn("\tme\t\tSend a CTCP Action (same as /me)");
	Logger::warn("\tmessage\t\tSend a message to someone or a channel");
	Logger::warn("\tmode\t\tChange a channel mode");
	Logger::warn("\tnotice\t\tSend a private notice");
	Logger::warn("\tnick\t\tChange your nickname");
	Logger::warn("\tpart\t\tLeave a channel");
	Logger::warn("\treload\t\tReload a Lua plugin");
	Logger::warn("\ttopic\t\tChange a channel topic");
	Logger::warn("\tumode\t\tChange a user mode");
	Logger::warn("\tunload\t\tUnload a Lua plugin");

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
	int ret;

	if (argc < 1)
		usage();
		/* NOTREACHED */

	// exceptional, do not open for "help" subject
	if (strcmp(argv[0], "help") != 0)
		openConfig();

	try {
		string cmd = argv[0];

		handlers.at(cmd)(this, --argc, ++argv);
		ret = getResponse();
	} catch (out_of_range ex) {
		Logger::warn("irccdctl: unknown command %s", argv[0]);
		return 1;
	}

	m_socket.close();
	
	return ret;
}
