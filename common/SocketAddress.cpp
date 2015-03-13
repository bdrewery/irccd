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

#include <algorithm>
#include <cstring>

#include "Socket.h"
#include "SocketAddress.h"

namespace address {

/* --------------------------------------------------------
 * Internet implementation
 * -------------------------------------------------------- */

Internet::Internet(const std::string &host, unsigned port, int domain)
{
	if (host == "*") {
		if (domain == AF_INET6) {
			sockaddr_in6 *ptr = (sockaddr_in6 *)&m_addr;

			ptr->sin6_addr = in6addr_any;
			ptr->sin6_family = AF_INET6;
			ptr->sin6_port = htons(port);

			m_addrlen = sizeof (sockaddr_in6);
		} else {
			sockaddr_in *ptr = (sockaddr_in *)&m_addr;

			ptr->sin_addr.s_addr = INADDR_ANY;
			ptr->sin_family = AF_INET;
			ptr->sin_port = htons(port);

			m_addrlen = sizeof (sockaddr_in);
		}
	} else {
		addrinfo hints, *res;

		std::memset(&hints, 0, sizeof (addrinfo));
		hints.ai_family = domain;

		auto error = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
		if (error != 0) {
			throw SocketError(SocketError::System, "getaddrinfo", gai_strerror(error));
		}

		std::memcpy(&m_addr, res->ai_addr, res->ai_addrlen);
		m_addrlen = res->ai_addrlen;
		freeaddrinfo(res);
	}
}

/* --------------------------------------------------------
 * Unix implementation
 * -------------------------------------------------------- */

#if !defined(_WIN32)

#include <sys/un.h>

Unix::Unix(const std::string &path, bool rm)
{
	sockaddr_un *sun = (sockaddr_un *)&m_addr;

	// Silently remove the file even if it fails
	if (rm) {
		::remove(path.c_str());
	}

	// Copy the path
	memset(sun->sun_path, 0, sizeof (sun->sun_path));
	strncpy(sun->sun_path, path.c_str(), sizeof (sun->sun_path) - 1);

	// Set the parameters
	sun->sun_family = AF_UNIX;
	m_addrlen = SUN_LEN(sun);
}

#endif // _WIN32

} // !address

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

const sockaddr_storage &SocketAddress::address() const
{
	return m_addr;
}

socklen_t SocketAddress::length() const
{
	return m_addrlen;
}

bool operator<(const SocketAddress &s1, const SocketAddress &s2)
{
	const auto &array1 = reinterpret_cast<const unsigned char *>(&s1.address());
	const auto &array2 = reinterpret_cast<const unsigned char *>(&s2.address());

	return std::lexicographical_compare(array1, array1 + s1.length(), array2, array2 + s2.length());
}

bool operator==(const SocketAddress &s1, const SocketAddress &s2)
{
	const auto &array1 = reinterpret_cast<const unsigned char *>(&s1.address());
	const auto &array2 = reinterpret_cast<const unsigned char *>(&s2.address());

	return std::equal(array1, array1 + s1.length(), array2, array2 + s2.length());
}
