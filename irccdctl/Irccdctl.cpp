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

#include <cstring>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>

#include <Logger.h>
#include <Parser.h>
#include <SocketAddress.h>
#include <SocketListener.h>
#include <Util.h>

#include "Irccdctl.h"

namespace irccd
{

/* {{{ help messages */

typedef std::function<void()> HelpHandler;

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

static std::map<std::string, HelpHandler> createHelpHandlers()
{
	std::map<std::string, HelpHandler> helpHandlers;

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

static std::map<std::string, HelpHandler> helpHandlers = createHelpHandlers();

/* }}} */

/* {{{ handlers */

typedef std::function<void(Irccdctl *, int, char **)> Handler;

static void handleHelp(Irccdctl *, int argc, char **argv)
{
	if (argc < 1)
		Logger::fatal(1, "help requires 1 argument");

	try
	{
		helpHandlers.at(argv[0])();
	}
	catch (std::out_of_range ex)
	{
		Logger::warn("There is no subject named %s", argv[0]);
	}

	std::exit(1);
}

static void handleChannelNotice(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "cnotice requires 3 arguments");

	oss << "CNOTICE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleInvite(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "invite requires 3 arguments");

	oss << "INVITE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";

	ctl->sendRaw(oss.str());
}

static void handleJoin(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "join requires at least 2 arguments");

	oss << "JOIN " << argv[0] << " " << argv[1];

	// optional password
	if (argc >= 3)
		oss << " " << argv[2];
	oss << "\n";
	ctl->sendRaw(oss.str());
}

static void handleKick(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "kick requires at least 3 arguments ");

	oss << "KICK " << argv[0] << " " << argv[1] << " " << argv[2];

	// optional reason
	if (argc >= 4)
		oss << " " << argv[3];
	oss << "\n";
	ctl->sendRaw(oss.str());
}

static void handleLoad(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "load requires 1 argument");

	oss << "LOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleMe(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "me requires 3 arguments");

	oss << "ME " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleMessage(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "message requires 3 arguments");

	oss << "MSG " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleMode(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "mode requires 3 arguments");

	oss << "MODE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleNick(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "nick requires 2 arguments");

	oss << "NICK " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleNotice(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "notice requires 3 arguments");

	oss << "NOTICE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handlePart(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "part requires 2 arguments");

	oss << "PART " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleReload(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "reload requires 1 argument");

	oss << "RELOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleTopic(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "topic requires 3 arguments");

	oss << "TOPIC " << argv[0] << " " << argv[1] << " ";
	oss << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleUnload(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "unload requires 1 argument");

	oss << "UNLOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

static void handleUserMode(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "umode requires 2 arguments");

	oss << "UMODE " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

