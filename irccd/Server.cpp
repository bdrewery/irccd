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

#include <Logger.h>

#include "Irccd.h"
#include "Server.h"

using namespace irccd;
using namespace std;

/* {{{ IRC handlers */

static string getNick(const char *target)
{
	char nickname[64 + 1];

	memset(nickname, 0, sizeof (nickname));
	irc_target_get_nick(target, nickname, sizeof (nickname) - 1);

	return string(nickname);
}

static void handleChannel(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string who, channel, message = "";
	const string &cmdToken = server->getCommandChar();

	channel = params[0];
	if (params[1] != nullptr)
		message = params[1];

	who = getNick(orig);

	for (Plugin *p : Irccd::getInstance()->getPlugins()) {
		/*
		 * Get the context for that plugin and try to concatenate
		 * the command token with the plugin name so we can call the good command
		 *
		 * like !ask will call onCommand for plugin ask
		 */
		string spCmd = cmdToken + p->getName();

		// handle special commands
		if (cmdToken.length() > 0 && message.compare(0, spCmd.length(), spCmd) == 0) {
			string module = message.substr(cmdToken.length(), spCmd.length() - cmdToken.length());

			if (module == p->getName())
				p->onCommand(server, channel, who, message.substr(spCmd.length()));
		} else
			p->onMessage(server, channel, who, message);
	}

	(void)ev;
	(void)count;
}

static void handleConnect(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);

	// Autojoin requested channels.
	for (Server::Channel c : server->getChannels()) {
		const char *password = nullptr;
		if (c.m_password.length() > 0)
			password = c.m_password.c_str();

		Logger::log("Autojoining channel %s on server %s",
		    c.m_name.c_str(), server->getName().c_str());

		irc_cmd_join(s, c.m_name.c_str(), password);
	}

	for (Plugin *p : Irccd::getInstance()->getPlugins())
		p->onConnect(server);

	Logger::log("Successfully connected on server %s", server->getName().c_str());

	(void)ev;
	(void)orig;
	(void)params;
	(void)count;
}

static void handleChannelNotice(irc_session_t *s, const char *ev, const char *orig,
				const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string nick, target, notice;

	nick = getNick(orig);
	target = getNick(params[0]);

	if (params[1] != nullptr)
		notice = params[1];

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onChannelNotice(server, nick, target, notice);
	}

	(void)ev;
	(void)count;
}

static void handleInvite(irc_session_t *s, const char *ev, const char *orig,
			 const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string who = getNick(orig);

	// if join-invite is set to true goes in
	if (server->getJoinInvite())
		server->join(params[1], "");

	for (Plugin *p : Irccd::getInstance()->getPlugins())
		p->onInvite(server, params[1], who);

	(void)ev;
	(void)count;
}

static void handleJoin(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string nickname = getNick(orig);

	// do not log self, XXX: add an option to allow that
	if (server->getIdentity().m_nickname != nickname) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onJoin(server, params[0], nickname);
	}

	(void)ev;
	(void)count;
}

static void handleMode(irc_session_t *s, const char *ev, const char *orig,
		       const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string nick, modeValue;

	nick = getNick(orig);

	// params[2] is optional mode argument
	if (params[2] != nullptr)
		modeValue = params[2];

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onMode(server, params[0], nick, params[1], modeValue);
	}

	(void)ev;
	(void)count;
}

static void handleNick(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string oldnick, newnick;

	oldnick = getNick(orig);
	newnick = getNick(params[0]);

	// Don't forget to update our own nickname
	if (oldnick == server->getIdentity().m_nickname)
		server->getIdentity().m_nickname = newnick;

	// do not log self, XXX: add an option to allow that
	if (server->getIdentity().m_nickname != oldnick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onNick(server, oldnick, newnick);
	}

	(void)ev;
	(void)count;
}

static void handleNotice(irc_session_t *s, const char *ev, const char *orig,
			 const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string nick, target, notice;

	nick = getNick(orig);
	target = getNick(params[0]);

	if (params[1] != nullptr)
		notice = params[1];

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onNotice(server, nick, target, notice);
	}

	(void)ev;
	(void)count;
}

static void handleNumeric(irc_session_t *s, unsigned int ev, const char *orig,
			  const char **params, unsigned int count)
{
	(void)s;
	(void)ev;
	(void)orig;
	(void)params;
	(void)count;
}

static void handleQuit(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	(void)s;
	(void)ev;
	(void)orig;
	(void)params;
	(void)count;
}

static void handlePart(irc_session_t *s, const char *ev, const char *orig,
			  const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string reason = "", nick;

	nick = getNick(orig);

	// params[1] is an optional reason
	if (params[1] != nullptr)
		reason = params[1];

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onPart(server, params[0], nick, reason);
	}

	(void)ev;
	(void)count;
}

static void handleQuery(irc_session_t *s, const char *ev, const char *orig,
			const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string who, message = "";

	who = getNick(orig);
	if (params[1] != nullptr)
		message = params[1];

	if (server->getIdentity().m_nickname != who) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onQuery(server, who, message);
	}

	(void)ev;
	(void)count;
}

