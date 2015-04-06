/*
 * Irccdctl.cpp -- irccd controller class
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#include <IrccdConfig.h>

#include <ElapsedTimer.h>
#include <Filesystem.h>
#include <Json.h>
#include <Logger.h>
#include <SocketAddress.h>
#include <SocketListener.h>
#include <Util.h>

#include "Irccdctl.h"

using namespace std::string_literals;

namespace irccd {

/* --------------------------------------------------------
 * Help messages
 * -------------------------------------------------------- */

void Irccdctl::helpChannelNotice() const
{
	Logger::warning() << "usage: " << getprogname() << " cnotice server channel message\n\n"
			  << "Send a notice to a public channel. This is a notice that everyone\n"
			  << "will be notified by.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " cnotice freenode #staff \"Don't flood\"" << std::endl;
}

void Irccdctl::helpConnect() const
{
	Logger::warning() << "usage: " << getprogname() << " connect [-k password] [-i identity] name address port\n\n"
			  << "Connect to a new server. Specify the server ressource name, address and the port\n"
			  << "to use. Optional -k option specify a password. Optional -i option specify a\n"
			  << "specific identity to use.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " connect superserver irc.superserver.foo 6667\n"
			  << "\t" << getprogname() << " connect -k secret -i fabrice serverz irc.svz.bar 6667" << std::endl;
}

void Irccdctl::helpDisconnect() const
{
	Logger::warning() << "usage: " << getprogname() << " disconnect server\n\n"
			  << "Disconnect from a connected server.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " disconnect server" << std::endl;
}

void Irccdctl::helpInvite() const
{
	Logger::warning() << "usage: " << getprogname() << " invite server nickname channel\n\n"
			  << "Invite someone to a channel, needed for channel with mode +i\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " invite freenode xorg62 #staff" << std::endl;
}

void Irccdctl::helpJoin() const
{
	Logger::warning() << "usage: " << getprogname() << " join server channel [password]\n\n"
			  << "Join a channel on a specific server registered in irccd. The server\n"
			  << "is referenced by the parameter server. Parameter channel is the channel\n"
			  << "to join. An optional password may be set as the third argument.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " join freenode #staff" << std::endl;
}

void Irccdctl::helpKick() const
{
	Logger::warning() << "usage: " << getprogname() << " kick server nick channel [reason]\n"
			  << "Kick someone from a channel. The parameter reason is optional and\n"
			  << "may be ommited but when specified it must be unclosed between quotes.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " kick freenode jean #staff \"Stop flooding\"" << std::endl;
}

void Irccdctl::helpLoad() const
{
	Logger::warning() << "usage: " << getprogname() << " load name\n"
			  << "Load a plugin into the irccd instance.\n\n"
			  << "Example:\n"
			  << "\t%s load logger" << std::endl;
}

void Irccdctl::helpMe() const
{
	Logger::warning() << "usage: " << getprogname() << " me server target message\n"
			  << "Send a CTCP ACTION message. It is exactly the same syntax as message.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " me freenode #staff \"going back soon\"" << std::endl;
}

void Irccdctl::helpMessage() const
{
	Logger::warning() << "usage: " << getprogname() << " message server target message\n"
			  << "Send a message to someone or a channel. The target may be a channel or a real person\n"
			  << "If the message contains more than one word it must be enclosed between quotes.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " message freenode #staff \"Hello from irccd\"" << std::endl;
}

void Irccdctl::helpMode() const
{
	Logger::warning() << "usage: " << getprogname() << " mode server channel mode\n"
			  << "Change the mode of the specified channel. The mode contains full parameters\n"
			  << "like \"+b\" or \"+k secret\".\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " mode freenode #staff +t" << std::endl;
}

void Irccdctl::helpNick() const
{
	Logger::warning() << "usage: " << getprogname() << " nick server nickname\n"
			  << "Change your nickname. The parameter nickname is the new nickname\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " nick freenode david" << std::endl;
}

void Irccdctl::helpNotice() const
{
	Logger::warning() << "usage: " << getprogname() << " notice server target message\n"
			  << "Send a private notice to a target user.\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " notice freenode jean \"Private notice\"" << std::endl;
}

