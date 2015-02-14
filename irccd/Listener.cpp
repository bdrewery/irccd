/*
 * Listener.cpp -- listener for irccdctl clients
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

#include <algorithm>
#include <utility>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"
#include "Listener.h"
#include "ServerManager.h"
#include "PluginManager.h"

namespace irccd {

namespace {

using Optional		= std::pair<std::string, std::string>;
using SocketFunction	= void (*)(const std::vector<std::string> &);

/**
 * @struct ClientHandler
 * @brief Irccdctl function
 *
 * This describe a function to be called. It requires a number of arguments,
 * how many to split and a function to call.
 */
struct ClientHandler {
	int		m_noargs;
	int		m_nosplit;
	SocketFunction	m_function;
};

Optional getOptional(const std::string &line)
{
	auto pos = line.find(":");

	if (pos != std::string::npos)
		return std::make_pair(line.substr(0, pos), line.substr(pos + 1));

	return Optional();
}

void handleConnect(const std::vector<std::string> &params)
{
	Server::Info info;
	Server::Identity ident;
	Server::RetryInfo reco;
	unsigned options = 0;

	info.name = params[0];
	info.host = params[1];

	auto &manager = ServerManager::instance();

	if (manager.has(info.name))
		throw std::runtime_error("server " + info.name + " already connected");

	try {
		info.port = std::stoi(params[2]);
	} catch (...) {
		throw std::runtime_error("invalid port");
	}

	if (params.size() >= 4) {
		for (size_t i = 3; i < params.size(); ++i) {
			auto o = getOptional(params[i]);

			if (o.first == "key")
				info.password = o.second;
			if (o.first == "ident")
				ident = Irccd::instance().findIdentity(o.second);
			if (o.first == "ssl")
				options |= Server::OptionSsl;
		}
	}

	manager.add(std::make_shared<Server>(info, ident, reco, options));
}

void handleChannelNotice(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->cnotice(params[1], params[2]);
}

void handleDisconnect(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->stop();
}

void handleInvite(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->invite(params[1], params[2]);
}

void handleJoin(const std::vector<std::string> &params)
{
	std::string password;
	if (params.size() == 3)
		password = params[2];

	ServerManager::instance().get(params[0])->join(params[1], password);
}

void handleKick(const std::vector<std::string> &params)
{
	std::string reason;
	if (params.size() == 4)
		reason = params[3];

	ServerManager::instance().get(params[0])->kick(params[1], params[2], reason);
}

void handleLoad(const std::vector<std::string> &params)
{
#if defined(WITH_LUA)
	PluginManager::instance().load(params[0]);
#else
	(void)params;
#endif
}

void handleMe(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->me(params[1], params[2]);
}

void handleMessage(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->say(params[1], params[2]);
}

void handleMode(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->mode(params[1], params[2]);
}

void handleNick(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->nick(params[1]);
}

void handleNotice(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->notice(params[1], params[2]);
}

void handlePart(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->part(params[1]);
}

void handleReload(const std::vector<std::string> &params)
{
#if defined(WITH_LUA)
	PluginManager::instance().reload(params[0]);
#else
	(void)params;
#endif
}

void handleRestart(const std::vector<std::string> &params)
{
	if (params[0] == "__ALL__") {
		ServerManager::instance().forAll([] (auto &s) {
			s->reconnect();
		});
	} else
		ServerManager::instance().get(params[0])->reconnect();
}

void handleTopic(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->topic(params[1], params[2]);
}

void handleUnload(const std::vector<std::string> &params)
{
#if defined(WITH_LUA)
	PluginManager::instance().unload(params[0]);
#else
	(void)params;
#endif
}

void handleUserMode(const std::vector<std::string> &params)
{
	ServerManager::instance().get(params[0])->umode(params[1]);
}

std::unordered_map<std::string, ClientHandler> handlers {
	{ "CNOTICE",	{ 3, 3, handleChannelNotice }	},
	{ "DISCONNECT",	{ 1, 1, handleDisconnect }	},
	{ "CONNECT",	{ 3, 6, handleConnect }		},
	{ "INVITE",	{ 3, 3, handleInvite }		},
	{ "JOIN",	{ 2, 3, handleJoin }		},
	{ "KICK",	{ 3, 4, handleKick }		},
	{ "LOAD",	{ 1, 1, handleLoad }		},
	{ "ME",		{ 3, 3, handleMe }		},
	{ "MSG",	{ 3, 3, handleMessage }		},
	{ "MODE",	{ 3, 3, handleMode }		},
	{ "NICK",	{ 2, 2, handleNick }		},
	{ "NOTICE",	{ 3, 3, handleNotice }		},
	{ "PART",	{ 2, 2, handlePart }		},
	{ "RELOAD",	{ 1, 1, handleReload }		},
	{ "RESTART",	{ 1, 1, handleRestart }		},
	{ "TOPIC",	{ 3, 3, handleTopic }		},
	{ "UMODE",	{ 2, 2, handleUserMode }	},
	{ "UNLOAD",	{ 1, 1, handleUnload }		}
};

} // !namespace

