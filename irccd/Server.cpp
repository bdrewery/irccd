/*
 * Server.cpp -- a IRC server to connect to
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

#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <utility>

#include <common/Logger.h>
#include <common/Util.h>

#include "Irccd.h"
#include "Server.h"
#include "System.h"

#include "server/ServerDead.h"
#include "server/ServerConnecting.h"
#include "server/ServerUninitialized.h"

#include "command/CommandChannelNotice.h"
#include "command/CommandInvite.h"
#include "command/CommandJoin.h"
#include "command/CommandKick.h"
#include "command/CommandMessage.h"
#include "command/CommandMe.h"
#include "command/CommandMode.h"
#include "command/CommandNames.h"
#include "command/CommandNick.h"
#include "command/CommandNotice.h"
#include "command/CommandPart.h"
#include "command/CommandSend.h"
#include "command/CommandTopic.h"
#include "command/CommandUserMode.h"
#include "command/CommandWhois.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

namespace irccd {

/* --------------------------------------------------------
 * Server
 * -------------------------------------------------------- */

Server::Map	Server::servers;
Server::Threads	Server::threads;
Server::Mutex	Server::serverLock;

void Server::add(Server::Ptr server)
{
	Lock lk(serverLock);

	auto info = server->info();

	assert(!has(info.name));

	/*
	 * If a stale thread was here from an old dead server, join the
	 * thread before adding a new one.
	 */
	if (threads.count(info.name) > 0) {
		Logger::debug("server %s: removing stale thread", info.name.c_str());

		try {
			threads[info.name].join();
		} catch (...) { }
	}

	servers[info.name] = server;

	auto thread = std::thread([=] () {
		while (server->m_state) {
			auto next = server->m_state->exec(server);

			Lock lk(server->m_lock);
			server->m_state = std::move(next);
		}
	});

	threads[info.name] = std::move(thread);
}

void Server::remove(Server::Ptr server)
{
	Lock lk(serverLock);

	auto &info = server->info();

	servers.erase(info.name);

#if defined(WITH_LUA)
	/*
	 * Some server objects may still live in the Lua registry. Remove them
	 * now.
	 */
	Plugin::collectGarbage();
#endif
}

bool Server::has(const std::string &name)
{
	Lock lk(serverLock);

	return servers.count(name) > 0;
}

Server::Ptr Server::get(const std::string &name)
{
	Lock lk(serverLock);
	Server::Ptr sv;

	try {
		sv = servers.at(name);
	} catch (std::out_of_range) {
		throw std::out_of_range("server " + name + " not found");
	}

	return sv;
}

void Server::forAll(MapFunc func)
{
	Lock lk(serverLock);

	for (auto &s : servers)
		func(s.second);
}

void Server::clearThreads()
{
	for (auto &it : threads) {
		Logger::debug("server %s: joining thread", it.first.c_str());
		it.second.join();
	}
}

Server::Channel Server::toChannel(const std::string &line)
{
	Channel c;
	size_t colon;

	// detect an optional channel password
	colon = line.find_first_of(':');
	if (colon != std::string::npos) {
		c.name = line.substr(0, colon);
		c.password = line.substr(colon + 1);
	} else {
		c.name = line;
		c.password = "";
	}

	return c;
}

Server::Server(const Info &info,
	       const Identity &identity,
	       const RetryInfo &reco,
	       unsigned options)
	: m_state(ServerState::Ptr(new ServerUninitialized))
	, m_info(info)
	, m_identity(identity)
	, m_reco(reco)
	, m_options(options)
{
}

Server::~Server()
{
	Logger::debug("server %s: destroyed", m_info.name.c_str());
}

