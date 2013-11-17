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

#include <cstring>
#include <iostream>
#include <map>
#include <utility>

#if !defined(_WIN32)
#  include <sys/types.h>
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif

#include <libirc_rfcnumeric.h>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"

namespace irccd {

/* {{{ IRC handlers */

namespace {

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

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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

	Logger::log("server %s: successfully connected",
	    server->getInfo().m_name.c_str());

	// Auto join channels
	for (auto c : server->getChannels()) {
		Logger::log("server %s: autojoining channel %s",
		    server->getInfo().m_name.c_str(), c.m_name.c_str());

		server->join(c.m_name, c.m_password);
	}

	Plugin::handleIrcEvent(
	    IrcEvent(IrcEventType::Connection, evparams, server)
	);

	server->resetRetries();
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

	Plugin::handleIrcEvent(
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
	if (server->getOptions().m_joinInvite)
		server->join(params[0], "");

	evparams.push_back(params[1]);
	evparams.push_back(orig);

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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
	if (server->getIdentity().m_nickname == params[1])
		server->removeChannel(params[0]);

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back(params[1]);
	evparams.push_back((params[2] == nullptr) ? "" : params[2]);

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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

	if (id.m_nickname == std::string(orig))
		id.m_nickname = std::string(orig);

	evparams.push_back(orig);
	evparams.push_back(params[0]);

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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
			Plugin::handleIrcEvent(
			    IrcEvent(IrcEventType::Names, list[params[1]], server)
			);
		}

		// Don't forget to remove the list
		list.clear();
	}

