/*
 * Message.cpp -- on channel messages (and commands)
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

#include <Server.h>

#include <js/Plugin.h>

#include "Message.h"

namespace irccd {

namespace event {

Message::Message(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
	: ServerEvent(server->info().name, channel)
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_channel(std::move(channel))
	, m_message(std::move(message))
{
}

void Message::call(Plugin &p) const
{
	auto pack = parseMessage(m_message, *m_server, p);

	if (pack.second == MessageType::Message) {
		p.onMessage(m_server, m_origin, m_channel, pack.first);
	} else {
		p.onCommand(m_server, m_origin, m_channel, pack.first);
	}
}

std::string Message::name(Plugin &p) const
{
	auto pack = parseMessage(m_message, *m_server, p);

	return (pack.second == MessageType::Message) ? "onMessage" : "onCommand";
}

std::string Message::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Message\","
	    << "\"server\":\"" << m_server->info().name << "\","
	    << "\"origin\":\"" << m_origin << "\","
	    << "\"channel\":\"" << m_channel << "\","
	    << "\"message\":\"" << m_message << "\""
	    << "}";

	return oss.str();
}

std::string Message::ident() const
{
	return "Message:" + m_server->info().name + ":" + m_origin + ":" + m_channel + ":" + m_message;
}

} // !event

} // !irccd
