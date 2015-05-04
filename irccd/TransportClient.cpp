/*
 * TransportClient.cpp -- client connected to irccd
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

#include <functional>

#include <Logger.h>

#include "SocketListener.h"
#include "TransportClient.h"

namespace irccd {

void TransportClientAbstract::receive()
{
	std::string incoming = socket().recv(512);

	if (incoming.size() == 0) {
		throw std::invalid_argument("client disconnected");
	}

	m_input += incoming;
}

void TransportClientAbstract::send()
{
	m_output.erase(0, socket().send(m_output));
}

/*
 * Send a channel notice
 * --------------------------------------------------------
 *
 * Send a channel notice to the specified channel.
 *
 * {
 *   "command": "cnotice",
 *   "server: "the server name",
 *   "channel": "name",
 *   "message": "the message"
 * }
 */
void TransportClientAbstract::parseChannelNotice(const JsonObject &object) const
{
	onChannelNotice(
		value(object, "server").toString(),
		value(object, "channel").toString(),
		value(object, "message").toString()
	);
}

/*
 * Connect to a server
 * --------------------------------------------------------
 *
 * Connect to a server.
 *
 * There are no default argument, everything must be set.
 *
 * {
 *   "command": "connect",
 *   "name": "server ident",
 *   "host": "server host",
 *   "port": 6667,
 *   "ssl": true,
 *   "ssl-verify": true
 * }
 *
 * Responses:
 *   - Error if a server with that name already exists
 */
void TransportClientAbstract::parseConnect(const JsonObject &) const
{
	// TODO
}

/*
 * Disconnect a server
 * --------------------------------------------------------
 *
 * Disconnect from a server.
 *
 * {
 *   "command": "disconnect",
 *   "server: "the server name"
 * }
 *
 * Responses:
 *   - Error if the server does not exist
 */
void TransportClientAbstract::parseDisconnect(const JsonObject &object) const
{
	onDisconnect(valueOr(object, "server", "").toString());
}

/*
 * Invite someone
 * --------------------------------------------------------
 *
 * Invite someone to the specified channel.
 *
 * {
 *   "command": "invite",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "channel": "the channel"
 * }
 */
void TransportClientAbstract::parseInvite(const JsonObject &object) const
{
	onInvite(
		value(object, "server").toString(),
		value(object, "target").toString(),
		value(object, "channel").toString()
	);
}

/*
 * Join a channel
 * --------------------------------------------------------
 *
 * Join a channel, you may add an optional password.
 *
 * {
 *   "command": "join",
 *   "server: "the server name",
 *   "channel": "channel name",
 *   "password": "the password"		(Optional)
 * }
 */
void TransportClientAbstract::parseJoin(const JsonObject &object) const
{
	onInvite(
		value(object, "server").toString(),
		value(object, "channel").toString(),
		valueOr(object, "password", "").toString()
	);
}

/*
 * Kick someone from a channel
 * --------------------------------------------------------
 *
 * Kick a target from a channel.
 *
 * {
 *   "command": "kick",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "channel": "the channel",
 *   "reason": "the optional reason"	(Optional)
 * }
 */
void TransportClientAbstract::parseKick(const JsonObject &object) const
{
	onInvite(
		value(object, "server").toString(),
		value(object, "target").toString(),
		value(object, "channel").toString(),
		valueOr(object, "reason", "").toString()
	);
}

/*
 * Load a plugin.
 * --------------------------------------------------------
 *
 * Load a plugin not already loaded by Irccd. This function only works
 * with relative paths as the client may connect from a different machine.
 *
 * {
 *   "command": "load",
 *   "plugin": "name"
 * }
 *
 * Responses:
 *   - Error if the plugin failed to load or was not found
 */
void TransportClientAbstract::parseLoad(const JsonObject &object) const
{
	onLoad(value(object, "plugin").toString());
}

