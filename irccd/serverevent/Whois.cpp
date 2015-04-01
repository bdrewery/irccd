/*
 * Whois.cpp -- on whois information
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

#include "Whois.h"

#include <Plugin.h>

namespace irccd {

namespace event {

Whois::Whois(std::shared_ptr<Server> server, ServerWhois info)
	: m_server(std::move(server))
	, m_info(std::move(info))
{
}

void Whois::call(Plugin &p)
{
	p.onWhois(m_server, m_info);
}

const char *Whois::name(Plugin &) const
{
	return "onWhois";
}

std::string Whois::ident() const
{
	return "Whois:" + m_server->info().name;
}

} // !event

} // !irccd