static void handleTopic(irc_session_t *s, const char *ev, const char *orig,
			const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string topic = "", nick;

	nick = getNick(orig);

	// params[1] is the optional new topic
	if (params[1] != nullptr)
		topic = params[1];

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onTopic(server, params[0], nick, topic);
	}

	(void)ev;
	(void)count;
}

static void handleUserMode(irc_session_t *s, const char *ev, const char *orig,
			   const char **params, unsigned int count)
{
	Server *server = (Server *)irc_get_ctx(s);
	string nick;

	nick = getNick(orig);

	if (server->getIdentity().m_nickname != nick) {
		for (Plugin *p : Irccd::getInstance()->getPlugins())
			p->onUserMode(server, nick, params[0]);
	}

	(void)ev;
	(void)count;
}

static irc_callbacks_t functions = {
	.event_channel		= handleChannel,
	.event_channel_notice	= handleChannelNotice,
	.event_connect		= handleConnect,
	.event_invite		= handleInvite,
	.event_mode		= handleMode,
	.event_nick		= handleNick,
	.event_notice		= handleNotice,
	.event_numeric		= handleNumeric,
	.event_quit		= handleQuit,
	.event_join		= handleJoin,
	.event_part		= handlePart,
	.event_privmsg		= handleQuery,
	.event_topic		= handleTopic,
	.event_umode		= handleUserMode
};

/* }}} */

Server::Server(void)
	:m_commandChar("!"), m_threadStarted(false)
{
}

Server::~Server(void)
{
	stopConnection();
}

const string & Server::getCommandChar(void) const
{
	return m_commandChar;
}

void Server::setCommandChar(const string &commandChar)
{
	m_commandChar = commandChar;
}

bool Server::getJoinInvite(void) const
{
	return m_joinInvite;
}

void Server::setJoinInvite(bool joinInvite)
{
	m_joinInvite = joinInvite;
}

const vector<Server::Channel> & Server::getChannels(void)
{
	return m_channels;
}

Identity & Server::getIdentity(void)
{
	return m_identity;
}

void Server::setIdentity(const Identity &identity)
{
	m_identity = identity;
}

const string & Server::getName(void) const
{
	return m_name;
}

const string & Server::getHost(void) const
{
	return m_host;
}

void Server::setConnection(const std::string &name, const std::string &host,
			   unsigned port, bool ssl, const std::string &password)
{
	m_name = name;
	m_host = host;
	m_port = port;
	m_password = password;

	if (ssl)
		m_host.insert(0, 1, '#');
}

void Server::addChannel(const std::string &name, const std::string &password)
{
	Channel channel;

	channel.m_name = name;
	channel.m_password = password;

	m_channels.push_back(channel);
}

void Server::startConnection(void)
{
	m_thread = thread([=] () {
		m_session = irc_create_session(&functions);
		if (m_session != nullptr) {
			const char *password = nullptr;	
			int error;

			if (m_password.length() > 0)
				password = m_password.c_str();

			irc_set_ctx(m_session, this);
			error = irc_connect(
			    m_session,
			    m_host.c_str(),
			    m_port,
			    password,
			    m_identity.m_nickname.c_str(),
			    m_identity.m_username.c_str(),
			    m_identity.m_realname.c_str());

			if (error)
				Logger::warn("failed to connect to %s: %s", m_host.c_str(),
				    irc_strerror(irc_errno(m_session)));
			irc_run(m_session);
		}
	});

	m_threadStarted = true;
}

void Server::stopConnection(void)
{
	if (m_threadStarted) {
		m_threadStarted = false;
		m_thread.detach();
	}
}

void Server::invite(const string &target, const string &channel)
{
	if (m_threadStarted)
		irc_cmd_invite(m_session, target.c_str(), channel.c_str());
}

void Server::join(const string &name, const string &password)
{
	if (m_threadStarted)
		irc_cmd_join(m_session, name.c_str(), password.c_str());
}

void Server::kick(const string &name, const string &channel, const string &reason)
{
	if (m_threadStarted)
		irc_cmd_kick(m_session, name.c_str(), channel.c_str(),
		    (reason.length() == 0) ? nullptr : reason.c_str());
}

void Server::me(const std::string &target, const std::string &message)
{
	if (m_threadStarted)
		irc_cmd_me(m_session, target.c_str(), message.c_str());
}

void Server::nick(const std::string &nick)
{
	if (m_threadStarted)
		irc_cmd_nick(m_session, nick.c_str());

	// Don't forget to change our own name
	m_identity.m_nickname = nick;
}

void Server::part(const string &channel)
{
	if (m_threadStarted)
		irc_cmd_part(m_session, channel.c_str());
}

void Server::query(const std::string &who, const std::string &message)
{
	// Do not write to public channel
	if (m_threadStarted && who[0] != '#')
		irc_cmd_msg(m_session, who.c_str(), message.c_str());
}

void Server::say(const string &target, const string &message)
{
	if (m_threadStarted)
		irc_cmd_msg(m_session, target.c_str(), message.c_str());
}

void Server::topic(const string &channel, const string &topic)
{
	if (m_threadStarted)
		irc_cmd_topic(m_session, channel.c_str(), topic.c_str());
}