	if (event == LIBIRC_RFC_RPL_WHOISUSER) {
		Server::WhoisInfo info;

		info.nick = params[1];
		info.user = params[2];
		info.host = params[3];
		info.realname = params[5];

		server->getWhoisLists()[info.nick] = info;
	} else if (event == LIBIRC_RFC_RPL_WHOISCHANNELS) {
		Server::WhoisInfo &info = server->getWhoisLists()[params[1]];

		// Add all channels
		for (unsigned int i = 2; i < c; ++i)
			info.channels.push_back(params[i]);
	} else if (event == LIBIRC_RFC_RPL_ENDOFWHOIS) {
		const Server::WhoisInfo &info = server->getWhoisLists()[params[1]];
		std::vector<std::string> params;

		// Convert as nick, user, host, realname, chan1, chan2, ... chanN
		params.push_back(info.nick);
		params.push_back(info.user);
		params.push_back(info.host);
		params.push_back(info.realname);

		for (size_t i = 0; i < info.channels.size(); ++i)
			params.push_back(info.channels[i]);

		Plugin::handleIrcEvent(
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

	printf("who part = %s\n", orig);

	if (id.m_nickname == who)
		server->removeChannel(params[0]);

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
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

	Plugin::handleIrcEvent(
	    IrcEvent(IrcEventType::UserMode, evparams, server)
	);
}

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
	delete reinterpret_cast<std::shared_ptr<Server> *>(irc_get_ctx(s));

	irc_destroy_session(s);
}

IrcSession::IrcSession(irc_session_t *s)
	: m_handle(s)
{
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
	Lock lk(serverLock);

	servers[server->getInfo().m_name] = server;
	server->startConnection();

 	Logger::log("server %s: connecting...",
 	    server->getInfo().m_name.c_str());
}

Server::Ptr Server::get(const std::string &name)
{
	Lock lk(serverLock);

	return servers.at(name);
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
	auto i(servers.begin());

	while (i != servers.end()) {
		if (i->second->m_shouldDelete)
			i = servers.erase(i);
		else
			++i;
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
		c.m_name = line.substr(0, colon);
		c.m_password = line.substr(colon + 1);
	} else {
		c.m_name = line;
		c.m_password = "";
	}

	return c;
}

Server::Server(const Info &info,
	       const Identity &identity,
	       const Options &options)
	: m_session(nullptr)
	, m_shouldDelete(false)
	, m_info(info)
	, m_identity(identity)
	, m_options(options)
{
	init();
}

Server::~Server()
{
	stopConnection();
}

void Server::init()
{
	memset(&m_callbacks, 0, sizeof (irc_callbacks_t));

	m_callbacks.event_channel		= handleChannel;
	m_callbacks.event_channel_notice	= handleChannelNotice;
	m_callbacks.event_connect		= handleConnect;
	m_callbacks.event_ctcp_action		= handleCtcpAction;
	m_callbacks.event_invite		= handleInvite;
	m_callbacks.event_join			= handleJoin;
	m_callbacks.event_kick			= handleKick;
	m_callbacks.event_mode			= handleMode;
	m_callbacks.event_numeric		= handleNumeric;
	m_callbacks.event_nick			= handleNick;
	m_callbacks.event_notice		= handleNotice;
	m_callbacks.event_part			= handlePart;
	m_callbacks.event_privmsg		= handleQuery;
	m_callbacks.event_topic			= handleTopic;
	m_callbacks.event_umode			= handleUserMode;
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

		m_info.m_prefixes[key] = value;
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

const Server::ChanList &Server::getChannels() const
{
	return m_info.m_channels;
}

void Server::addChannel(const Channel &channel)
{
	if (!hasChannel(channel.m_name))
		m_info.m_channels.push_back(channel);
}

bool Server::hasChannel(const std::string &name)
{
	for (auto c : m_info.m_channels)
		if (c.m_name == name)
			return true;

	return false;
}

bool Server::hasPrefix(const std::string &nickname) const
{
	if (nickname.length() == 0)
		return false;

	for (auto p : m_info.m_prefixes) {
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

	for (iter = m_info.m_channels.begin(); iter != m_info.m_channels.end(); ++iter) {
		if ((*iter).m_name == name) {
			found = true;
			break;
		}
	}

	if (found)
		m_info.m_channels.erase(iter);
}

void Server::startConnection()
{
	Lock lk(m_lock);

	auto command = [&] () {
		bool shouldConnect;

		/*
		 * Main thread loop
		 */
		shouldConnect = true;
		while (shouldConnect) {
			/*
			 * This is needed if irccd is started before DHCP or if
			 * DNS cache is outdated.
			 *
			 * For more information see #190
			 */
#if !defined(_WIN32)
			(void)res_init();
#endif

			irc_session_t *s = irc_create_session(&m_callbacks);
			if (s == nullptr)
				return;

			const char *password = nullptr;	
			unsigned major, minor;

			// Copy the unique pointer.
			m_session = IrcSession(s);
			if (m_info.m_password.length() > 0)
				password = m_info.m_password.c_str();

			irc_set_ctx(m_session, new Server::Ptr(shared_from_this()));
			irc_get_version(&major, &minor);

			/*
			 * After some discuss with George, SSL has been fixed in older version
			 * of libircclient. > 1.6 is needed for SSL.
			 */
			if (major >= 1 && minor > 6) {
				// SSL needs to add # front of host
				if (m_info.m_ssl)
					m_info.m_host.insert(0, 1, '#');

				if (!m_info.m_sslVerify)
					irc_option_set(m_session,
					    LIBIRC_OPTION_SSL_NO_VERIFY);
			} else {
				if (m_info.m_ssl)
					Logger::log("server %s: SSL is only supported with libircclient > 1.6",
					    m_info.m_name.c_str());
			}

			irc_connect(
			    m_session,
			    m_info.m_host.c_str(),
			    m_info.m_port,
			    password,
			    m_identity.m_nickname.c_str(),
			    m_identity.m_username.c_str(),
			    m_identity.m_realname.c_str());

			/*
			 * Run over the forever loop.
			 */
			m_threadStarted = true;
			if (irc_run(m_session)) {
				m_threadStarted = false;
				irc_disconnect(m_session);

				Logger::warn("server %s: failed to connect to %s: %s",
				    m_info.m_name.c_str(),
				    m_info.m_host.c_str(),
				    irc_strerror(irc_errno(m_session)));

				/*
				 * Value of 0 mean retry forever.
				 */
				if (m_options.m_maxretries > 0) {
					m_options.m_curretries ++;
				
					shouldConnect = m_options.m_retry &&
					    m_options.m_curretries < m_options.m_maxretries;
				}

				/*
				 * Don't show the message if the user didn't wanted
				 * the retry mechanism.
				 */
				if (!shouldConnect && m_options.m_retry) {
					Logger::warn("server %s: giving up",
					    m_info.m_name.c_str());
				} else if (shouldConnect) {
					Logger::warn("server %s: retrying in %d seconds...",
					    m_info.m_name.c_str(),
					    m_options.m_timeout);
					Util::usleep(m_options.m_timeout * 1000);
				}
			} else {
				m_threadStarted = false;
				irc_disconnect(m_session);
			}
		}

		/*
		 * Here we are in the step that the server should be destroyed.
		 */
		m_shouldDelete = true;
		m_session = nullptr;
	};

	m_thread = std::thread(command);
	m_thread.detach();
}

void Server::resetRetries()
{
	Lock lk(m_lock);

	m_options.m_curretries = 0;
}

void Server::stopConnection()
{
	Lock lk(m_lock);

	if (m_threadStarted) {
		Logger::log("server %s: disconnecting...", m_info.m_name.c_str());

		if (m_session != nullptr)
			irc_disconnect(m_session);

		m_threadStarted = false;
	}
}

void Server::cnotice(const std::string &channel, const std::string &message)
{
	Lock lk(m_lock);

	if (m_threadStarted && channel[0] == '#')
		irc_cmd_notice(m_session, channel.c_str(), message.c_str());
}

void Server::invite(const std::string &target, const std::string &channel)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_invite(m_session, target.c_str(), channel.c_str());
}

void Server::join(const std::string &name, const std::string &password)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_join(m_session, name.c_str(), password.c_str());
}

void Server::kick(const std::string &name, const std::string &channel, const std::string &reason)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_kick(m_session, name.c_str(), channel.c_str(),
		    (reason.length() == 0) ? nullptr : reason.c_str());
}