/*
 * Send a CTCP Action
 * --------------------------------------------------------
 *
 * Send a CTCP action knows as /me.
 *
 * {
 *   "command": "me",
 *   "server: "the server name",
 *   "channel": "the channel",
 *   "message": "the message"		(Optional)
 * }
 */
void TransportClientAbstract::parseMe(const JsonObject &object) const
{
	onMe(
		value(object, "server").toString(),
		value(object, "channel").toString(),
		valueOr(object, "message", "").toString()
	);
}

/*
 * Say something to a target
 * --------------------------------------------------------
 *
 * Send a message to a target which can be a nickname or a channel.
 *
 * {
 *   "command": "say",
 *   "server: "the server name",
 *   "target": "channel or nickname",
 *   "message": "The message"
 * }
 */
void TransportClientAbstract::parseMessage(const JsonObject &object) const
{
	onMessage(
		value(object, "server").toString(),
		value(object, "target").toString(),
		valueOr(object, "message", "").toString()
	);
}

/*
 * Change the channel mode
 * --------------------------------------------------------
 *
 * Change the channel mode.
 *
 * {
 *   "command": "mode",
 *   "server: "the server name",
 *   "channel": "channel",
 *   "mode": "mode and its arguments"
 * }
 */
void TransportClientAbstract::parseMode(const JsonObject &) const
{
}

/*
 * Change the bot nickname
 * --------------------------------------------------------
 *
 * Change the daemon nickname.
 *
 * {
 *   "command": "nick",
 *   "server: "the server name",
 *   "nickname": "the new nickname"
 * }
 */
void TransportClientAbstract::parseNick(const JsonObject &) const
{
}

/*
 * Send a notice
 * --------------------------------------------------------
 *
 * Send a notice to a target.
 *
 * {
 *   "command": "notice",
 *   "server: "the server name",
 *   "target": "the nickname",
 *   "message": "the message"
 * }
 */
void TransportClientAbstract::parseNotice(const JsonObject &) const
{
}

/*
 * Part from a channel
 * --------------------------------------------------------
 *
 * Leaves a channel. May add an optional reason but it does not work
 * for every servers.
 *
 * {
 *   "command": "part",
 *   "server: "the server name",
 *   "channel": "the channel name",
 *   "reason": "the reason"		(Optional)
 * }
 */
void TransportClientAbstract::parsePart(const JsonObject &) const
{
}

/*
 * Force reconnection of a server
 * --------------------------------------------------------
 *
 * May be used to force the reconnexion of a server if Irccd didn't catch
 * the disconnection.
 *
 * If no server has been specified, all servers are marked for reconnection.
 *
 * {
 *   "command": "reconnect",
 *   "server": "the server name",	(Optional)
 * }
 *
 * Responses:
 *   - Error if the server does not exist
 */
void TransportClientAbstract::parseReconnect(const JsonObject &) const
{
}

/*
 * Reload a plugin
 * --------------------------------------------------------
 *
 * Reload the plugin by name, invokes the onReload function, does not unload and
 * load the plugin.
 *
 * {
 *   "command": "reload",
 *   "plugin": "crazygame"
 * }
 *
 * Responses:
 *   - Error if the plugin does not exists
 */
void TransportClientAbstract::parseReload(const JsonObject &) const
{
}

/*
 * Change a channel topic
 * --------------------------------------------------------
 *
 * Change the topic on the specified channel.
 *
 * {
 *   "command": "topic",
 *   "server: "the server name",
 *   "channel": "the channel name",
 *   "topic": "the new topic"
 * }
 */
void TransportClientAbstract::parseTopic(const JsonObject &) const
{
}

/*
 * Unload a plugin
 * --------------------------------------------------------
 *
 * Unload a plugin by its name. Also invokes the onUnload function
 * before removing it.
 *
 * {
 *   "command": "unload",
 *   "plugin": "crazygame"
 * }
 *
 * Responses:
 *   - Error if the plugin does not exists
 */
void TransportClientAbstract::parseUnload(const JsonObject &) const
{
}

