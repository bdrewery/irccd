/*
 * Server.cpp -- a IRC server to connect to
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <Logger.h>

#include "Server.h"

namespace irccd {

void Server::handleConnect(const char *, const char **) noexcept
{
	// Reset the number of tried reconnection.
	m_settings.recocurrent = 0;

	// Don't forget to notify.
	next(ServerState::Connected);
	onConnect();

	// Auto join listed channels
	for (const ServerChannel &channel : m_settings.channels) {
		Logger::info() << "server " << m_info.name << ": auto joining " << channel.name << std::endl;
		join(channel.name, channel.password);
	}
}

void Server::handleChannel(const char *orig, const char **params) noexcept
{
	onMessage(strify(orig), strify(params[0]), strify(params[1]));
}

void Server::handleChannelNotice(const char *orig, const char **params) noexcept
{
	onChannelNotice(strify(orig), strify(params[0]), strify(params[1]));
}

void Server::handleCtcpAction(const char *orig, const char **params) noexcept
{
	onMe(strify(orig), strify(params[0]), strify(params[1]));
}

void Server::handleInvite(const char *orig, const char **params) noexcept
{
	/*
	 * The libircclient says that invite contains the target nickname, it's quite
	 * uncommon to need it so it is passed as the last argument to be
	 * optional in the plugin.
	 */
	onInvite(strify(orig), strify(params[1]), strify(params[0]));
}

void Server::handleJoin(const char *orig, const char **params) noexcept
{
	onJoin(strify(orig), strify(params[0]));
}

void Server::handleKick(const char *orig, const char **params) noexcept
{
	/*
	 * Rejoin the channel if the option has been set and I was kicked.
	 */
	char target[32]{0};

	irc_target_get_nick(orig, target, sizeof (target));

	if (m_identity.nickname == target && m_settings.autorejoin) {
		join(strify(params[1]));
	}

	onKick(strify(orig), strify(params[0]), strify(params[1]), strify(params[2]));
}

void Server::handleMode(const char *orig, const char **params) noexcept
{
	onMode(strify(orig), strify(params[0]), strify(params[1]), strify(params[2]));
}

void Server::handleNick(const char *orig, const char **params) noexcept
{
	/*
	 * Update our nickname.
	 */
	char target[32]{0};

	irc_target_get_nick(orig, target, sizeof (target));

	if (m_identity.nickname == target) {
		m_identity.nickname = strify(params[0]);
	}

	onNick(strify(orig), strify(params[0]));
}

void Server::handleNotice(const char *orig, const char **params) noexcept
{
	/*
	 * As for handleInvite, the notice provides the target nickname, we discard it.
	 */
	onNotice(strify(orig), strify(params[1]));
}

void Server::handlePart(const char *orig, const char **params) noexcept
{
	onPart(strify(orig), strify(params[0]), strify(params[1]));
}

void Server::handleQuery(const char *orig, const char **params) noexcept
{
	onQuery(strify(orig), strify(params[1]));
}

void Server::handleTopic(const char *orig, const char **params) noexcept
{
	onTopic(strify(orig), strify(params[0]), strify(params[1]));
}

void Server::handleUserMode(const char *orig, const char **params) noexcept
{
	onUserMode(strify(orig), strify(params[1]));
}

Server::Server(ServerInfo info, ServerIdentity identity, ServerSettings settings)
	: m_info(std::move(info))
	, m_settings(std::move(settings))
	, m_identity(std::move(identity))
	, m_session(nullptr, nullptr)
	, m_state(ServerState::Connecting)
	, m_next(ServerState::Undefined)
{
	irc_callbacks_t callbacks;

	/*
	 * GCC 4.9.2 triggers some missing-field-initializers warnings when
	 * using uniform initialization so use a std::memset as a workaround.
	 */
	std::memset(&callbacks, 0, sizeof (irc_callbacks_t));

	/*
	 * Convert the raw pointer functions from libircclient to Server member
	 * function.
	 *
	 * While doing this, discard useless arguments.
	 */
	callbacks.event_channel = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleChannel(orig, params);
	};
	callbacks.event_channel_notice = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleChannelNotice(orig, params);
	};
	callbacks.event_connect = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleConnect(orig, params);
	};
	callbacks.event_ctcp_action = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleCtcpAction(orig, params);
	};
	callbacks.event_invite = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleInvite(orig, params);
	};
	callbacks.event_join = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleJoin(orig, params);
	};
	callbacks.event_kick = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleKick(orig, params);
	};
	callbacks.event_mode = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleMode(orig, params);
	};
	callbacks.event_nick = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleNick(orig, params);
	};
	callbacks.event_notice = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleNotice(orig, params);
	};
	callbacks.event_part = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handlePart(orig, params);
	};
	callbacks.event_privmsg = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleQuery(orig, params);
	};
	callbacks.event_topic = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleTopic(orig, params);
	};
	callbacks.event_umode = [] (auto session, auto, auto orig, auto params, auto) {
		static_cast<Server *>(irc_get_ctx(session))->handleUserMode(orig, params);
	};

	m_session = Session(irc_create_session(&callbacks), irc_destroy_session);

	// Save this to the session
	irc_set_ctx(m_session.get(), this);
}

Server::~Server()
{
	Logger::debug() << "server " << m_info.name << ": disconnecting..." << std::endl;

	irc_disconnect(m_session.get());
}

void Server::sync(fd_set &setinput, fd_set &setoutput) noexcept
{
	/* 1. Send maximum of command possible if available for write */
	/*
	 * Break on the first failure to avoid changing the order of the
	 * commands if any of them fails.
	 */
	bool done = false;

	while (!m_queue.empty() && !done) {
		if (m_queue.front()()) {
			m_queue.pop();
		} else {
			done = true;
		}
	}

	/* 2. Read data */
	irc_process_select_descriptors(m_session.get(), &setinput, &setoutput);
}

#if 0

Server::Channel Server::toChannel(const std::string &line)
{
	Channel c;

	// detect an optional channel password
	auto colon = line.find_first_of(':');
	if (colon != std::string::npos) {
		c.name = line.substr(0, colon);
		c.password = line.substr(colon + 1);
	} else {
		c.name = line;
		c.password = "";
	}

	return c;
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
		auto key = static_cast<IrcChanNickMode>(table[i].first);
		auto value = table[i].second;

		m_info.prefixes[key] = value;
	}
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

#endif

} // !irccd
