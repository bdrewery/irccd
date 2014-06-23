/*
 * SocketAddress.cpp -- socket addresses management
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include <cerrno>
#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
#  include <sys/un.h>

#  define gai_strerrorA(s)	gai_strerror(s)
#  define INVALID_SOCKET	-1
#  define SOCKET_ERROR		-1
#endif

#include "SocketAddress.h"

namespace irccd {

/* --------------------------------------------------------
 * BindAddressIP implementation
 * -------------------------------------------------------- */

BindAddressIP::BindAddressIP(const std::string &iface, unsigned port, int family)
	: m_host(iface)
	, m_family(family)
	, m_port(port)
{
	if (m_family == AF_INET6) {
		sockaddr_in6 *ptr = (sockaddr_in6 *)&m_addr;

		ptr->sin6_family = AF_INET6;
		ptr->sin6_port = htons(m_port);

		if (m_host == "*")
			ptr->sin6_addr = in6addr_any;
		else if (inet_pton(AF_INET6, m_host.c_str(), &ptr->sin6_addr) <= 0)
			throw SocketError(Socket::getLastSysError());

		m_addrlen = sizeof (sockaddr_in6);
	} else {
		sockaddr_in *ptr = (sockaddr_in *)&m_addr;

		ptr->sin_family = AF_INET;
		ptr->sin_port = htons(m_port);

		if (m_host == "*")
			ptr->sin_addr.s_addr = INADDR_ANY;
		else if (inet_pton(AF_INET, m_host.c_str(), &ptr->sin_addr) <= 0)
			throw SocketError(Socket::getLastSysError());

		m_addrlen = sizeof (sockaddr_in);
	}
}

/* --------------------------------------------------------
 * ConnectAddressIP implementation
 * -------------------------------------------------------- */

ConnectAddressIP::ConnectAddressIP(const std::string &host, unsigned port, int family, int type)
{
	addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = family;
	hints.ai_socktype = type;

	error = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
	if (error)
		throw SocketError(gai_strerrorA(error));

	memcpy(&m_addr, res->ai_addr, res->ai_addrlen);
	m_addrlen = res->ai_addrlen;

	freeaddrinfo(res);
}

/* --------------------------------------------------------
 * AddressUnix implementation
 * -------------------------------------------------------- */

#if !defined(_WIN32)

AddressUnix::AddressUnix(const std::string &path, bool rm)
{
	sockaddr_un *sun = (sockaddr_un *)&m_addr;

	// Silently remove the file even if it fails
	if (rm)
		::remove(path.c_str());

	// Copy the path
	memset(sun->sun_path, 0, sizeof (sun->sun_path));
	strncpy(sun->sun_path, path.c_str(), sizeof (sun->sun_path) - 1);

	// Set the parameters
	sun->sun_family = AF_UNIX;
	m_addrlen = SUN_LEN(sun);
}

#endif // _WIN32

/* --------------------------------------------------------
 * SocketAddress implementation
 * -------------------------------------------------------- */

SocketAddress::SocketAddress()
	: m_addrlen(0)
{
	memset(&m_addr, 0, sizeof (m_addr));
}

SocketAddress::SocketAddress(const sockaddr_storage &addr, socklen_t length)
	: m_addr(addr)
	, m_addrlen(length)
{
}

SocketAddress::~SocketAddress()
{
}

const sockaddr_storage &SocketAddress::address() const
{
	return m_addr;
}

socklen_t SocketAddress::length() const
{
	return m_addrlen;
}

bool operator==(const SocketAddress &sa1, const SocketAddress &sa2)
{
	return sa1.length() == sa2.length() &&
	    memcmp(&sa1.address(), &sa2.address(), sizeof (sockaddr_storage)) == 0;
}

bool operator<(const SocketAddress &sa1, const SocketAddress &sa2)
{
	return sa1.length() < sa2.length();
}

} // !irccd