static std::map<std::string, Handler> createHandlers()
{
	std::map<std::string, Handler> handlers;

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

static std::map<std::string, Handler> handlers = createHandlers();

/* }}} */

Irccdctl::Irccdctl()
	: m_needResponse(true)
	, m_removeFiles(false)
{
	Socket::init();

	Logger::setVerbose(false);
}

Irccdctl::~Irccdctl()
{
	Socket::finish();

#if !defined(_WIN32)
	if (m_socket.getDomain() == AF_LOCAL)
		removeUnixFiles();
#endif
}

#if !defined(_WIN32)

void Irccdctl::connectUnix(const Section &section, int type)
{
	using std::string;

	string path;

	path = section.requireOption<string>("path");

	try
	{
		char *p;
		char dir[FILENAME_MAX] = "/tmp/irccdctl-XXXXXXXXX";

		m_socket = Socket(AF_LOCAL, type, 0);

		if (type == SOCK_STREAM)
			m_socket.connect(AddressUnix(path));
		else
			m_addr = AddressUnix(path);

		/*
		 * Unix domain socket needs a temporarly file for getting a
		 * response.
		 *
		 * If we can't create a directory we don't wait for a response
		 * silently.
		 */
		if ((p = mkdtemp(dir)) != NULL)
		{
			m_tmpDir = dir;
			m_tmpPath = string(dir) + string("/response.sock");

			m_socket.bind(AddressUnix(m_tmpPath));
			m_removeFiles = true;
		}
		else
			m_needResponse = false;
	}
	catch (SocketError error)
	{
		removeUnixFiles();
		Logger::fatal(1, "irccdctl: failed to connect to %s: %s", path.c_str(), error.what());
	}
}

void Irccdctl::removeUnixFiles()
{
	if (m_removeFiles)
	{
		::remove(m_tmpPath.c_str());
		::remove(m_tmpDir.c_str());
	}
}

#endif

void Irccdctl::connectInet(const Section &section, int type)
{
	using std::string;

	string host, inet;
	int port, family = 0;

	host = section.requireOption<string>("host");
	port = section.requireOption<int>("port");
	inet = section.requireOption<string>("family");

	if (inet == "ipv4")
		family = AF_INET;
	else if (inet == "ipv6")
		family = AF_INET6;
	else
		Logger::fatal(1, "socket: parameter family is one of them: ipv4, ipv6");

	try
	{
		m_socket = Socket(family, type, 0);

		if (type == SOCK_STREAM)
			m_socket.connect(ConnectAddressIP(host, port, family));
		else
			m_addr = ConnectAddressIP(host, port, family, SOCK_DGRAM);
	}
	catch (SocketError error)
	{
		Logger::fatal(1, "irccdctl: failed to connect: %s", error.what());
	}
}

void Irccdctl::readConfig(Parser &config)
{
	using std::string;

	try
	{
		const Section &sectionSocket = config.getSection("socket");
		string type;
		string proto;

		type = sectionSocket.requireOption<string>("type");
		proto = sectionSocket.requireOption<string>("protocol");

		if (proto != "tcp" && proto != "udp")
			Logger::fatal(1, "listener: protocol not valid, must be tcp or udp");

		/*
		 * Connect to the socket, every of these function may exits if
		 * they can't connect.
		 */
		if (type == "unix")
		{
#if !defined(_WIN32)
			connectUnix(sectionSocket, (proto == "tcp") ? SOCK_STREAM : SOCK_DGRAM);
#else
			Logger::fatal(1, "socket: unix sockets are not supported on Windows");
#endif
		}
		else if (type == "internet")
			connectInet(sectionSocket, (proto == "tcp") ? SOCK_STREAM : SOCK_DGRAM);
		else
			Logger::fatal(1, "socket: invalid socket type %s", type.c_str());
	}
	catch (NotFoundException ex)
	{
		Logger::fatal(1, "socket: missing parameter %s", ex.which().c_str());
	}
}

void Irccdctl::openConfig()
{
	Parser config;

	/*
	 * If m_configPath.length() is 0 we have not specified
	 * a config file by hand.
	 *
	 * Otherwise, we open the default files.
	 */
	if (m_configPath.length() == 0)
	{
		Util::findConfig("irccdctl.conf", Util::HintAll,
		    [&] (const std::string &path) -> bool {
			config = Parser(path);

			// Keep track of loaded files
			if (!config.open())
				return false;

			m_configPath = path;

			return true;
		    }
		);
	}
	else
	{
		config = Parser(m_configPath);

		if (!config.open())
			Logger::fatal(1, "irccdctl: could not open %s, exiting", m_configPath.c_str());
	}

	readConfig(config);
}

void Irccdctl::sendRaw(const std::string &message)
{
	try
	{
		if (m_socket.getType() == SOCK_STREAM)
			m_socket.send(message.c_str(), message.length());
		else
			m_socket.sendto(message.c_str(), message.length(), m_addr);
	}
	catch (SocketError ex)
	{
		Logger::fatal(1, "irccdctl: failed to send message: %s", ex.what());
	}
}

int Irccdctl::getResponse()
{
	SocketListener listener;
	std::ostringstream oss;
	bool finished = false;
	int ret = 0;

	listener.add(m_socket);
	try
	{
		while (!finished)
		{
			char data[128];
			unsigned nbread;
			size_t pos;

			listener.select(30);

			if (m_socket.getType() == SOCK_DGRAM)
				nbread = m_socket.recvfrom(data, sizeof (data) - 1);
			else
				nbread = m_socket.recv(data, sizeof (data) - 1);

			if (nbread == 0)
				finished = true;
			else
			{
				std::string result;

				data[nbread] = '\0';
				oss << data;

				pos = oss.str().find_first_of('\n');
				if (pos == std::string::npos)
					continue;

				result = oss.str().substr(0, pos);
				if (result != "OK")
				{
					Logger::warn("irccdctl: error, server said: %s", result.c_str());
					ret = 1;
				}

				finished = true;
			}
		}
	}
	catch (SocketError ex)
	{
		Logger::warn("irccdctl: error: %s", ex.what());
		ret = 1;
	}
	catch (SocketTimeout)
	{
		Logger::warn("irccdctl: did not get a response from irccd");
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

	Logger::fatal(1, "\nFor more information on a command, type %s help <command>", getprogname());
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
	int ret = 0;

	if (argc < 1)
		usage();
		/* NOTREACHED */

	// exceptional, do not open for "help" subject
	if (strcmp(argv[0], "help") != 0)
		openConfig();

	try
	{
		std::string cmd = argv[0];

		handlers.at(cmd)(this, --argc, ++argv);

		if (m_needResponse)
			ret = getResponse();
	}
	catch (std::out_of_range ex)
	{
		Logger::warn("irccdctl: unknown command %s", argv[0]);
		return 1;
	}

	m_socket.close();
	
	return ret;
}

} // !irccd
