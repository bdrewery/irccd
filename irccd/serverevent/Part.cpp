/*
 * Part.cpp -- on channel parts
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
#include <Plugin.h>

#include "Part.h"

namespace irccd {

namespace event {

Part::Part(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
	: ServerEvent(server->info().name, channel)
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_channel(std::move(channel))
	, m_reason(std::move(reason))
{
}

void Part::call(Plugin &p) const
{
	p.onPart(m_server, m_origin, m_channel, m_reason);
}

std::string Part::name(Plugin &) const
{
	return "onPart";
}

std::string Part::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Part\","
	    << "\"server\":\"" << m_server->info().name << "\","
	    << "\"origin\":\"" << m_origin << "\","
	    << "\"channel\":\"" << m_channel << "\","
	    << "\"reason\":\"" << m_reason << "\""
	    << "}";

	return oss.str();
}

std::string Part::ident() const
{
	return "Part:" + m_server->info().name + ":" + m_origin + ":" + m_channel + ":" + m_reason;
}

} // !event

} // !irccd
