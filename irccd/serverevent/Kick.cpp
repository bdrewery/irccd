/*
 * Kick.cpp -- on channel kick
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

#include <Json.h>

#include "Kick.h"

namespace irccd {

namespace event {

Kick::Kick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
	: ServerEvent(server->info().name, channel)
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_channel(std::move(channel))
	, m_target(std::move(target))
	, m_reason(std::move(reason))
{
}

void Kick::call(Plugin &p) const
{
#if defined(WITH_JS)
	p.onKick(m_server, m_origin, m_channel, m_target, m_reason);
#else
	(void)p;
#endif
}

std::string Kick::name(Plugin &) const
{
	return "onKick";
}

std::string Kick::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Kick\","
	    << "\"server\":\"" << m_server->info().name << "\","
	    << "\"origin\":\"" << JsonValue::escape(m_origin) << "\","
	    << "\"channel\":\"" << JsonValue::escape(m_channel) << "\","
	    << "\"target\":\"" << JsonValue::escape(m_target) << "\","
	    << "\"reason\":\"" << JsonValue::escape(m_reason) << "\""
	    << "}";

	return oss.str();
}

std::string Kick::ident() const
{
	return "Kick:" + m_server->info().name + ":" + m_origin + ":" + m_channel + ":" + m_target + ":" + m_reason;
}

} // !event

} // !irccd
