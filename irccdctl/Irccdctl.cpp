/*
 * Irccdctl.cpp -- irccd controller class
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
#include <sstream>
#include <unordered_map>
#include <stdexcept>

#include <Logger.h>
#include <Parser.h>
#include <SocketAddress.h>
#include <SocketListener.h>
#include <Util.h>

#include "Irccdctl.h"

namespace irccd {

namespace {

/* {{{ help messages */

using HelpHandler = std::function<void()>;

void helpChannelNotice()
{
	Logger::warn("usage: %s cnotice server channel message\n", getprogname());
	Logger::warn("Send a notice to a public channel. This is a notice that everyone");
	Logger::warn("will be notified by.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s cnotice freenode #staff \"Don't flood\"", getprogname());
}

void helpConnect()
{
	Logger::warn("usage: %s [-k password] [-i identity] connect name address port\n", getprogname());
	Logger::warn("Connect to a new server. Specify the server ressource name, address and the port");
	Logger::warn("to use. Optional -k option specify a password. Optional -i option specify a");
	Logger::warn("specific identity to use.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s connect superserver irc.superserver.foo 6667", getprogname());
	Logger::warn("\t%s connect -k secret -i fabrice serverz irc.svz.bar 6667", getprogname());
}

void helpDisconnect()
{
	Logger::warn("usage: %s disconnect server\n", getprogname());
	Logger::warn("Disconnect from a connected server.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s disconnect server", getprogname());
}

void helpInvite()
{
	Logger::warn("usage: %s invite server nickname channel\n", getprogname());
	Logger::warn("Invite someone to a channel, needed for channel with mode +i\n");

	Logger::warn("Example:");
	Logger::warn("\t%s invite freenode xorg62 #staff", getprogname());
}

void helpJoin()
{
	Logger::warn("usage: %s join server channel [password]\n", getprogname());
	Logger::warn("Join a channel on a specific server registered in irccd. The server");
	Logger::warn("is referenced by the parameter server. Parameter channel is the channel");
	Logger::warn("to join. An optional password may be set as password parameter.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s join freenode #staff", getprogname());
}

void helpKick()
{
	Logger::warn("usage: %s kick server nick channel [reason]\n", getprogname());
	Logger::warn("Kick someone from a channel. The parameter reason is optional and");
	Logger::warn("may be ommited but when specified it must be unclosed between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s kick freenode jean #staff \"Stop flooding\"", getprogname());
}

void helpLoad()
{
	Logger::warn("usage: %s load name\n", getprogname());
	Logger::warn("Load a plugin into the irccd instance.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s load logger", getprogname());
}

void helpMe()
{
	Logger::warn("usage: %s me server target message\n", getprogname());
	Logger::warn("Send a CTCP ACTION message. It is exactly the same syntax as message.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s me freenode #staff \"going back soon\"", getprogname());
}

void helpMessage()
{
	Logger::warn("usage: %s message server target message\n", getprogname());
	Logger::warn("Send a message to someone or a channel. The target may be a channel or a real person");
	Logger::warn("If the message contains more than one word it must be enclosed between quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s message freenode #staff \"Hello from irccd\"", getprogname());
}

void helpMode()
{
	Logger::warn("usage: %s mode server channel mode\n", getprogname());
	Logger::warn("Change the mode of the specified channel. The mode contains full parameters");
	Logger::warn("like \"+b\" or \"+k secret\".\n");

	Logger::warn("Example:");
	Logger::warn("\t%s mode freenode #staff +t", getprogname());
}

void helpNick()
{
	Logger::warn("usage: %s nick server nickname\n", getprogname());
	Logger::warn("Change your nickname. The parameter nickname is the new nickname\n");

	Logger::warn("Example:");
	Logger::warn("\t%s nick freenode david", getprogname());
}

void helpNotice()
{
	Logger::warn("usage: %s notice server target message\n", getprogname());
	Logger::warn("Send a private notice to a target user.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s notice freenode jean \"Private notice\"", getprogname());
}

void helpPart()
{
	Logger::warn("usage: %s part server channel\n", getprogname());
	Logger::warn("Leave a channel. Parameter server is one registered in irccd config.");
	Logger::warn("Parameter channel is the channel to leave.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s part freenode #staff", getprogname());
}

