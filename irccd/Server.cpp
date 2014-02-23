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

#include <Logger.h>
#include <System.h>
#include <Util.h>

#include "Irccd.h"
#include "Server.h"

#include "server/ServerDead.h"
#include "server/ServerConnecting.h"
#include "server/ServerUninitialized.h"

namespace irccd {

/* --------------------------------------------------------
 * Server
 * -------------------------------------------------------- */

Server::List Server::servers;
Server::Mutex Server::serverLock;

void Server::add(Server::Ptr server)
{
	auto &info = server->getInfo();

	assert(!Server::has(info.name));

	Lock lk(serverLock);

	Logger::log("server %s: connecting...", info.name.c_str());

	servers[info.name] = server;
	server->start();
}

void Server::remove(Server::Ptr server)
{
	auto info = server->getInfo();

	assert(Server::has(info.name));

	Lock lk(serverLock);
	servers.erase(info.name);
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

	for (auto s : servers)
		func(s.second);
}

void Server::flush()
{
	Lock lk(serverLock);

	for (auto it = servers.cbegin(); it != servers.cend(); ) {
		if (it->second->m_state->which() == "Dead") {
			it->second->m_session = IrcSession();
			it->second->m_state = ServerState::Ptr();

			Logger::debug("server: removing %s from registry",
			    it->second->getInfo().name.c_str());
			servers.erase(it++);
		} else {
			++it;
		}
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
	       const Options &options,
	       const RetryInfo &reco)
	: m_state(ServerState::Ptr(new ServerUninitialized))
	, m_info(info)
	, m_identity(identity)
	, m_options(options)
	, m_reco(reco)

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

Server::NameList &Server::getNameLists()
{
	return m_nameLists;
}

Server::WhoisList &Server::getWhoisLists()
{
	return m_whoisLists;
}

Server::Info &Server::getInfo()
{
	return m_info;
}

Server::Identity &Server::getIdentity()
{
	return m_identity;
}

Server::Options &Server::getOptions()
{
	return m_options;
}

Server::RetryInfo &Server::getRecoInfo()
{
	return m_reco;
}

IrcSession &Server::getSession()
{
	return m_session;
}

const Server::ChanList &Server::getChannels() const
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

void Server::start()
{
	m_thread = std::thread([=] () {
		while (m_state->which() != "Dead") {
			auto state = m_state->exec(shared_from_this());

			Lock lk(m_lock);
			m_state = std::move(state);
		}
	});
}

void Server::restart()
{
	Lock lk(m_lock);

	m_reco.restarting = true;
	m_session.disconnect();
}

void Server::stop()
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

	try {
		m_thread.join();
	} catch (std::system_error error) {
		Logger::warn("server %s: %s", m_info.name.c_str(), error.what());
	}
}

void Server::cnotice(const std::string &channel, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.cnotice(channel, message);
}

void Server::invite(const std::string &target, const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.invite(target, channel);
}

void Server::join(const std::string &name, const std::string &password)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.join(name, password);
}

void Server::kick(const std::string &name, const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.kick(name, channel, reason);
}

void Server::me(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.me(target, message);
}

void Server::mode(const std::string &channel, const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.mode(channel, mode);
}

void Server::names(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.names(channel);
}

void Server::nick(const std::string &nick)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.nick(nick);
}

void Server::notice(const std::string &nickname, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running" && nickname[0] != '#')
		m_session.notice(nickname, message);
}

void Server::part(const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.part(channel, reason);
}

void Server::query(const std::string &who, const std::string &message)
{
	Lock lk(m_lock);

	// Do not write to public channel
	if (m_state->which() == "Running" && who[0] != '#')
		m_session.query(who, message);
}

void Server::say(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.say(target, message);
}

void Server::send(const std::string &msg)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.send(msg);
}

void Server::topic(const std::string &channel, const std::string &topic)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.topic(channel, topic);
}

void Server::umode(const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.umode(mode);
}

void Server::whois(const std::string &target)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		m_session.whois(target);
}

} // !irccd
