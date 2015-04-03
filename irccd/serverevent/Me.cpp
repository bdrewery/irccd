/*
 * Me.cpp -- on CTCP Action
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

#include "Me.h"

namespace irccd {

namespace event {

Me::Me(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message)
	: ServerEvent(server->info().name, target)
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_target(std::move(target))
	, m_message(std::move(message))
{
}

void Me::call(Plugin &p)
{
	p.onMe(std::move(m_server), std::move(m_origin), std::move(m_target), std::move(m_message));
}

const char *Me::name(Plugin &) const
{
	return "onMe";
}

std::string Me::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Me\","
	    << "\"server\":\"" << m_server->info().name << "\""
	    << "\"origin\":\"" << m_origin << "\","
	    << "\"target\":\"" << m_target << "\","
	    << "\"message\":\"" << m_message << "\""
	    << "}";

	return oss.str();
}

std::string Me::ident() const
{
	return "Me:" + m_server->info().name + ":" + m_origin + ":" + m_target + ":" + m_message;
}

} // !event

} // !irccd
