/*
 * Server.cpp -- a IRC server to connect to
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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
			Logger::debug("server: removing %s from registry",
			    it->second->getInfo().name.c_str());
			servers.erase(it++);
		} else {
			++it;
		}
	}
}

Server::Ptr Server::toServer(irc_session_t *s)
{
	return *reinterpret_cast<Server::Ptr *>(irc_get_ctx(s));
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

	if (m_state->which() == "Running") {
		m_reco.restarting = true;
		irc_disconnect(m_session);
	}
}

void Server::stop()
{
	/*
	 * Notify the thread that we are stopping the server.
	 */
	{
		Lock lk(m_lock);

		if (m_state->which() == "Running") {
			// Be sure that it won't try again
			m_reco.enabled = false;
			irc_disconnect(m_session);
		}

		m_state = ServerState::Ptr(new ServerDead);
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

	if (m_state->which() == "Running" && channel[0] == '#')
		irc_cmd_notice(m_session, channel.c_str(), message.c_str());
}

void Server::invite(const std::string &target, const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_invite(m_session, target.c_str(), channel.c_str());
}

void Server::join(const std::string &name, const std::string &password)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_join(m_session, name.c_str(), password.c_str());
}

void Server::kick(const std::string &name, const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_kick(m_session, name.c_str(), channel.c_str(),
		    (reason.length() == 0) ? nullptr : reason.c_str());
}

void Server::me(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_me(m_session, target.c_str(), message.c_str());
}

void Server::mode(const std::string &channel, const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_channel_mode(m_session, channel.c_str(), mode.c_str());
}

void Server::names(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_names(m_session, channel.c_str());
}

void Server::nick(const std::string &nick)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_nick(m_session, nick.c_str());
}

void Server::notice(const std::string &nickname, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running" && nickname[0] != '#')
		irc_cmd_notice(m_session, nickname.c_str(), message.c_str());
}

void Server::part(const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running") {
		if (reason.length() > 0)
			irc_send_raw(m_session, "PART %s :%s", channel.c_str(), reason.c_str());
		else
			irc_cmd_part(m_session,channel.c_str());
	}
}

void Server::query(const std::string &who, const std::string &message)
{
	Lock lk(m_lock);

	// Do not write to public channel
	if (m_state->which() == "Running" && who[0] != '#')
		irc_cmd_msg(m_session, who.c_str(), message.c_str());
}

void Server::say(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_msg(m_session, target.c_str(), message.c_str());
}

void Server::sendRaw(const std::string &msg)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_send_raw(m_session, "%s", msg.c_str());
}

void Server::topic(const std::string &channel, const std::string &topic)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_topic(m_session, channel.c_str(), topic.c_str());
}

void Server::umode(const std::string &mode)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_user_mode(m_session, mode.c_str());
}

void Server::whois(const std::string &target)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_whois(m_session, target.c_str());
}

} // !irccd
