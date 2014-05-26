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
#endif

using namespace std::placeholders;

namespace irccd {

namespace {

inline bool isMe(Server::Ptr s, const std::string &target)
{
	char tmp[32];
	auto &identity = s->identity();

	std::memset(tmp, 0, sizeof (tmp));
	irc_target_get_nick(target.c_str(), tmp, sizeof (tmp) -1);

	return identity.nickname == tmp;
}

#if defined(WITH_LUA)

inline std::string strify(const char *t)
{
	return t == nullptr ? std::string("") : t;
}

#endif

void handleChannel(irc_session_t *session,
		   const char *,
		   const char *orig,
		   const char **params,
		   unsigned int)
{
#if defined(WITH_LUA)
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onMessage, _1, s, strify(params[0]),
	    strify(orig), strify(params[1]));

	EventInfo info(s->info().name, params[0], "onMessage");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onChannelNotice, _1, s, strify(orig),
	    strify(params[0]), strify(params[1]));

	EventInfo info(s->info().name, params[0], "onChannelNotice");
	EventQueue::add(call, info);
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
	EventInfo einfo(s->info().name, "", "onConnect");
	EventQueue::add(std::bind(&Plugin::onConnect, _1, s), einfo);
#endif
}

void handleCtcpAction(irc_session_t *session,
		      const char *,
		      const char *orig,
		      const char **params,
		      unsigned int)
{
#if defined(WITH_LUA)
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onMe, _1, s, strify(params[0]),
	    strify(orig), strify(params[1]));

	EventInfo info(s->info().name, params[0], "onMe");
	EventQueue::add(call, info);
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
	if (s->options() & Server::OptionSsl)
		s->join(params[0], "");

#if defined(WITH_LUA)
	auto call = std::bind(&Plugin::onInvite, _1, s, strify(params[1]), strify(orig));

	EventInfo info(s->info().name, strify(params[1]), "onInvite");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onJoin, _1, s, strify(params[0]), strify(orig));

	EventInfo info(s->info().name, params[0], "onJoin");
	EventQueue::add(call, info);
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
	auto call = std::bind(&Plugin::onKick, _1, s, strify(params[0]), strify(orig),
	    strify(params[1]), strify(params[2]));

	EventInfo info(s->info().name, strify(params[0]), "onKick");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onMode, _1, s, strify(params[0]),
	    strify(orig), strify(params[1]), strify(params[2]));

	EventInfo info(s->info().name, strify(params[0]), "onMode");
	EventQueue::add(call, info);
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
	auto call = std::bind(&Plugin::onNick, _1, s, strify(orig), strify(params[0]));

	EventInfo info(s->info().name, "", "onNick");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onNotice, _1, s, strify(orig),
	    strify(params[0]), strify(params[1]));

	EventInfo info(s->info().name, strify(params[0]), "onNotice");
	EventQueue::add(call, info);
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

		if (params[1] != nullptr) {
			auto call = std::bind(&Plugin::onNames, _1, s, strify(params[1]), list[params[1]]);

			EventInfo info(s->info().name, strify(params[1]), "onNames");
			EventQueue::add(call, info);
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

		s->whoisLists()[info.nick] = info;
	} else if (event == LIBIRC_RFC_RPL_WHOISCHANNELS) {
		auto &info = s->whoisLists()[params[1]];

		// Add all channels
		for (unsigned int i = 2; i < c; ++i)
			info.channels.push_back(params[i]);
	} else if (event == LIBIRC_RFC_RPL_ENDOFWHOIS) {
		auto &info = s->whoisLists()[params[1]];

		EventInfo einfo(s->info().name, info.nick, "onWhois");
		EventQueue::add(std::bind(&Plugin::onWhois, _1, s, info), einfo);
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
	auto call = std::bind(&Plugin::onPart, _1, s, strify(params[0]),
	    strify(orig), strify(params[1]));

	EventInfo info(s->info().name, strify(params[0]), "onPart");
	EventQueue::add(call, info);
#endif
}

void handleQuery(irc_session_t *session,
		 const char *,
		 const char *orig,
		 const char **params,
		 unsigned int)
{
#if defined(WITH_LUA)
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onQuery, _1, s, strify(orig), strify(params[1]));

	EventInfo info(s->info().name, orig, "onQuery");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onTopic, _1, s, strify(params[0]),
	    strify(orig), strify(params[1]));

	EventInfo info(s->info().name, strify(params[0]), "onTopic");
	EventQueue::add(call, info);
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
	auto s = IrcSession::toServer(session);
	auto call = std::bind(&Plugin::onUserMode, _1, s, strify(orig), strify(params[0]));

	EventInfo info(s->info().name, strify(orig), "onUserMode");
	EventQueue::add(call, info);
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

}

void IrcDeleter::operator()(irc_session_t *s)
{
	Logger::debug("server: destroying IrcSession");

	delete reinterpret_cast<Server::Ptr *>(irc_get_ctx(s));

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
			Logger::log("server %s: SSL is only supported with libircclient > 1.6",
			    info.name.c_str());
	}

	const char *password = nullptr;

	if (info.password.length() > 0)
		password = info.password.c_str();

	irc_connect(
	    m_handle.get(),
	    info.host.c_str(),
	    info.port,
	    password,
	    identity.nickname.c_str(),
	    identity.username.c_str(),
	    identity.realname.c_str());
}

void IrcSession::run()
{
	irc_run(m_handle.get());
}

bool IrcSession::cnotice(const std::string &channel,
			 const std::string &message)
{
	if (channel[0] == '#')
		return call(irc_cmd_notice, channel.c_str(), message.c_str());

	return true;
}

bool IrcSession::invite(const std::string &target,
			const std::string &channel)
{
	return call(irc_cmd_invite, target.c_str(), channel.c_str());
}

bool IrcSession::join(const std::string &channel,
		      const std::string &password)
{
	return call(irc_cmd_join, channel.c_str(), password.c_str());
}

bool IrcSession::kick(const std::string &name,
		      const std::string &channel,
		      const std::string &reason)
{
	const char *r = (reason.length() == 0) ? nullptr : reason.c_str();

	return call(irc_cmd_kick, name.c_str(), channel.c_str(), r);
}

bool IrcSession::me(const std::string &target,
		    const std::string &message)
{
	return call(irc_cmd_me, target.c_str(), message.c_str());
}

bool IrcSession::mode(const std::string &channel,
		      const std::string &mode)
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

bool IrcSession::notice(const std::string &target,
			const std::string &message)
{
	if (target[0] != '#')
		return call(irc_cmd_notice, target.c_str(), message.c_str());

	return true;
}

bool IrcSession::part(const std::string &channel,
		      const std::string &reason)
{
	if (reason.length() > 0) {
		auto str = "PART " + channel + ":" + reason;

		return send(str);
	}

	return call(irc_cmd_part, channel.c_str());
}

bool IrcSession::say(const std::string &target,
		     const std::string &message)
{
	return call(irc_cmd_msg, target.c_str(), message.c_str());
}

bool IrcSession::topic(const std::string &channel,
		       const std::string &topic)
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
