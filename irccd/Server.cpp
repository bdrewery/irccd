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

#include <libirc_rfcnumeric.h>

#include <Logger.h>
#include <System.h>
#include <Util.h>

#include "Irccd.h"
#include "Server.h"

#include "server/ServerDead.h"
#include "server/ServerConnecting.h"
#include "server/ServerUninitialized.h"

namespace irccd {

/* {{{ IRC handlers */

namespace {

#if defined(WITH_LUA)
#  define handlePlugin(event) Plugin::handleIrcEvent(event)
#else
#  define handlePlugin(event)
#endif

void handleChannel(irc_session_t *s,
		   const char *,
		   const char *orig,
		   const char **params,
		   unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back((orig == nullptr) ? "" : orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Message, evparams, server)
	);
}

void handleChannelNotice(irc_session_t *s,
			 const char *,
			 const char *orig,
			 const char **params,
			 unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back((orig == nullptr) ? "" : orig);
	evparams.push_back(params[0]);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::ChannelNotice, evparams, server)
	);
}

void handleConnect(irc_session_t *s,
		   const char *,
		   const char *,
		   const char **,
		   unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;
	Server::Info &info = server->getInfo();

	Logger::log("server %s: successfully connected", info.name.c_str());

	// Auto join channels
	for (auto c : server->getChannels()) {
		Logger::log("server %s: autojoining channel %s",
		    info.name.c_str(), c.name.c_str());

		server->join(c.name, c.password);
	}

	handlePlugin(
	    IrcEvent(IrcEventType::Connection, evparams, server)
	);
}

void handleCtcpAction(irc_session_t *s,
		      const char *,
		      const char *orig,
		      const char **params,
		      unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);
	evparams.push_back(params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Me, evparams, server)
	);
}

void handleInvite(irc_session_t *s,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	// if join-invite is set to true join it
	if (server->getOptions().joinInvite)
		server->join(params[0], "");

	evparams.push_back(params[1]);
	evparams.push_back(orig);

	handlePlugin(
	    IrcEvent(IrcEventType::Invite, evparams, server)
	);
}

void handleJoin(irc_session_t *s,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);

	handlePlugin(
	    IrcEvent(IrcEventType::Join, evparams, server)
	);
}

void handleKick(irc_session_t *s,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	// If I was kicked, I need to remove the channel list
	if (server->getIdentity().nickname == params[1])
		server->removeChannel(params[0]);

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back(params[1]);
	evparams.push_back((params[2] == nullptr) ? "" : params[2]);

	handlePlugin(
	    IrcEvent(IrcEventType::Kick, evparams, server)
	);
}

void handleMode(irc_session_t *s,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back(params[1]);
	evparams.push_back((params[2] == nullptr) ? "" : params[2]);

	handlePlugin(
	    IrcEvent(IrcEventType::Mode, evparams, server)
	);
}

void handleNick(irc_session_t *s,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	Server::Identity &id = server->getIdentity();
	IrcEventParams evparams;

	if (id.nickname == std::string(orig))
		id.nickname = std::string(orig);

	evparams.push_back(orig);
	evparams.push_back(params[0]);

	handlePlugin(
	    IrcEvent(IrcEventType::Nick, evparams, server)
	);
}

void handleNotice(irc_session_t *s,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Notice, evparams, server)
	);
}

