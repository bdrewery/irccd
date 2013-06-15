/*
 * Server.cpp -- a IRC server to connect to
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

#include <iostream>
#include <map>

#include <libirc_rfcnumeric.h>

#include <Logger.h>
#include <Util.h>

#include "Irccd.h"

using namespace irccd;
using namespace std;

/* {{{ IRC handlers */

static void handleChannel(irc_session_t *s,
			  const char *ev,
			  const char *orig,
			  const char **params,
			  unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Message, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleChannelNotice(irc_session_t *s,
				const char *ev,
				const char *orig,
				const char **params,
				unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::ChannelNotice, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleConnect(irc_session_t *s,
			  const char *ev,
			  const char *orig,
			  const char **params,
			  unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Connection, evparams, server)
	);

	(void)ev;
	(void)orig;
	(void)params;
	(void)count;
}

static void handleCtcpRequest(irc_session_t *s,
			      const char *ev,
			      const char *orig,
			      const char **params,
			      unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);

	// not implemented yet
	(void)server;
	(void)s;
	(void)ev;
	(void)orig;
	(void)params;
	(void)count;
}

static void handleInvite(irc_session_t *s,
			 const char *ev,
			 const char *orig,
			 const char **params,
			 unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[1]);
	evparams.push_back(orig);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Invite, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleJoin(irc_session_t *s,
		       const char *ev,
		       const char *orig,
		       const char **params,
		       unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Join, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleKick(irc_session_t *s,
		       const char *ev,
		       const char *orig,
		       const char **params,
		       unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back(params[1]);
	evparams.push_back((params[2] == nullptr) ? "" : params[2]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Kick, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleMode(irc_session_t *s,
		       const char *ev,
		       const char *orig,
		       const char **params,
		       unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back(params[1]);
	evparams.push_back((params[2] == nullptr) ? "" : params[2]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Mode, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleNick(irc_session_t *s,
		       const char *ev,
		       const char *orig,
		       const char **params,
		       unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Nick, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleNotice(irc_session_t *s,
			 const char *ev,
			 const char *orig,
			 const char **params,
			 unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Notice, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleNumeric(irc_session_t *s,
			  unsigned int event,
			  const char *origin,
			  const char **params,
			  unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	if (event == LIBIRC_RFC_RPL_NAMREPLY) {
		Server::NameList & list = server->getNameLists();

		if (params[3] != nullptr && params[2] != nullptr) {
			std::vector<string> users = Util::split(params[3], " \t");
			for (string u : users)
				list[params[2]].push_back(u);
		}
	} else if (event == LIBIRC_RFC_RPL_ENDOFNAMES) {
		Server::NameList & list = server->getNameLists();

		if (params[1] != nullptr) {
			Irccd::getInstance()->handleIrcEvent(
				IrcEvent(IrcEventType::Names, list[params[1]], server)
			);
		}
	}

	(void)origin;
	(void)count;
}

static void handlePart(irc_session_t *s,
		       const char *ev,
		       const char *orig,
		       const char **params,
		       unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Part, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleQuery(irc_session_t *s,
			const char *ev,
			const char *orig,
			const char **params,
			unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Query, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleTopic(irc_session_t *s,
			const char *ev,
			const char *orig,
			const char **params,
			unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(params[0]);
	evparams.push_back(orig);
	evparams.push_back((params[1] == nullptr) ? "" : params[1]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::Topic, evparams, server)
	);

	(void)ev;
	(void)count;
}

static void handleUserMode(irc_session_t *s,
			   const char *ev,
			   const char *orig,
			   const char **params,
			   unsigned int count)
{
	shared_ptr<Server> server = Server::toServer(s);
	IrcEventParams evparams;

	evparams.push_back(orig);
	evparams.push_back(params[0]);

	Irccd::getInstance()->handleIrcEvent(
	    IrcEvent(IrcEventType::UserMode, evparams, server)
	);

	(void)ev;
	(void)count;	
}

/* }}} */

/* --------------------------------------------------------
 * IRC Events, used by Server
 * -------------------------------------------------------- */

IrcEvent::IrcEvent()
{
}

IrcEvent::IrcEvent(IrcEventType type, IrcEventParams params, std::shared_ptr<Server> server)
	: m_type(type)
	, m_params(params)
	, m_server(server)
{
}

IrcEvent::~IrcEvent()
{
}

/* --------------------------------------------------------
 * Server
 * -------------------------------------------------------- */

shared_ptr<Server> Server::Server::toServer(irc_session_t *s)
{
	return *reinterpret_cast<shared_ptr<Server> *>(irc_get_ctx(s));
}

Server::Server()
	: m_commandChar("!")
	, m_joinInvite(false)
	, m_ctcpReply(true)
	, m_threadStarted(false)
{
	memset(&m_callbacks, 0, sizeof (irc_callbacks_t));

	m_callbacks.event_channel		= handleChannel;
	m_callbacks.event_channel_notice	= handleChannelNotice;
	m_callbacks.event_connect		= handleConnect;
	m_callbacks.event_ctcp_req		= handleCtcpRequest;
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

Server::~Server()
{
	stopConnection();
}

const string & Server::getCommandChar() const
{
	return m_commandChar;
}

void Server::setCommandChar(const string &commandChar)
{
	m_commandChar = commandChar;
}

bool Server::autoJoinInvite() const
{
	return m_joinInvite;
}

void Server::setJoinInvite(bool joinInvite)
{
	m_joinInvite = joinInvite;
}

void Server::setAutoCtcpReply(bool autoCtcpReply)
{
	m_ctcpReply = autoCtcpReply;
}

bool Server::autoCtcpReply() const
{
	return m_ctcpReply;
}

const vector<Server::Channel> & Server::getChannels(void)
{
	return m_channels;
}

Server::NameList& Server::getNameLists()
{
	return m_nameLists;
}

Identity & Server::getIdentity()
{
	return m_identity;
}

void Server::setIdentity(const Identity &identity)
{
	m_identity = identity;
}

const string & Server::getName() const
{
	return m_name;
}

const string & Server::getHost() const
{
	return m_host;
}

unsigned Server::getPort() const
{
	return m_port;
}

void Server::setConnection(const string &name,
			   const string &host,
			   unsigned port,
			   const string &password)
{
	m_name = name;
	m_host = host;
	m_port = port;
	m_password = password;
}

void Server::setSSL(bool ssl, bool sslVerify)
{
	m_ssl = ssl;
	m_sslVerify = sslVerify;
}

void Server::addChannel(const string &name, const string &password)
{
	Channel channel;

	if (!hasChannel(name)) {
		channel.m_name = name;
		channel.m_password = password;

		m_channels.push_back(channel);
	}
}

bool Server::hasChannel(const string &name)
{
	for (const Channel &c : m_channels)
		if (c.m_name == name)
			return true;

	return false;
}

void Server::removeChannel(const string &name)
{
	vector<Channel>::const_iterator iter;
	bool found = false;

	for (iter = m_channels.begin(); iter != m_channels.end(); ++iter) {
		if ((*iter).m_name == name) {
			found = true;
			break;
		}
	}

	if (found)
		m_channels.erase(iter);
}

void Server::startConnection()
{
	m_thread = thread([=] () {
		irc_session_t *s = irc_create_session(&m_callbacks);
		if (s != nullptr) {
			const char *password = nullptr;	
			int error;

			// Copy the unique pointer.
			m_session = unique_ptr<irc_session_t, IrcDeleter>(s);
			if (m_password.length() > 0)
				password = m_password.c_str();

			irc_set_ctx(m_session.get(), new shared_ptr<Server>(shared_from_this()));

			// SSL needs to add # front of host
			if (m_ssl)
				m_host.insert(0, 1, '#');

			if (!m_sslVerify)
				irc_option_set(m_session.get(),
				    LIBIRC_OPTION_SSL_NO_VERIFY);
			
			error = irc_connect(
			    m_session.get(),
			    m_host.c_str(),
			    m_port,
			    password,
			    m_identity.m_nickname.c_str(),
			    m_identity.m_username.c_str(),
			    m_identity.m_realname.c_str());

			if (error) {
				m_threadStarted = false;

				Logger::warn("server %s: failed to connect to %s: %s",
				    m_name.c_str(),
				    m_host.c_str(),
				    irc_strerror(irc_errno(m_session.get())));
			}

			irc_run(m_session.get());
		}
	});

	m_threadStarted = true;
}

void Server::stopConnection()
{
	if (m_threadStarted) {
		m_threadStarted = false;
		m_thread.detach();
	}
}

void Server::cnotice(const string &channel, const string &message)
{
	if (m_threadStarted && channel[0] == '#')
		irc_cmd_notice(m_session.get(), channel.c_str(), message.c_str());
}

void Server::invite(const string &target, const string &channel)
{
	if (m_threadStarted)
		irc_cmd_invite(m_session.get(), target.c_str(), channel.c_str());
}

void Server::join(const string &name, const string &password)
{
	if (m_threadStarted) {
		Channel c;

		c.m_name = name;
		c.m_password = password;

		irc_cmd_join(m_session.get(), name.c_str(), password.c_str());
		addChannel(name, password);
	}
}

void Server::kick(const string &name, const string &channel, const string &reason)
{
	if (m_threadStarted)
		irc_cmd_kick(m_session.get(), name.c_str(), channel.c_str(),
		    (reason.length() == 0) ? nullptr : reason.c_str());
}

void Server::me(const string &target, const string &message)
{
	if (m_threadStarted)
		irc_cmd_me(m_session.get(), target.c_str(), message.c_str());
}

void Server::mode(const string &channel, const string &mode)
{
	if (m_threadStarted)
		irc_cmd_channel_mode(m_session.get(), channel.c_str(), mode.c_str());
}

void Server::names(const string &channel)
{
	if (m_threadStarted)
		irc_cmd_names(m_session.get(), channel.c_str());
}

void Server::nick(const string &nick)
{
	if (m_threadStarted)
		irc_cmd_nick(m_session.get(), nick.c_str());

	// Don't forget to change our own name
	m_identity.m_nickname = nick;
}

void Server::notice(const string &nickname, const string &message)
{
	if (m_threadStarted && nickname[0] != '#')
		irc_cmd_notice(m_session.get(), nickname.c_str(), message.c_str());
}

void Server::part(const string &channel)
{
	if (m_threadStarted) {
		irc_cmd_part(m_session.get(), channel.c_str());
		removeChannel(channel);
	}
}

void Server::query(const string &who, const string &message)
{
	// Do not write to public channel
	if (m_threadStarted && who[0] != '#')
		irc_cmd_msg(m_session.get(), who.c_str(), message.c_str());
}

void Server::say(const string &target, const string &message)
{
	if (m_threadStarted)
		irc_cmd_msg(m_session.get(), target.c_str(), message.c_str());
}

void Server::topic(const string &channel, const string &topic)
{
	if (m_threadStarted)
		irc_cmd_topic(m_session.get(), channel.c_str(), topic.c_str());
}

void Server::umode(const string &mode)
{
	if (m_threadStarted)
		irc_cmd_user_mode(m_session.get(), mode.c_str());
}
