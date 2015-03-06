/*
 * Server.cpp -- a IRC server to connect to
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
#include <iostream>
#include <map>
#include <utility>

#include <common/Logger.h>
#include <common/Util.h>

#include "Irccd.h"
#include "Server.h"
#include "System.h"

#include "server/Disconnected.h"
#include "server/Uninitialized.h"

#include "command/ChannelNotice.h"
#include "command/Invite.h"
#include "command/Join.h"
#include "command/Kick.h"
#include "command/Message.h"
#include "command/Me.h"
#include "command/Mode.h"
#include "command/Names.h"
#include "command/Nick.h"
#include "command/Notice.h"
#include "command/Part.h"
#include "command/Send.h"
#include "command/Topic.h"
#include "command/UserMode.h"
#include "command/Whois.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#endif

using namespace irccd::command;
using namespace irccd::state;

namespace irccd {

Server::Channel Server::toChannel(const std::string &line)
{
	Channel c;

	// detect an optional channel password
	auto colon = line.find_first_of(':');
	if (colon != std::string::npos) {
		c.name = line.substr(0, colon);
		c.password = line.substr(colon + 1);
	} else {
		c.name = line;
		c.password = "";
	}

	return c;
}

void Server::routine()
{
	while (m_running && m_state) {
		m_state->exec(*this);

		Lock lk(m_lock);

		if (m_nextState)
			m_state = std::move(m_nextState);
	}
}

Server::Server(Info info, Identity identity, RetryInfo reco, unsigned options)
	: m_state(std::make_unique<Uninitialized>())
	, m_info(std::move(info))
	, m_identity(std::move(identity))
	, m_reco(std::move(reco))
	, m_options(options)
{
}

Server::~Server()
{
	try {
		m_thread.join();
	} catch (const std::exception &ex) {
		Logger::warn("server %s: %s", m_info.name.c_str(), ex.what());
	}

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
		auto key = static_cast<IrcChanNickMode>(table[i].first);
		auto value = table[i].second;

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

bool Server::isDead() const
{
	return !m_running;
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
	Lock lk(m_lock);

	if (!hasChannel(channel.name))
		m_info.channels.push_back(channel);
}

bool Server::hasChannel(const std::string &name)
{
	Lock lk(m_lock);

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

void Server::start()
{
	Lock lk(m_lock);

	m_thread = std::thread(&Server::routine, this);
}

void Server::reconnect()
{
	Lock lk(m_lock);

	m_nextState = std::make_unique<Disconnected>();
	m_session.disconnect();
}

void Server::stop()
{
	// Notify the thread that we are stopping the server.
	Lock lk(m_lock);

	// Be sure that it won't try again
	m_running = false;
	m_reco.enabled = false;
	m_reco.stopping = true;
	m_nextState = nullptr;
	m_session.disconnect();
}

void Server::cnotice(const std::string &channel, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<ChannelNotice>(shared_from_this(), channel, message);
}

void Server::invite(const std::string &target, const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Invite>(shared_from_this(), target, channel);
}

void Server::join(const std::string &name, const std::string &password)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Join>(shared_from_this(), name, password);
}

void Server::kick(const std::string &name, const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Kick>(shared_from_this(), name, channel, reason);
}

void Server::me(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Me>(shared_from_this(), target, message);
}

void Server::mode(const std::string &channel, const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Mode>(shared_from_this(), channel, mode);
}

void Server::names(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Names>(shared_from_this(), channel);
}

void Server::nick(const std::string &nick)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Nick>(shared_from_this(), nick);
}

void Server::notice(const std::string &nickname, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running" && nickname[0] != '#')
		m_queue.add<Notice>(shared_from_this(), nickname, message);
}

void Server::part(const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Part>(shared_from_this(), channel, reason);
}

void Server::query(const std::string &who, const std::string &message)
{
	Lock lk(m_lock);

	// Do not write to public channel
	if (m_state->which() == "Running" && who[0] != '#')
		m_queue.add<Message>(shared_from_this(), who, message);
}

void Server::say(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Message>(shared_from_this(), target, message);
}

void Server::send(const std::string &msg)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Send>(shared_from_this(), msg);
}

void Server::topic(const std::string &channel, const std::string &topic)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Topic>(shared_from_this(), channel, topic);
}

void Server::umode(const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<UserMode>(shared_from_this(), mode);
}

void Server::whois(const std::string &target)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_queue.add<Whois>(shared_from_this(), target);
}

} // !irccd
