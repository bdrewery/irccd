/*
 * IrcSession.cpp -- libircclient wrapper
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

#include <cstring>

#include "Logger.h"
#include "IrcSession.h"
#include "Server.h"
#include "Util.h"

#if defined(WITH_LUA)
#  include "Plugin.h"

#  include "event/IrcEventChannelMode.h"
#  include "event/IrcEventChannelNotice.h"
#  include "event/IrcEventConnect.h"
#  include "event/IrcEventInvite.h"
#  include "event/IrcEventJoin.h"
#  include "event/IrcEventKick.h"
#  include "event/IrcEventMe.h"
#  include "event/IrcEventMessage.h"
#  include "event/IrcEventMode.h"
#  include "event/IrcEventNames.h"
#  include "event/IrcEventNick.h"
#  include "event/IrcEventNotice.h"
#  include "event/IrcEventPart.h"
#  include "event/IrcEventQuery.h"
#  include "event/IrcEventTopic.h"
#  include "event/IrcEventWhois.h"
#endif

namespace irccd {

namespace {

#if defined(WITH_LUA)
#  define handlePlugin(event) Plugin::handleIrcEvent(event)
#else
#  define handlePlugin(event)
#endif

inline bool isMe(Server::Ptr s, const std::string &target)
{
	char tmp[32];
	auto &identity = s->getIdentity();

	std::memset(tmp, 0, sizeof (tmp));
	irc_target_get_nick(target.c_str(), tmp, sizeof (tmp) -1);

	return identity.nickname == tmp;
}

void handleChannel(irc_session_t *session,
		   const char *,
		   const char *orig,
		   const char **params,
		   unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventMessage(s, params[0], orig, (!params[1]) ? "" : params[1])
	);
}

void handleChannelNotice(irc_session_t *session,
			 const char *,
			 const char *orig,
			 const char **params,
			 unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventChannelNotice(s, params[0], orig, (!params[1]) ? "" : params[1])
	);
}

void handleConnect(irc_session_t *session,
		   const char *,
		   const char *,
		   const char **,
		   unsigned int)
{
	auto s = IrcSession::toServer(session);
	const auto &info = s->getInfo();

	Logger::log("server %s: successfully connected", info.name.c_str());

	// Auto join channels
	for (const auto &c : s->getChannels()) {
		Logger::log("server %s: autojoining channel %s",
		    info.name.c_str(), c.name.c_str());

		s->join(c.name, c.password);
	}

	handlePlugin(IrcEventConnect(s));
}

void handleCtcpAction(irc_session_t *session,
		      const char *,
		      const char *orig,
		      const char **params,
		      unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventMe(s, params[0], orig, (!params[1]) ? "" : params[1])
	);
}

void handleInvite(irc_session_t *session,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
	auto s = IrcSession::toServer(session);

	// if join-invite is set to true join it
	if (s->getOptions().joinInvite)
		s->join(params[0], "");

	handlePlugin(
	    IrcEventInvite(s, orig, params[0])
	);
}

void handleJoin(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventJoin(s, orig, params[0])
	);
}

void handleKick(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);

	// If I was kicked, I need to remove the channel list
	if (isMe(s, params[1]))
		s->removeChannel(params[0]);

	handlePlugin(
	    IrcEventKick(s, orig, params[0], params[1], (!params[2]) ? "" : params[2])
	);
}

void handleMode(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventChannelMode(s, orig, params[0], params[1], (!params[2]) ? "" : params[2])
	);
}

void handleNick(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);
	auto &id = s->getIdentity();
	auto nick = std::string(orig);

	if (isMe(s, nick))
		id.nickname = nick;

	handlePlugin(
	    IrcEventNick(s, orig, params[0])
	);
}

void handleNotice(irc_session_t *session,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventNotice(s, orig, params[0], (!params[1]) ? "" : params[1])
	);
}

void handleNumeric(irc_session_t *session,
		   unsigned int event,
		   const char *,
		   const char **params,
		   unsigned int c)
{
	auto s = IrcSession::toServer(session);

	if (event == LIBIRC_RFC_RPL_NAMREPLY) {
		Server::NameList &list = s->getNameLists();

		if (params[3] != nullptr && params[2] != nullptr) {
			std::vector<std::string> users = Util::split(params[3], " \t");

			// The listing may add some prefixes, remove them if needed
			for (std::string u : users) {
				if (s->hasPrefix(u))
					u.erase(0, 1);

				list[params[2]].push_back(u);
			}
		}
	} else if (event == LIBIRC_RFC_RPL_ENDOFNAMES) {
		Server::NameList &list = s->getNameLists();

		if (params[1] != nullptr) {
			handlePlugin(IrcEventNames(s, list[params[1]], params[1]));
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

		s->getWhoisLists()[info.nick] = info;
	} else if (event == LIBIRC_RFC_RPL_WHOISCHANNELS) {
		auto &info = s->getWhoisLists()[params[1]];

		// Add all channels
		for (unsigned int i = 2; i < c; ++i)
			info.channels.push_back(params[i]);
	} else if (event == LIBIRC_RFC_RPL_ENDOFWHOIS) {
		auto &info = s->getWhoisLists()[params[1]];

		handlePlugin(IrcEventWhois(s, info));
	}

	/*
	 * The event 5 is usually RPL_BOUNCE, but it does not match what I'm
	 * seeing here, if someone could give me an explanation. I've also read
	 * somewhere that the event 5 is ISUPPORT. So?
	 */
	if (event == 5) {
		for (unsigned int i = 0; i < c; ++i) {
			if (strncmp(params[i], "PREFIX", 6) == 0) {
				s->extractPrefixes(params[i]);
				break;
			}
		}
	}
}

