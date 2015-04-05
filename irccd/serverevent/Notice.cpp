/*
 * Notice.cpp -- on private notices
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

#include "Notice.h"

namespace irccd {

namespace event {

Notice::Notice(std::shared_ptr<Server> server, std::string origin, std::string notice)
	: ServerEvent(server->info().name, "")
	, m_server(std::move(server))
	, m_origin(std::move(origin))
	, m_notice(std::move(notice))
{
}

void Notice::call(Plugin &p) const
{
#if defined(WITH_JS)
	p.onNotice(m_server, m_origin, m_notice);
#else
	(void)p;
#endif
}

std::string Notice::name(Plugin &) const
{
	return "onNotice";
}

std::string Notice::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"Notice\","
	    << "\"server\":\"" << m_server->info().name << "\","
	    << "\"origin\":\"" << JsonValue::escape(m_origin) << "\","
	    << "\"notice\":\"" << JsonValue::escape(m_notice) << "\""
	    << "}";

	return oss.str();
}

std::string Notice::ident() const
{
	return "Notice:" + m_server->info().name + ":" + m_origin + ":" + m_notice;
}

} // !event

} // !irccd
