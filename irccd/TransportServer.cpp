/*
 * TransportServer.cpp -- I/O for irccd clients (acceptors)
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

#if !defined(IRCCD_SYSTEM_WINDOWS)
#  include <cstdio>
#endif

#include <sstream>

#include "TransportServer.h"

namespace irccd {

/* --------------------------------------------------------
 * TransportServerIpv6
 * -------------------------------------------------------- */

TransportServerIpv6::TransportServerIpv6(std::string host, unsigned port, bool ipv6only)
	: TransportServer{AF_INET6, address::Ipv6{std::move(host), port}}
{
	int v6opt = ipv6only;

	m_socket.set(IPPROTO_IPV6, IPV6_V6ONLY, v6opt);
}

std::string TransportServerIpv6::info() const
{
	std::ostringstream oss;

	oss << "ipv6, address: " << m_socket.getsockname().ip() << ", "
	    << "port: " << m_socket.getsockname().port();

	return oss.str();
}

/* --------------------------------------------------------
 * TransportServerIpv4
 * -------------------------------------------------------- */

TransportServerIpv4::TransportServerIpv4(std::string host, unsigned port)
	: TransportServer{AF_INET, address::Ipv4{std::move(host), port}}
{
}

/**
 * @copydoc TransportAbstract::info
 */
std::string TransportServerIpv4::info() const
{
	std::ostringstream oss;

	oss << "ipv4, address: " << m_socket.getsockname().ip() << ", "
	    << "port: " << m_socket.getsockname().port();

	return oss.str();
}

/* --------------------------------------------------------
 * TransportServerUnix
 * -------------------------------------------------------- */

#if !defined(IRCCD_SYSTEM_WINDOWS)

TransportServerUnix::TransportServerUnix(std::string path)
	: TransportServer{AF_UNIX, address::Unix{path, true}}
	, m_path{std::move(path)}
{
}

TransportServerUnix::~TransportServerUnix()
{
	::remove(m_path.c_str());
}

std::string TransportServerUnix::info() const
{
	return "unix, path: " + m_path;
}

#endif

} // !irccd