void Listener::clientAdd(Socket &server)
{
	try {
		auto client = server.accept();

		// Add to clients to read data
#if 0
		m_streamClients[client] = Message();
#endif
		m_listener.add(client);
	} catch (const std::exception &ex) {
		Logger::warn("listener: could not accept client: %s", ex.what());
	}
}

void Listener::clientRead(Socket &client)
{
	bool removeIt = false;

	/*
	 * First, read what is available and execute the command
	 * even if the client has disconnected.
	 */
	try {
		char data[128 + 1];
		auto length = client.recv(data, sizeof (data) - 1);

		// Disconnection?
		if (length == 0)
			removeIt = true;
		else {
			std::string ret;

			data[length] = '\0';

			if (m_streamClients[client].isFinished(data, ret))
				execute(ret, client);
		}
	} catch (const std::exception &) {
		removeIt = true;
	}

	if (removeIt) {
		m_streamClients.erase(client);
		m_listener.remove(client);
	}
}

void Listener::peerRead(Socket &s)
{
	try {
		SocketAddress addr;
		std::string ret;
		char data[128 + 1];

		auto length = s.recvfrom(data, sizeof (data) - 1, addr);

		data[length] = '\0';

		// If no client, create first
#if 0
		if (m_dgramClients.find(addr) == m_dgramClients.end())
			m_dgramClients[addr] = Message();
#endif

		if (m_dgramClients[addr].isFinished(data, ret)) {
			execute(ret, s, addr);

			// Clear the message buffer
			m_dgramClients.erase(addr);
#if 0
			m_dgramClients[addr] = Message();
#endif
		}
	} catch (const SocketError &ex) {
		Logger::warn("listener: could not read %s", ex.what());
	}
}

void Listener::execute(const std::string &cmd, Socket s, const SocketAddress &addr)
{
	auto cmdDelim = cmd.find_first_of(" \t");

	if (cmdDelim != std::string::npos) {
		auto cmdName = cmd.substr(0, cmdDelim);

		if (handlers.find(cmdName) == handlers.end())
			Logger::warn("listener: invalid command %s", cmdName.c_str());
		else {
			std::string result = "OK\n";
			auto h = handlers[cmdName];

			try {
				auto lineArgs = cmd.substr(cmdDelim + 1);
				auto params = Util::split(lineArgs, " \t", h.m_nosplit);

				/*
				 * Check the number of args needed.
				 */
				if (params.size() < static_cast<size_t>(h.m_noargs)) {
					std::ostringstream oss;

					oss << cmdName << " requires at least ";
					oss << handlers[cmdName].m_noargs << "\n";
					result = oss.str();
				} else
					h.m_function(params);
			} catch (const std::exception &ex) {
				result = ex.what() + std::string("\n");
			}

			notifySocket(result, s, addr);
		}
	}
}

void Listener::notifySocket(const std::string &message, Socket s, const SocketAddress &addr)
{
	if (s.getType() == SOCK_STREAM)
		s.send(message.c_str(), message.length());
	else
		s.sendto(message.c_str(), message.length(), addr);
}

void Listener::add(Socket s)
{
	// On success add to listener and servers
	m_socketServers.push_back(s);
	m_listener.add(s);
}

int Listener::count()
{
	return m_listener.size();
}

void Listener::process()
{
	try {
		auto s = m_listener.select(1, 0);

		/*
		 * For stream based server add a client and wait for its data,
		 * otherwise, read the UDP socket and try to execute it.
		 */
		if (s.getType() == SOCK_STREAM) {
			auto i = std::find(m_socketServers.begin(), m_socketServers.end(), s);

			if (i != m_socketServers.end())
				clientAdd(s);
			else
				clientRead(s);
		} else
			peerRead(s);
	} catch (const SocketError &ex) {
		if (Irccd::instance().isRunning())
			Logger::warn("listener: socket error %s", ex.what());
	} catch (const SocketTimeout &) { }
}

void Listener::close()
{
	for (auto s : m_socketServers)
		s.close();
}

} // !irccd