void helpReload()
{
	Logger::warn("usage: %s reload name\n", getprogname());
	Logger::warn("Reload a plugin, parameter name is the plugin to reload.");
	Logger::warn("The plugin needs to be loaded.\n");

	Logger::warn("Example:");
	Logger::warn("\t %s reload logger", getprogname());
}

void helpRestart()
{
	Logger::warn("usage: %s restart [name]\n", getprogname());
	Logger::warn("Force a server restart. If no name parameter is given all");
	Logger::warn("servers are restarted.\n");


	Logger::warn("Example:");
	Logger::warn("\t %s restart", getprogname());
	Logger::warn("\t %s restart wanadoo", getprogname());
}

void helpTopic()
{
	Logger::warn("usage: %s topic server channel topic\n", getprogname());
	Logger::warn("Set the new topic of a channel. Topic must be enclosed between");
	Logger::warn("quotes.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s topic freenode #wmfs \"This is the best channel\"", getprogname());
}

void helpUnload()
{
	Logger::warn("usage: %s unload name\n", getprogname());
	Logger::warn("Unload a loaded plugin from the irccd instance.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s unload logger", getprogname());
}

void helpUserMode()
{
	Logger::warn("usage: %s umode server mode\n", getprogname());
	Logger::warn("Change your own user mode.\n");

	Logger::warn("Example:");
	Logger::warn("\t%s umode +i", getprogname());
}

std::unordered_map<std::string, HelpHandler> helpHandlers {
	{ "cnotice", 	helpChannelNotice	},
	{ "disconnect",	helpDisconnect		},
	{ "connect",	helpConnect		},
	{ "invite",	helpInvite		},
	{ "join",	helpJoin		},
	{ "kick",	helpKick		},
	{ "load",	helpLoad		},
	{ "me",		helpMe			},
	{ "message",	helpMessage		},
	{ "mode",	helpMode		},
	{ "notice",	helpNotice		},
	{ "nick",	helpNick		},
	{ "part",	helpPart		},
	{ "reload",	helpReload		},
	{ "restart",	helpRestart		},
	{ "topic",	helpTopic		},
	{ "umode",	helpUserMode		},
	{ "unload",	helpUnload		}
};

/* }}} */

/* {{{ handlers */

using Handler = std::function<void(Irccdctl *, int, char **)>;

void handleHelp(Irccdctl *, int argc, char **argv)
{
	if (argc < 1)
		Logger::fatal(1, "help requires 1 argument");

	try {
		helpHandlers.at(argv[0])();
	} catch (std::out_of_range ex) {
		Logger::warn("There is no subject named %s", argv[0]);
	}

	std::exit(1);
}

void handleChannelNotice(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "cnotice requires 3 arguments");

	oss << "CNOTICE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handleConnect(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "connect requires 3 arguments");

	oss << "CONNECT " << argv[0] << " " << argv[1];
	oss << " " << argv[2];

	// Identity and password are optional
	if (ctl->hasArg('i'))
		oss << " ident:" << ctl->getArg('i');
	if (ctl->hasArg('k'))
		oss << " key:" << ctl->getArg('k');
	if (ctl->hasArg('s'))
		oss << " ssl:on";
	oss << "\n";
	
	ctl->sendRaw(oss.str());
}

void handleDisconnect(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "disonnect requires 1 argument");

	oss << "DISCONNECT " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

void handleInvite(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "invite requires 3 arguments");

	oss << "INVITE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";

	ctl->sendRaw(oss.str());
}

void handleJoin(Irccdctl *ctl, int argc, char **argv)
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

void handleKick(Irccdctl *ctl, int argc, char **argv)
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

void handleLoad(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "load requires 1 argument");

	oss << "LOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

void handleMe(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "me requires 3 arguments");

	oss << "ME " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handleMessage(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "message requires 3 arguments");

	oss << "MSG " << argv[0] << " " << argv[1] << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handleMode(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "mode requires 3 arguments");

	oss << "MODE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handleNick(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "nick requires 2 arguments");

	oss << "NICK " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

void handleNotice(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "notice requires 3 arguments");

	oss << "NOTICE " << argv[0] << " " << argv[1];
	oss << " " << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handlePart(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "part requires 2 arguments");

	oss << "PART " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

void handleReload(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "reload requires 1 argument");

	oss << "RELOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

