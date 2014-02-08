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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);
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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);
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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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
	auto s = Server::toServer(session);

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

} // !irccd