void Server::extractPrefixes(const std::string &line)
{
	std::pair<char, char> table[16];
	std::string buf = line.substr(7);

	for (int i = 0; i < 16; ++i)
		table[i] = std::make_pair(-1, -1);

	int j = 0;
	bool readModes = true;
	for (size_t i = 0; i < buf.size(); ++i) {
		if (buf[i] == '(')
			continue;
		if (buf[i] == ')') {
			j = 0;
			readModes = false;
			continue;
		}

		if (readModes)
			table[j++].first = buf[i];
		else
			table[j++].second = buf[i];
	}

	// Put these as a map of mode to prefix
	for (int i = 0; i < 16; ++i) {
		IrcChanNickMode key = static_cast<IrcChanNickMode>(table[i].first);
		char value = table[i].second;

		m_info.prefixes[key] = value;
	}
}

Server::NameList &Server::nameLists()
{
	return m_nameLists;
}

Server::WhoisList &Server::whoisLists()
{
	return m_whoisLists;
}

Server::Info &Server::info()
{
	return m_info;
}

Server::Identity &Server::identity()
{
	return m_identity;
}

Server::RetryInfo &Server::reco()
{
	return m_reco;
}

IrcSession &Server::session()
{
	return m_session;
}

unsigned Server::options() const
{
	return m_options;
}

const Server::ChannelList &Server::channels() const
{
	return m_info.channels;
}

void Server::addChannel(const Channel &channel)
{
	if (!hasChannel(channel.name))
		m_info.channels.push_back(channel);
}

bool Server::hasChannel(const std::string &name)
{
	for (auto c : m_info.channels)
		if (c.name == name)
			return true;

	return false;
}

bool Server::hasPrefix(const std::string &nickname) const
{
	if (nickname.length() == 0)
		return false;

	for (auto p : m_info.prefixes) {
		if (nickname[0] == p.second)
			return true;
	}

	return false;
}

void Server::removeChannel(const std::string &name)
{
	Lock lk(m_lock);

	std::vector<Channel>::iterator iter;
	bool found = false;

	for (iter = m_info.channels.begin(); iter != m_info.channels.end(); ++iter) {
		if ((*iter).name == name) {
			found = true;
			break;
		}
	}

	if (found)
		m_info.channels.erase(iter);
}

void Server::reconnect()
{
	Lock lk(m_lock);

	m_reco.restarting = true;
	m_session.disconnect();
}

void Server::kill()
{
	/*
	 * Notify the thread that we are stopping the server.
	 */
	{
		Lock lk(m_lock);

		// Be sure that it won't try again
		m_reco.enabled = false;
		m_reco.stopping = true;
		m_session.disconnect();
	}
}

void Server::clearCommands()
{
	m_queue.clear();
}

void Server::cnotice(const std::string &channel, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandChannelNotice(shared_from_this(), channel, message));
}

void Server::invite(const std::string &target, const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandInvite(shared_from_this(), target, channel));
}

void Server::join(const std::string &name, const std::string &password)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandJoin(shared_from_this(), name, password));
}

void Server::kick(const std::string &name,
		  const std::string &channel,
		  const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandKick(shared_from_this(), name, channel, reason));
}

void Server::me(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandMe(shared_from_this(), target, message));
}

void Server::mode(const std::string &channel, const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandMode(shared_from_this(), channel, mode));
}

void Server::names(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandNames(shared_from_this(), channel));
}

void Server::nick(const std::string &nick)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandNick(shared_from_this(), nick));
}

void Server::notice(const std::string &nickname, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running" && nickname[0] != '#')
		m_queue.add(CommandNotice(shared_from_this(), nickname, message));
}

void Server::part(const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandPart(shared_from_this(), channel, reason));
}

void Server::query(const std::string &who, const std::string &message)
{
	Lock lk(m_lock);

	// Do not write to public channel
	if (m_state->which() == "Running" && who[0] != '#')
		m_queue.add(CommandMessage(shared_from_this(), who, message));
}

void Server::say(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandMessage(shared_from_this(), target, message));
}

void Server::send(const std::string &msg)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandSend(shared_from_this(), msg));
}

void Server::topic(const std::string &channel, const std::string &topic)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandTopic(shared_from_this(), channel, topic));
}

void Server::umode(const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandUserMode(shared_from_this(), mode));
}

void Server::whois(const std::string &target)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add(CommandWhois(shared_from_this(), target));
}

} // !irccd