void handleNumeric(irc_session_t *s,
		   unsigned int event,
		   const char *,
		   const char **params,
		   unsigned int c)
{
	Server::Ptr server = Server::toServer(s);

	if (event == LIBIRC_RFC_RPL_NAMREPLY) {
		Server::NameList &list = server->getNameLists();

		if (params[3] != nullptr && params[2] != nullptr) {
			std::vector<std::string> users = Util::split(params[3], " \t");

			// The listing may add some prefixes, remove them if needed
			for (std::string u : users) {
				if (server->hasPrefix(u))
					u.erase(0, 1);

				list[params[2]].push_back(u);
			}
		}
	} else if (event == LIBIRC_RFC_RPL_ENDOFNAMES) {
		Server::NameList &list = server->getNameLists();

		if (params[1] != nullptr) {
			// Insert channel name at first position
			list[params[1]].insert(list[params[1]].begin(), params[1]);

			handlePlugin(
			    IrcEvent(IrcEventType::Names, list[params[1]], server)
			);
		}

		// Don't forget to remove the list
		list.clear();
	}

	if (event == LIBIRC_RFC_RPL_WHOISUSER) {
		IrcWhois info;

		info.nick = params[1];
		info.user = params[2];
		info.host = params[3];
		info.realname = params[5];

		server->getWhoisLists()[info.nick] = info;
	} else if (event == LIBIRC_RFC_RPL_WHOISCHANNELS) {
		auto &info = server->getWhoisLists()[params[1]];

		// Add all channels
		for (unsigned int i = 2; i < c; ++i)
			info.channels.push_back(params[i]);
	} else if (event == LIBIRC_RFC_RPL_ENDOFWHOIS) {
		auto &info = server->getWhoisLists()[params[1]];
		std::vector<std::string> params;

		// Convert as nick, user, host, realname, chan1, chan2, ... chanN
		params.push_back(info.nick);
		params.push_back(info.user);
		params.push_back(info.host);
		params.push_back(info.realname);

		for (size_t i = 0; i < info.channels.size(); ++i)
			params.push_back(info.channels[i]);

		handlePlugin(
		    IrcEvent(IrcEventType::Whois, params, server)
		);
	}

	/*
	 * The event 5 is usually RPL_BOUNCE, but it does not match what I'm
	 * seeing here, if someone could give me an explanation. I've also read
	 * somewhere that the event 5 is ISUPPORT. So?
	 */
	if (event == 5) {
		for (unsigned int i = 0; i < c; ++i) {
			if (strncmp(params[i], "PREFIX", 6) == 0) {
				server->extractPrefixes(params[i]);
				break;
			}
		}
	}
}

void handlePart(irc_session_t *s,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	Server::Identity id = server->getIdentity();
	std::string who;
	IrcEventParams evparams;

	if (id.nickname == who)
		server->removeChannel(params[0]);

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Part, evparams, server)
	);
}

void handleQuery(irc_session_t *s,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Query, evparams, server)
	);
}

void handleTopic(irc_session_t *s,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	handlePlugin(
	    IrcEvent(IrcEventType::Topic, evparams, server)
	);
}

void handleUserMode(irc_session_t *s,
		    const char *,
		    const char *orig,
		    const char **params,
		    unsigned int)
{
	Server::Ptr server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);

	handlePlugin(
	    IrcEvent(IrcEventType::UserMode, evparams, server)
	);
}

irc_callbacks_t createHandlers()
{
	irc_callbacks_t callbacks;

	memset(&callbacks, 0, sizeof (irc_callbacks_t));

	callbacks.event_channel		= handleChannel;
	callbacks.event_channel_notice	= handleChannelNotice;
	callbacks.event_connect		= handleConnect;
	callbacks.event_ctcp_action	= handleCtcpAction;
	callbacks.event_invite		= handleInvite;
	callbacks.event_join		= handleJoin;
	callbacks.event_kick		= handleKick;
	callbacks.event_mode		= handleMode;
	callbacks.event_numeric		= handleNumeric;
	callbacks.event_nick		= handleNick;
	callbacks.event_notice		= handleNotice;
	callbacks.event_part		= handlePart;
	callbacks.event_privmsg		= handleQuery;
	callbacks.event_topic		= handleTopic;
	callbacks.event_umode		= handleUserMode;

	return callbacks;
}

irc_callbacks_t callbacks = createHandlers();

}

/* }}} */

/* --------------------------------------------------------
 * IRC Events, used by Server
 * -------------------------------------------------------- */

IrcEvent::IrcEvent(IrcEventType type,
		   IrcEventParams params,
		   Server::Ptr server)
	: m_type(type)
	, m_params(params)
	, m_server(server)
{
}

/* --------------------------------------------------------
 * IRC Session wrapper
 * -------------------------------------------------------- */

void IrcDeleter::operator()(irc_session_t *s)
{
	Logger::debug("server: destroying IrcSession");

	delete reinterpret_cast<std::shared_ptr<Server> *>(irc_get_ctx(s));

	irc_destroy_session(s);
}

IrcSession::IrcSession()
{
	m_handle = Ptr(irc_create_session(&callbacks));
}

IrcSession::IrcSession(IrcSession &&other)
{
	m_handle = std::move(other.m_handle);
}

IrcSession &IrcSession::operator=(IrcSession &&other)
{
	m_handle = std::move(other.m_handle);

	return *this;
}

IrcSession::operator irc_session_t *()
{
	return m_handle.get();
}

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
		Logger::warn("%d vs %d\n", std::this_thread::get_id(), m_thread.get_id());
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

void Server::part(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_state->which() == "Running")
		irc_cmd_part(m_session, channel.c_str());
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