void handleRestart(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	oss << "RESTART ";
	oss << ((argc < 1) ? "__ALL__" : argv[0]);
	oss << "\n";

	ctl->sendRaw(oss.str());
}

void handleTopic(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 3)
		Logger::fatal(1, "topic requires 3 arguments");

	oss << "TOPIC " << argv[0] << " " << argv[1] << " ";
	oss << argv[2] << "\n";
	ctl->sendRaw(oss.str());
}

void handleUnload(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 1)
		Logger::fatal(1, "unload requires 1 argument");

	oss << "UNLOAD " << argv[0] << "\n";
	ctl->sendRaw(oss.str());
}

void handleUserMode(Irccdctl *ctl, int argc, char **argv)
{
	std::ostringstream oss;

	if (argc < 2)
		Logger::fatal(1, "umode requires 2 arguments");

	oss << "UMODE " << argv[0] << " " << argv[1] << "\n";
	ctl->sendRaw(oss.str());
}

std::unordered_map<std::string, Handler> handlers {
	{ "cnotice",	handleChannelNotice	},
	{ "connect",	handleConnect		},
	{ "disconnect",	handleDisconnect	},
	{ "help",	handleHelp		},
	{ "invite",	handleInvite		},
	{ "join",	handleJoin		},
	{ "kick",	handleKick		},
	{ "load",	handleLoad		},
	{ "me",		handleMe		},
	{ "message",	handleMessage		},
	{ "mode",	handleMode		},
	{ "nick",	handleNick		},
	{ "notice",	handleNotice		},
	{ "part",	handlePart		},
	{ "reload",	handleReload		},
	{ "restart",	handleRestart		},
	{ "topic",	handleTopic		},
	{ "umode",	handleUserMode		},
	{ "unload",	handleUnload		}
};

}

/* }}} */