void Irccdctl::helpPart() const
{
	Logger::warning() << "usage: " << getprogname() << " part server channel\n"
			  << "Leave a channel. Parameter server is one registered in irccd config.\n"
			  << "Parameter channel is the channel to leave.\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " part freenode #staff" << std::endl;
}

void Irccdctl::helpReconnect() const
{
	Logger::warning() << "usage: " << getprogname() << " reconnect [name]\n"
			  << "Force a server restart. If no name parameter is given all\n"
			  << "servers are restarted.\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " reconnect"
			  << "\t" << getprogname() << " reconnect wanadoo" << std::endl;
}

void Irccdctl::helpReload() const
{
	Logger::warning() << "usage: " << getprogname() << " reload name\n"
			  << "Reload a plugin, parameter name is the plugin to reload.\n"
			  << "The plugin needs to be loaded.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " reload logger" << std::endl;
}

void Irccdctl::helpTopic() const
{
	Logger::warning() << "usage: " << getprogname() << " topic server channel topic\n"
			  << "Set the new topic of a channel. Topic must be enclosed between\n"
			  << "quotes.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " topic freenode #wmfs \"This is the best channel\"" << std::endl;
}

void Irccdctl::helpUnload() const
{
	Logger::warning() << "usage: " << getprogname() << " unload name\n"
			  << "Unload a loaded plugin from the irccd instance.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " unload logger" << std::endl;
}

void Irccdctl::helpUserMode() const
{
	Logger::warning() << "usage: " << getprogname() << " umode server mode\n"
			  << "Change your own user mode.\n\n"
			  << "Example:\n"
			  << "\t" << getprogname() << " umode +i" << std::endl;
}

/* --------------------------------------------------------
 * Commands
 * -------------------------------------------------------- */

void Irccdctl::handleHelp(int argc, char **argv)
{
	if (argc < 1) {
		Logger::warning() << "help requires 1 argument" << std::endl;
	} else {
		try {
			// TODO
			//m_helpers.at(argv[0])();
		} catch (const std::exception &) {
			Logger::warning() << "There are no subject named " << argv[0] << std::endl;
		}
	}
}