void Server::me(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_me(m_session, target.c_str(), message.c_str());
}

void Server::mode(const std::string &channel, const std::string &mode)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_channel_mode(m_session, channel.c_str(), mode.c_str());
}

void Server::names(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_names(m_session, channel.c_str());
}

void Server::nick(const std::string &nick)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_nick(m_session, nick.c_str());
}

void Server::notice(const std::string &nickname, const std::string &message)
{
	Lock lk(m_lock);

	if (m_threadStarted && nickname[0] != '#')
		irc_cmd_notice(m_session, nickname.c_str(), message.c_str());
}

void Server::part(const std::string &channel)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_part(m_session, channel.c_str());
}

void Server::query(const std::string &who, const std::string &message)
{
	Lock lk(m_lock);

	// Do not write to public channel
	if (m_threadStarted && who[0] != '#')
		irc_cmd_msg(m_session, who.c_str(), message.c_str());
}

void Server::say(const std::string &target, const std::string &message)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_msg(m_session, target.c_str(), message.c_str());
}

void Server::sendRaw(const std::string &msg)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_send_raw(m_session, "%s", msg.c_str());
}

void Server::topic(const std::string &channel, const std::string &topic)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_topic(m_session, channel.c_str(), topic.c_str());
}

void Server::umode(const std::string &mode)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_user_mode(m_session, mode.c_str());
}

void Server::whois(const std::string &target)
{
	Lock lk(m_lock);

	if (m_threadStarted)
		irc_cmd_whois(m_session, target.c_str());
}

} // !irccd