/*
 * Set the irccd user mode
 * --------------------------------------------------------
 *
 * Change your irccd user mode for the specified server.
 *
 * {
 *   "command": "umode",
 *   "server: "the server name",
 *   "mode": "the mode"
 * }
 */
void TransportClientAbstract::parseUserMode(const JsonObject &) const
{
}

void TransportClientAbstract::parse(const std::string &message) const
{
	using namespace std;
	using namespace std::placeholders;

	static std::unordered_map<std::string, std::function<void (const JsonObject &)>> parsers = {
		{ "cnotice",	bind(&TransportClientAbstract::parseChannelNotice, this, _1)	},
		{ "connect",	bind(&TransportClientAbstract::parseConnect, this, _1)		},
		{ "disconnect",	bind(&TransportClientAbstract::parseDisconnect, this, _1)	},
		{ "invite",	bind(&TransportClientAbstract::parseInvite, this, _1)		},
		{ "kick",	bind(&TransportClientAbstract::parseKick, this, _1)		},
		{ "load",	bind(&TransportClientAbstract::parseLoad, this, _1)		},
		{ "me",		bind(&TransportClientAbstract::parseMe, this, _1)		},
		{ "message",	bind(&TransportClientAbstract::parseMessage, this, _1)		},
		{ "mode",	bind(&TransportClientAbstract::parseMode, this, _1)		},
		{ "nick",	bind(&TransportClientAbstract::parseNick, this, _1)		},
		{ "notice",	bind(&TransportClientAbstract::parseNotice, this, _1)		},
		{ "part",	bind(&TransportClientAbstract::parsePart, this, _1)		},
		{ "reconnect",	bind(&TransportClientAbstract::parseReconnect, this, _1)	},
		{ "reload",	bind(&TransportClientAbstract::parseReload, this, _1)		},
		{ "topic",	bind(&TransportClientAbstract::parseTopic, this, _1)		},
		{ "unload",	bind(&TransportClientAbstract::parseUnload, this, _1)		},
		{ "umode",	bind(&TransportClientAbstract::parseUserMode, this, _1)		}
	};

	JsonDocument document(message);
	if (!document.isObject()) {
		throw std::invalid_argument("the message is not a valid JSON object");
	}

	JsonObject object = document.toObject();
	if (!object.contains("command")) {
		throw std::invalid_argument("invalid message: missing `command' property");
	}

	auto it = parsers.find(object["command"].toString());
	if (it == parsers.end()) {
		throw std::invalid_argument("invalid command: " + object["command"].toString());
	}

	it->second(object);
}

void TransportClientAbstract::process(int direction)
{
	try {
		if (direction & SocketListener::Read) {
			receive();
		}
		if (direction & SocketListener::Write) {
			send();
		}
	} catch (const std::exception &ex) {
		onDie();
	}

	std::string::size_type pos;
	while ((pos = m_input.find("\r\n\r\n")) != std::string::npos) {
		/*
		 * Make a copy and erase it in case that onComplete function
		 * throws.
		 */
		std::string message = m_input.substr(0, pos);

		m_input.erase(m_input.begin(), m_input.begin() + pos + 4);

		try {
			parse(message);
		} catch (const std::exception &ex) {
			// TODO: report error to client
			Logger::warning() << "transport: " << ex.what() << std::endl;
		}
	}
}

void TransportClientAbstract::error(std::string message, bool notify)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_output += "{";
	m_output += "\"error\":";
	m_output += "\"" + JsonValue::escape(message) + "\"";
	m_output += "}\r\n\r\n";

	if (notify) {
		onWrite();
	}
}

void TransportClientAbstract::send(std::string message, bool notify)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_output += JsonValue::escape(message);
	m_output += "\r\n\r\n";

	if (notify) {
		onWrite();
	}
}

bool TransportClientAbstract::hasOutput() const noexcept
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_output.size() > 0;
}

} // !irccd
