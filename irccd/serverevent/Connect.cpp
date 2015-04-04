/*
 * Connect.cpp -- on connection
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

#include "Connect.h"

namespace irccd {

namespace event {

Connect::Connect(std::shared_ptr<Server> server)
	: m_server(std::move(server))
{
}

void Connect::call(Plugin &p) const
{
	p.onConnect(m_server);
}

std::string Connect::name(Plugin &) const
{
	return "onConnect";
}

std::string Connect::toJson() const
{
	std::ostringstream oss;

	oss << "{"
	    << "\"event\":\"connect\","
	    << "\"server\":\"" << m_server->info().name << "\""
	    << "}";

	return oss.str();
}

std::string Connect::ident() const
{
	return "Connect:" + m_server->info().name;
}

} // !event

} // !irccd
