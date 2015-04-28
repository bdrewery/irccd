/*
 * ServerEvent.cpp -- base event class for server events
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

#include <cassert>

#include "Server.h"
#include "ServerEvent.h"

namespace irccd {

ServerEvent::ServerEvent(std::string name,
			 std::string json,
			 std::shared_ptr<Server> server,
			 std::string origin,
			 std::string channel,
			 std::function<void (Plugin &p)> function)
	: m_name(std::move(name))
	, m_json(std::move(json))
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_channel(std::move(channel))
	, m_call(std::move(function))
{
	assert(m_call);
}

void ServerEvent::call(Plugin &p)
{
	m_call(p);
}

MessagePack ServerEvent::parseMessage(std::string message, Server &server, Plugin &plugin) const
{
#if defined(WITH_JS)
	std::string cc = server.settings().command;
	std::string name = plugin.info().name;
	std::string result = message;
	bool iscommand = false;

	// handle special commands "!<plugin> command"
	if (cc.length() > 0) {
		auto pos = result.find_first_of(" \t");
		auto fullcommand = cc + name;

		/*
		 * If the message that comes is "!foo" without spaces we
		 * compare the command char + the plugin name. If there
		 * is a space, we check until we find a space, if not
		 * typing "!foo123123" will trigger foo plugin.
		 */
		if (pos == std::string::npos) {
			iscommand = result == fullcommand;
		} else {
			iscommand = result.length() >= fullcommand.length() &&
			    result.compare(0, pos, fullcommand) == 0;
		}

		if (iscommand) {
			/*
			 * If no space is found we just set the message to "" otherwise
			 * the plugin name will be passed through onCommand
			 */
			if (pos == std::string::npos) {
				result = "";
			} else {
				result = message.substr(pos + 1);
			}
		}
	}

	return MessagePack(result, ((iscommand) ? MessageType::Command : MessageType::Message));
#else
	(void)message;
	(void)server;
	(void)plugin;
	return MessagePack("", MessageType::Message);
#endif
}

} // !irccd