void Irccdctl::handleChannelNotice(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "cnotice requires 3 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"cnotice\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"channel\":\"" << argv[1] << "\","
		    <<   "\"message\":\"" << JsonValue::escape(argv[2]) << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleConnect(int argc, char **argv)
{
#if 0
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
#endif
}

void Irccdctl::handleDisconnect(int argc, char **argv)
{
	if (argc < 1) {
		Logger::warning() << "disonnect requires 1 argument" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"disconnect\","
		    <<   "\"server\":\"" << argv[0] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleInvite(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "invite requires 3 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"invite\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"target\":\"" << argv[1] << "\","
		    <<   "\"channel\":\"" << argv[2] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleJoin(int argc, char **argv)
{
	if (argc < 2) {
		Logger::warning() << "join requires at least 2 arguments" << std::endl;
	} else {
		std::ostringstream oss;
		std::string password = (argc >= 3) ? argv[2] : "";

		oss << "{"
		    <<   "\"command\":\"join\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"channel\":\"" << argv[1] << "\","
		    <<   "\"password\":\"" << password << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleKick(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "kick requires at least 3 arguments " << std::endl;
	} else {
		std::ostringstream oss;
		std::string reason = (argc >= 4) ? argv[3] : "";

		oss << "{"
		    <<   "\"command\":\"kick\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"target\":\"" << argv[1] << "\","
		    <<   "\"channel\":\"" << argv[2] << "\","
		    <<   "\"reason\":\"" << reason << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleLoad(int argc, char **argv)
{
	if (argc < 1) {
		Logger::warning() << "load requires 1 argument" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"load\",";

		if (Filesystem::isAbsolute(argv[0])) {
			oss << "\"path\":\"" << argv[0] << "\"";
		} else {
			oss << "\"name\":\"" << argv[0] << "\"";
		}

		oss << "}";

		send(oss.str());
	}
}

void Irccdctl::handleMe(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "me requires 3 arguments" << std::endl;
	} else {
		//std::ostringstream oss;

		// TRANSPORT MUST UNDERSTAND TARGET INSTEAD
	}
}

void Irccdctl::handleMessage(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "message requires 3 arguments" << std::endl;
	} else {
		//std::ostringstream oss;

		// TRANSPORT MUST RENAME SAY TO MESSAGE
	}
}

void Irccdctl::handleMode(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "mode requires 3 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"mode\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"channel\":\"" << argv[1] << "\","
		    <<   "\"mode\":\"" << argv[2] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleNick(int argc, char **argv)
{
	if (argc < 2) {
		Logger::warning() << "nick requires 2 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"nick\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"nickname\":\"" << argv[1] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleNotice(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "notice requires 3 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"notice\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"target\":\"" << argv[1] << "\","
		    <<   "\"message\":\"" << JsonValue::escape(argv[2]) << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handlePart(int argc, char **argv)
{
	if (argc < 2) {
		Logger::warning() << "part requires 2 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"part\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"channel\":\"" << argv[1] << "\","
		    <<   "\"reason\":\"" << JsonValue::escape(argv[2]) << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleReconnect(int argc, char **argv)
{
	std::ostringstream oss;

	oss << "{"
	    <<   "\"command\":\"reconnect\"";

	if (argc >= 1) {
		oss << ",\"server\":\"" << argv[0] << "\"";
	}

	oss << "}";

	send(oss.str());
}

void Irccdctl::handleReload(int argc, char **argv)
{
	if (argc < 1) {
		Logger::warning() << "reload requires 1 argument" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"reload\","
		    <<   "\"plugin\":\"" << argv[0] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleTopic(int argc, char **argv)
{
	if (argc < 3) {
		Logger::warning() << "topic requires 3 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"topic\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"channel\":\"" << argv[1] << "\","
		    <<   "\"topic\":\"" << argv[1] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleUnload(int argc, char **argv)
{
	if (argc < 1) {
		Logger::warning() << "unload requires 1 argument" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"unload\","
		    <<   "\"plugin\":\"" << argv[0] << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::handleUserMode(int argc, char **argv)
{
	if (argc < 2) {
		Logger::warning() << "umode requires 2 arguments" << std::endl;
	} else {
		std::ostringstream oss;

		oss << "{"
		    <<   "\"command\":\"umode\","
		    <<   "\"server\":\"" << argv[0] << "\","
		    <<   "\"mode\":\"" << JsonValue::escape(argv[1]) << "\""
		    << "}";

		send(oss.str());
	}
}

void Irccdctl::send(std::string message)
{
	try {
		message += "\r\n\r\n";

		m_socket.waitSend(message, 10000);
	} catch (const std::exception &ex) {
		throw std::runtime_error("irccdctl: "s + ex.what());
	}
}

void Irccdctl::usage()
{
	Logger::warning() << "usage: " << getprogname() << " [-cv] <command> [<args>]\n"
			  << "Commands supported:\n"
			  << "\tcnotice\t\tSend a channel notice\n"
			  << "\tconnect\t\tConnect to a server\n"
			  << "\tdisconnect\tDisconnect from a server\n"
			  << "\thelp\t\tGet this help\n"
			  << "\tinvite\t\tInvite someone to a channel\n"
			  << "\tjoin\t\tJoin a channel\n"
			  << "\tkick\t\tKick someone from a channel\n"
			  << "\tload\t\tLoad a JavaScript plugin\n"
			  << "\tme\t\tSend a CTCP Action (same as /me)\n"
			  << "\tmessage\t\tSend a message to someone or a channel\n"
			  << "\tmode\t\tChange a channel mode\n"
			  << "\tnotice\t\tSend a private notice\n"
			  << "\tnick\t\tChange your nickname\n"
			  << "\tpart\t\tLeave a channel\n"
			  << "\treload\t\tReload a JavaScript plugin\n"
			  << "\trestart\t\tRestart one or all servers\n"
			  << "\ttopic\t\tChange a channel topic\n"
			  << "\tumode\t\tChange a user mode\n"
			  << "\tunload\t\tUnload a JavaScript plugin\n"
			  << "\nFor more information on a command, type " << getprogname() << " help <command>" << std::endl;
}

Irccdctl::Irccdctl(SocketTcp s)
	: m_socket(std::move(s))
	, m_helpers{
		{ "cnotice", 	&Irccdctl::helpChannelNotice	},
		{ "disconnect",	&Irccdctl::helpDisconnect	},
		{ "connect",	&Irccdctl::helpConnect		},
		{ "invite",	&Irccdctl::helpInvite		},
		{ "join",	&Irccdctl::helpJoin		},
		{ "kick",	&Irccdctl::helpKick		},
		{ "load",	&Irccdctl::helpLoad		},
		{ "me",		&Irccdctl::helpMe		},
		{ "message",	&Irccdctl::helpMessage		},
		{ "mode",	&Irccdctl::helpMode		},
		{ "notice",	&Irccdctl::helpNotice		},
		{ "nick",	&Irccdctl::helpNick		},
		{ "part",	&Irccdctl::helpPart		},
		{ "reconnect",	&Irccdctl::helpReconnect	},
		{ "reload",	&Irccdctl::helpReload		},
		{ "topic",	&Irccdctl::helpTopic		},
		{ "umode",	&Irccdctl::helpUserMode		},
		{ "unload",	&Irccdctl::helpUnload		}
	}
	, m_handlers{
		{ "cnotice",	&Irccdctl::handleChannelNotice	},
		{ "connect",	&Irccdctl::handleConnect	},
		{ "disconnect",	&Irccdctl::handleDisconnect	},
		{ "help",	&Irccdctl::handleHelp		},
		{ "invite",	&Irccdctl::handleInvite		},
		{ "join",	&Irccdctl::handleJoin		},
		{ "kick",	&Irccdctl::handleKick		},
		{ "load",	&Irccdctl::handleLoad		},
		{ "me",		&Irccdctl::handleMe		},
		{ "message",	&Irccdctl::handleMessage	},
		{ "mode",	&Irccdctl::handleMode		},
		{ "nick",	&Irccdctl::handleNick		},
		{ "notice",	&Irccdctl::handleNotice		},
		{ "part",	&Irccdctl::handlePart		},
		{ "reconnect",	&Irccdctl::handleReconnect	},
		{ "reload",	&Irccdctl::handleReload		},
		{ "topic",	&Irccdctl::handleTopic		},
		{ "umode",	&Irccdctl::handleUserMode	},
		{ "unload",	&Irccdctl::handleUnload		}
	}
{
}

#if 0

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
		const Section &section = config.getSection("socket");
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
		Logger::fatal(1, "socket: parameter %s", ex.what());
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
		} catch (std::runtime_error ex) {
			Logger::fatal(1, "%s: %s", getprogname(), ex.what());
		}
	} else
		config = Parser(m_configPath);
	} catch (std::runtime_error) {
		Logger::fatal(1, "irccdctl: could not open %s, exiting", m_configPath.c_str());
	}

	readConfig(config);
}

#endif

std::string Irccdctl::response()
{
	ElapsedTimer timer;
	bool done = false;
	bool received = false;
	std::string result;
	std::string::size_type pos;

	while (!done && timer.elapsed() < 10000) {
		result += m_socket.waitRecv(512, 10000 - timer.elapsed());

		while ((pos = result.find("\r\n\r\n")) == std::string::npos) {
			std::string message = result.substr(0U, pos);

			result.erase(0U, pos + 4);

			JsonDocument document(message);
			JsonObject object = document.toObject();

			/* Skip non result messages */
			if (!object.contains("command")) {
				continue;
			}

			if (object.contains("result")) {
				result = object["result"].toString();
			} else if (object.contains("error")) {
				result = object["error"].toString();
			}

			received = true;
			done = true;
		}
	}

	if (!received) {
		throw std::runtime_error("no response received");
	}

	return result;
}

int Irccdctl::exec(int argc, char **argv)
{
	if (argc <= 0) {
		usage();
		return 1;
	}

	if (m_handlers.count(argv[0]) == 0) {
		Logger::warning() << getprogname() << ": " << argv[0] << ": invalid command" << std::endl;
		return 1;
	}

	try {
		std::string cmd = argv[0];

		(this->*m_handlers.at(cmd))(--argc, ++argv);
	} catch (const std::exception &ex) {
		Logger::warning() << getprogname() << ": " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}

} // !irccd
