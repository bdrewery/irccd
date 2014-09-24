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

#include <functional>
#include <cstring>

#include "Logger.h"
#include "IrcSession.h"
#include "Server.h"
#include "Util.h"

#if defined(WITH_LUA)
#  include "Plugin.h"
#  include "EventQueue.h"

#  include "event/Connect.h"
#  include "event/ChannelNotice.h"
#  include "event/Message.h"
#  include "event/Me.h"
#  include "event/Invite.h"
#  include "event/Join.h"
#  include "event/Kick.h"
#  include "event/Mode.h"
#  include "event/Nick.h"
#  include "event/Notice.h"
#  include "event/Names.h"
#  include "event/Whois.h"
#  include "event/Part.h"
#  include "event/Query.h"
#  include "event/Topic.h"
#  include "event/UserMode.h"
#endif

using namespace std::placeholders;

using namespace irccd::event;

namespace irccd {

namespace {

inline bool isMe(const std::shared_ptr<Server> &s, const std::string &target)
{
	char tmp[32];
	auto &identity = s->identity();

	std::memset(tmp, 0, sizeof (tmp));
	irc_target_get_nick(target.c_str(), tmp, sizeof (tmp) -1);

	return identity.nickname == tmp;
}

inline std::string strify(const char *t)
{
	return t == nullptr ? std::string("") : t;
}

void handleChannel(irc_session_t *session,
		   const char *,
		   const char *orig,
		   const char **params,
		   unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Message>(IrcSession::toServer(session),
	    strify(params[0]), strify(orig), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleChannelNotice(irc_session_t *session,
			 const char *,
			 const char *orig,
			 const char **params,
			 unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<ChannelNotice>(IrcSession::toServer(session),
	    strify(orig), strify(params[0]), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleConnect(irc_session_t *session,
		   const char *,
		   const char *,
		   const char **,
		   unsigned int)
{
	auto s = IrcSession::toServer(session);
	const auto &info = s->info();

	// Reset the noretried counter
	s->reco().noretried = 0;

	Logger::log("server %s: successfully connected", info.name.c_str());

	// Auto join channels
	for (const auto &c : s->channels()) {
		Logger::log("server %s: autojoining channel %s",
		    info.name.c_str(), c.name.c_str());

		s->join(c.name, c.password);
	}

#if defined(WITH_LUA)
	EventQueue::instance().add<Connect>(s);
#endif
}

void handleCtcpAction(irc_session_t *session,
		      const char *,
		      const char *orig,
		      const char **params,
		      unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Me>(IrcSession::toServer(session),
	    strify(params[0]), strify(orig), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleInvite(irc_session_t *session,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
	auto s = IrcSession::toServer(session);

	// if join-invite is set to true join it
	if (s->options() & Server::OptionJoinInvite)
		s->join(strify(params[1]), "");

#if defined(WITH_LUA)
	EventQueue::instance().add<Invite>(s, strify(params[1]), strify(orig));
#else
	(void)orig;
#endif
}

void handleJoin(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Join>(IrcSession::toServer(session),
	    strify(params[0]), strify(orig));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleKick(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);

	// If I was kicked, I need to remove the channel list
	if (isMe(s, params[1])) {
		s->removeChannel(params[0]);

		if (s->options() & Server::OptionAutoRejoin)
			s->join(params[0]);
	}

#if defined(WITH_LUA)
	EventQueue::instance().add<Kick>(s, strify(params[0]), strify(orig),
	    strify(params[1]), strify(params[2]));
#else
	(void)orig;
#endif
}

void handleMode(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Mode>(IrcSession::toServer(session),
	    strify(params[0]), strify(orig), strify(params[1]), strify(params[2]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleNick(irc_session_t *session,
		const char *,
		const char *orig,
		const char **params,
		unsigned int)
{
	auto s = IrcSession::toServer(session);
	auto &id = s->identity();
	auto nick = std::string(orig);

	if (isMe(s, nick))
		id.nickname = nick;

#if defined(WITH_LUA)
	EventQueue::instance().add<Nick>(s, strify(orig), strify(params[0]));
#else
	(void)params;
#endif
}

void handleNotice(irc_session_t *session,
		  const char *,
		  const char *orig,
		  const char **params,
		  unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Notice>(IrcSession::toServer(session),
	    strify(orig), strify(params[0]), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleNumeric(irc_session_t *session,
		   unsigned int event,
		   const char *,
		   const char **params,
		   unsigned int c)
{
#if defined(WITH_LUA)
	auto s = IrcSession::toServer(session);

	if (event == LIBIRC_RFC_RPL_NAMREPLY) {
		auto &list = s->nameLists();

		if (params[3] != nullptr && params[2] != nullptr) {
			auto users = Util::split(params[3], " \t");

			// The listing may add some prefixes, remove them if needed
			for (auto &u : users) {
				if (s->hasPrefix(u))
					u.erase(0, 1);

				list[params[2]].push_back(u);
			}
		}
	} else if (event == LIBIRC_RFC_RPL_ENDOFNAMES) {
		auto &list = s->nameLists();

		if (params[1] != nullptr)
			EventQueue::instance().add<Names>(s, strify(params[1]), list[params[1]]);

		// Don't forget to remove the list
		list.clear();
	}

	if (event == LIBIRC_RFC_RPL_WHOISUSER) {
		IrcWhois info;

		info.nick = params[1];
		info.user = params[2];
		info.host = params[3];
		info.realname = params[5];

		s->whoisLists()[info.nick] = info;
	} else if (event == LIBIRC_RFC_RPL_WHOISCHANNELS) {
		auto &info = s->whoisLists()[params[1]];

		// Add all channels
		for (unsigned int i = 2; i < c; ++i)
			info.channels.push_back(params[i]);
	} else if (event == LIBIRC_RFC_RPL_ENDOFWHOIS) {
		auto &info = s->whoisLists()[params[1]];

		EventQueue::instance().add<Whois>(s, info);
	}

	if (event == 5) {
		for (unsigned int i = 0; i < c; ++i) {
			if (strncmp(params[i], "PREFIX", 6) == 0) {
				s->extractPrefixes(params[i]);
				break;
			}
		}
	}
#else
	(void)session;
	(void)event;
	(void)c;
	(void)params;
#endif
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

#if defined(WITH_LUA)
	EventQueue::instance().add<Part>(s, strify(params[0]),
	    strify(orig), strify(params[1]));
#endif
}

void handleQuery(irc_session_t *session,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Query>(IrcSession::toServer(session),
	    strify(orig), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleTopic(irc_session_t *session,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<Topic>(IrcSession::toServer(session),
	    strify(params[0]), strify(orig), strify(params[1]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
}

void handleUserMode(irc_session_t *session,
		    const char *,
		    const char *orig,
		    const char **params,
		    unsigned int)
{
#if defined(WITH_LUA)
	EventQueue::instance().add<UserMode>(IrcSession::toServer(session),
	    strify(orig), strify(params[0]));
#else
	(void)session;
	(void)orig;
	(void)params;
#endif
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

} // !namespace

IrcSession::IrcSession()
	: m_handle(irc_create_session(&callbacks), irc_destroy_session)
{
}

IrcSession::operator irc_session_t *()
{
	return m_handle.get();
}

std::shared_ptr<Server> IrcSession::toServer(irc_session_t *s)
{
	return reinterpret_cast<Server *>(irc_get_ctx(s))->shared_from_this();
}

void IrcSession::connect(const std::shared_ptr<Server> server)
{
	unsigned int major, minor;

	irc_set_ctx(m_handle.get(), server.get());
	irc_get_version(&major, &minor);

	auto &info = server->info();
	auto &identity = server->identity();

	/*
	 * After some discuss with George, SSL has been fixed in newer version
	 * of libircclient. > 1.6 is needed for SSL.
	 */
	if (major >= 1 && minor > 6) {
		// SSL needs to add # front of host
		if (server->options() & Server::OptionSsl)
			info.host.insert(0, 1, '#');

		if (server->options() & Server::OptionSslNoVerify)
			irc_option_set(m_handle.get(), LIBIRC_OPTION_SSL_NO_VERIFY);
	} else {
		if (server->options() & Server::OptionSsl)
			Logger::log("server %s: SSL is only supported with libircclient > 1.6", info.name.c_str());
	}

	const char *password = nullptr;

	if (info.password.length() > 0)
		password = info.password.c_str();

	irc_connect(m_handle.get(), info.host.c_str(), info.port, password,
	    identity.nickname.c_str(), identity.username.c_str(), identity.realname.c_str());
}

void IrcSession::run()
{
	irc_run(m_handle.get());
}

bool IrcSession::cnotice(const std::string &channel, const std::string &message)
{
	if (channel[0] == '#')
		return call(irc_cmd_notice, channel.c_str(), message.c_str());

	return true;
}

bool IrcSession::invite(const std::string &target, const std::string &channel)
{
	return call(irc_cmd_invite, target.c_str(), channel.c_str());
}

bool IrcSession::join(const std::string &channel, const std::string &password)
{
	return call(irc_cmd_join, channel.c_str(), password.c_str());
}

bool IrcSession::kick(const std::string &name, const std::string &channel, const std::string &reason)
{
	const char *r = (reason.length() == 0) ? nullptr : reason.c_str();

	return call(irc_cmd_kick, name.c_str(), channel.c_str(), r);
}

bool IrcSession::me(const std::string &target, const std::string &message)
{
	return call(irc_cmd_me, target.c_str(), message.c_str());
}

bool IrcSession::mode(const std::string &channel, const std::string &mode)
{
	return call(irc_cmd_channel_mode, channel.c_str(), mode.c_str());
}

bool IrcSession::names(const std::string &channel)
{
	return call(irc_cmd_names, channel.c_str());
}

bool IrcSession::nick(const std::string &newnick)
{
	return call(irc_cmd_nick, newnick.c_str());
}

bool IrcSession::notice(const std::string &target, const std::string &message)
{
	if (target[0] != '#')
		return call(irc_cmd_notice, target.c_str(), message.c_str());

	return true;
}

bool IrcSession::part(const std::string &channel, const std::string &reason)
{
	if (reason.length() > 0) {
		auto str = "PART " + channel + ":" + reason;

		return send(str);
	}

	return call(irc_cmd_part, channel.c_str());
}

bool IrcSession::say(const std::string &target, const std::string &message)
{
	return call(irc_cmd_msg, target.c_str(), message.c_str());
}

bool IrcSession::topic(const std::string &channel, const std::string &topic)
{
	return call(irc_cmd_topic, channel.c_str(), topic.c_str());
}

bool IrcSession::umode(const std::string &mode)
{
	return call(irc_cmd_user_mode, mode.c_str());
}

bool IrcSession::whois(const std::string &target)
{
	return call(irc_cmd_whois, target.c_str());
}

bool IrcSession::send(const std::string &raw)
{
	return call(irc_send_raw, "%s", raw.c_str());
}

void IrcSession::disconnect()
{
	irc_disconnect(m_handle.get());
}

} // !irccd