void handlePart(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);

	if (isMe(s, orig))
		s->removeChannel(params[0]);

	handlePlugin(
	    IrcEventPart(s, orig, params[0], (!params[1]) ? "" : params[1])
	);
}

void handleQuery(irc_session_t *session,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventQuery(s, orig, (!params[1]) ? "" : params[1])
	);
}

void handleTopic(irc_session_t *session,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventTopic(s, orig, params[0], (!params[1]) ? "" : params[1])
	);
}

void handleUserMode(irc_session_t *session,
		    const char *,
		    const char *orig,
		    const char **params,
		    unsigned int)
{
	auto s = IrcSession::toServer(session);

	handlePlugin(
	    IrcEventMode(s, orig, params[0])
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

Server::Ptr IrcSession::toServer(irc_session_t *s)
{
	return *reinterpret_cast<Server::Ptr *>(irc_get_ctx(s));
}

void IrcSession::connect(Server::Ptr server)
{
	unsigned int major, minor;

	irc_set_ctx(m_handle.get(), new Server::Ptr(server));
	irc_get_version(&major, &minor);

	auto &info = server->getInfo();
	auto &identity = server->getIdentity();

	/*
	 * After some discuss with George, SSL has been fixed in newer version
	 * of libircclient. > 1.6 is needed for SSL.
	 */
	if (major >= 1 && minor > 6) {
		// SSL needs to add # front of host
		if (info.ssl)
			info.host.insert(0, 1, '#');

		if (!info.sslVerify)
			irc_option_set(m_handle.get(), LIBIRC_OPTION_SSL_NO_VERIFY);
	} else {
		if (info.ssl)
			Logger::log("server %s: SSL is only supported with libircclient > 1.6",
			    info.name.c_str());
	}

	const char *password = nullptr;

	if (info.password.length() > 0)
		password = info.password.c_str();

	auto res = irc_connect(
	    m_handle.get(),
	    info.host.c_str(),
	    info.port,
	    password,
	    identity.nickname.c_str(),
	    identity.username.c_str(),
	    identity.realname.c_str());

	if (res == 0)
		server->getRecoInfo().noretried = 0;
}

void IrcSession::run()
{
	irc_run(m_handle.get());
}

void IrcSession::cnotice(const std::string &channel,
			 const std::string &message)
{
	if (channel[0] == '#')
		irc_cmd_notice(m_handle.get(), channel.c_str(), message.c_str());
}

void IrcSession::invite(const std::string &target,
			const std::string &channel)
{
	irc_cmd_invite(m_handle.get(), target.c_str(), channel.c_str());
}

void IrcSession::join(const std::string &channel,
		      const std::string &password)
{
	irc_cmd_join(m_handle.get(), channel.c_str(), password.c_str());
}

void IrcSession::kick(const std::string &name,
		      const std::string &channel,
		      const std::string &reason)
{
	const char *r = (reason.length() == 0) ? nullptr : reason.c_str();

	irc_cmd_kick(m_handle.get(), name.c_str(), channel.c_str(), r);
}

void IrcSession::me(const std::string &target,
		    const std::string &message)
{
	irc_cmd_me(m_handle.get(), target.c_str(), message.c_str());
}

void IrcSession::mode(const std::string &channel,
		      const std::string &mode)
{
	irc_cmd_channel_mode(m_handle.get(), channel.c_str(), mode.c_str());
}

void IrcSession::names(const std::string &channel)
{
	irc_cmd_names(m_handle.get(), channel.c_str());
}

void IrcSession::nick(const std::string &newnick)
{
	irc_cmd_nick(m_handle.get(), newnick.c_str());
}

void IrcSession::notice(const std::string &target,
			const std::string &message)
{
	if (target[0] != '#')
		irc_cmd_notice(m_handle.get(), target.c_str(), message.c_str());
}

void IrcSession::part(const std::string &channel,
		      const std::string &reason)
{
	if (reason.length() > 0) {
		auto str = "PART " + channel + ":" + reason;

		send(str);
	} else {
		irc_cmd_part(m_handle.get(), channel.c_str());
	}
}

void IrcSession::query(const std::string &target,
		       const std::string &message)
{
	irc_cmd_msg(m_handle.get(), target.c_str(), message.c_str());
}

void IrcSession::say(const std::string &channel,
		     const std::string &message)
{
	irc_cmd_msg(m_handle.get(), channel.c_str(), message.c_str());
}

void IrcSession::topic(const std::string &channel,
		       const std::string &topic)
{
	irc_cmd_topic(m_handle.get(), channel.c_str(), topic.c_str());
}

void IrcSession::umode(const std::string &mode)
{
	irc_cmd_user_mode(m_handle.get(), mode.c_str());
}

void IrcSession::whois(const std::string &target)
{
	irc_cmd_whois(m_handle.get(), target.c_str());
}

void IrcSession::send(const std::string &raw)
{
	irc_send_raw(m_handle.get(), "%s", raw.c_str());
}

void IrcSession::disconnect()
{
	irc_disconnect(m_handle.get());
}

} // !irccd
