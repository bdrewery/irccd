/*
 * Connect.cpp -- connect transport command
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

#include <sstream>

#include "Connect.h"

namespace irccd {

namespace transport {

Connect::Connect(std::shared_ptr<TransportClientAbstract> client,
		std::string name,
		std::string host,
		int port,
		bool ssl,
		bool sslVerify)
	: TransportCommand(std::move(client))
	, m_name(std::move(name))
	, m_host(std::move(host))
	, m_port(port)
	, m_ssl(ssl)
	, m_sslVerify(sslVerify)
{
}

void Connect::exec(Irccd &)
{
	// TODO
}

std::string Connect::ident() const
{
	std::ostringstream oss;

	oss << std::boolalpha;
	oss << "connect:" << m_name << ":" << m_host;
	oss << m_port << ":" << m_ssl << ":" << m_sslVerify;

	return oss.str();
}

} // !transport

} // !irccd