Irccdctl::Irccdctl()
	: m_needResponse(true)
	, m_readConfig(true)
{
	Socket::init();
	Logger::setVerbose(false);

#if !defined(_WIN32)
	m_removeFiles = false;
#endif
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

void Irccdctl::loadUnix(const Section &section)
{
	m_domain = AF_LOCAL;
	m_unixPath = section.requireOption<std::string>("path");
}

void Irccdctl::connectUnix()
{
	try {
		char *p;
		char dir[FILENAME_MAX] = "/tmp/irccdctl-XXXXXXXXX";

		m_socket = Socket(AF_LOCAL, m_type, 0);

		if (m_type == SOCK_STREAM)
			m_socket.connect(AddressUnix(m_unixPath));
		else
			m_addr = AddressUnix(m_unixPath);

		/*
		 * Unix domain socket needs a temporarly file for getting a
		 * response.
		 *
		 * If we can't create a directory we don't wait for a response
		 * silently.
		 */
		if ((p = mkdtemp(dir)) != NULL) {
			m_tmpDir = dir;
			m_tmpPath = std::string(dir) + std::string("/response.sock");

			m_socket.bind(AddressUnix(m_tmpPath));
			m_removeFiles = true;
		} else
			m_needResponse = false;
	} catch (SocketError error) {
		removeUnixFiles();
		Logger::fatal(1, "irccdctl: failed to connect to %s: %s",
		    m_unixPath.c_str(), error.what());
	}
}

void Irccdctl::removeUnixFiles()
{
	if (m_removeFiles) {
		::remove(m_tmpPath.c_str());
		::remove(m_tmpDir.c_str());
	}
}

#endif

void Irccdctl::loadInet(const Section &section)
{
	std::string inet;

	m_host = section.requireOption<std::string>("host");
	m_port = section.requireOption<int>("port");

	inet = section.requireOption<std::string>("family");
	if (inet == "ipv4")
		m_domain = AF_INET;
	else if (inet == "ipv6")
		m_domain = AF_INET6;
	else
		Logger::fatal(1, "socket: parameter family is one of them: ipv4, ipv6");
}

void Irccdctl::connectInet()
{
	try {
		m_socket = Socket(m_domain, m_type, 0);

		if (m_type == SOCK_STREAM)
			m_socket.connect(ConnectAddressIP(m_host, m_port, m_domain));
		else
			m_addr = ConnectAddressIP(m_host, m_port, m_domain, SOCK_DGRAM);
	} catch (SocketError error) {
		Logger::fatal(1, "irccdctl: failed to connect: %s", error.what());
	}
}

void Irccdctl::readConfig(Parser &config)
{
	using std::string;

	try {
		const Section &section= config.getSection("socket");
		string type;
		string proto;

		type = section.requireOption<string>("type");
		proto = section.requireOption<string>("protocol");

		if (proto != "tcp" && proto != "udp")
			Logger::fatal(1, "socket: invalid protocol `%s'", proto.c_str());

		m_type = (proto == "tcp") ? SOCK_STREAM : SOCK_DGRAM;

		/*
		 * Connect to the socket, every of these function may exits if
		 * they can't connect.
		 */
		if (type == "unix")
#if !defined(_WIN32)
			loadUnix(section);
#else
			Logger::fatal(1, "socket: unix sockets are not supported on Windows");
#endif
		else if (type == "internet")
			loadInet(section);
		else
			Logger::fatal(1, "socket: invalid socket type %s", type.c_str());
	} catch (std::out_of_range ex) {
		Logger::fatal(1, "socket: missing parameter %s", ex.what());
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
	try {
	if (m_configPath.length() == 0) {
		try {
			m_configPath = Util::findConfiguration("irccdctl.conf");
			config = Parser(m_configPath);
		} catch (Util::ErrorException ex) {
			Logger::fatal(1, "%s: %s", getprogname(), ex.what());
		}
	} else
		config = Parser(m_configPath);
	} catch (std::runtime_error) {
		Logger::fatal(1, "irccdctl: could not open %s, exiting", m_configPath.c_str());
	}

	readConfig(config);
}

void Irccdctl::sendRaw(const std::string &message)
{
	try {
		if (m_socket.getType() == SOCK_STREAM)
			m_socket.send(message.c_str(), message.length());
		else
			m_socket.sendto(message.c_str(), message.length(), m_addr);
	} catch (SocketError ex) {
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
	try {
		while (!finished) {
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
			else {
				std::string result;

				data[nbread] = '\0';
				oss << data;

				pos = oss.str().find_first_of('\n');
				if (pos == std::string::npos)
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
		Logger::warn("irccdctl: did not get a response from irccd");
		ret = 1;
	}

	return ret;
}

void Irccdctl::addArg(char c, const std::string &arg)
{
	m_args[c] = arg;
}

bool Irccdctl::hasArg(char c)
{
	return m_args.count(c) > 0;
}

const std::string &Irccdctl::getArg(char c)
{
	return m_args[c];
}

void Irccdctl::usage()
{
	Logger::warn("usage: %s [-cv] <command> [<args>]\n", getprogname());

	Logger::warn("Commands supported:");
	Logger::warn("\tcnotice\t\tSend a channel notice");
	Logger::warn("\tconnect\t\tConnect to a server");
	Logger::warn("\tdisconnect\tDisconnect from a server");
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
	Logger::warn("\trestart\t\tRestart one or all servers");
	Logger::warn("\ttopic\t\tChange a channel topic");
	Logger::warn("\tumode\t\tChange a user mode");
	Logger::warn("\tunload\t\tUnload a Lua plugin");

	Logger::fatal(1, "\nFor more information on a command, type %s help <command>", getprogname());
}

void Irccdctl::setConfigPath(const std::string &path)
{
	m_configPath = path;
}

void Irccdctl::useInternet(const std::string &host,
			   int port,
			   int domain,
			   int type)
{
	m_readConfig = false;

	m_domain = domain;
	m_type = type;
	m_host = host;
	m_port = port;
}

#if !defined(_WIN32)

void Irccdctl::useUnix(const std::string &path, int type)
{
	m_readConfig = false;

	m_domain = AF_LOCAL;
	m_type = type;
	m_unixPath = path;
}

#endif

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
	if (strcmp(argv[0], "help") != 0 && m_readConfig)
		openConfig();

	// Try to connect
	if (m_domain == AF_INET || m_domain == AF_INET6)
		connectInet();
#if !defined(_WIN32)
	else if (m_domain == AF_LOCAL)
		connectUnix();
#endif

	try {
		std::string cmd = argv[0];

		handlers.at(cmd)(this, --argc, ++argv);

		if (m_needResponse)
			ret = getResponse();
	} catch (std::out_of_range ex) {
		Logger::warn("irccdctl: unknown command %s", argv[0]);
		return 1;
	}

	m_socket.close();
	
	return ret;
}

} // !irccd
