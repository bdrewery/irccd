/*
 * Query.cpp -- on private queries
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

#include "Query.h"

namespace irccd {

namespace event {

Query::Query(std::shared_ptr<Server> server, std::string origin, std::string message)
	: m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_message(std::move(message))
{
}

void Query::call(Plugin &p) const
{
#if defined(WITH_JS)
	auto pack = parseMessage(m_message, *m_server, p);

	if (pack.second == MessageType::Message) {
		p.onQuery(m_server, m_origin, pack.first);
	} else {
		p.onQueryCommand(m_server, m_origin, pack.first);
	}
#else
	(void)p;
#endif
}

std::string Query::name(Plugin &p) const
{
	auto pack = parseMessage(m_message, *m_server, p);

	return (pack.second == MessageType::Message) ? "onQuery" : "onQueryCommand";
}

std::string Query::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Query\""
	    << "\"server\":\"" << m_server->info().name << "\","
	    << "\"origin\":\"" << m_origin << "\","
	    << "\"message\":\"" << m_message << "\""
	    << "}";

	return oss.str();
}

std::string Query::ident() const
{
	return "Query:" + m_server->info().name + ":" + m_origin + ":" + m_message;
}

} // !event

} // !irccd
