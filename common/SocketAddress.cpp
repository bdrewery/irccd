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

namespace irccd {

bool operator==(const SocketAddressAbstract &address1, const SocketAddressAbstract &address2) noexcept
{
	const char *addr1 = reinterpret_cast<const char *>(&address1.address());
	const char *addr2 = reinterpret_cast<const char *>(&address2.address());

	return std::equal(
		addr1, addr1 + address1.length(),
		addr2, addr2 + address2.length()
	);
}

bool operator<(const SocketAddressAbstract &address1, const SocketAddressAbstract &address2) noexcept
{
	const char *addr1 = reinterpret_cast<const char *>(&address1.address());
	const char *addr2 = reinterpret_cast<const char *>(&address2.address());

	return std::lexicographical_compare(
		addr1, addr1 + address1.length(),
		addr2, addr2 + address2.length()
	);
}

namespace address {

/* --------------------------------------------------------
 * Ip implementation
 * -------------------------------------------------------- */

Ip::Ip()
{
	// Default uses IPv4
	std::memset(&m_sin, 0, sizeof (sockaddr_in));
}

Ip::Ip(const std::string &host, unsigned port, int domain)
	: m_domain{domain}
{
	if (host == "*") {
		if (m_domain == AF_INET6) {
			std::memset(&m_sin6, 0, sizeof (sockaddr_in6));

			m_sin6.sin6_addr = in6addr_any;
			m_sin6.sin6_family = AF_INET6;
			m_sin6.sin6_port = htons(port);
		} else {
			std::memset(&m_sin, 0, sizeof (sockaddr_in));

			m_sin.sin_addr.s_addr = INADDR_ANY;
			m_sin.sin_family = AF_INET;
			m_sin.sin_port = htons(port);
		}
	} else {
		addrinfo hints, *res;

		std::memset(&hints, 0, sizeof (addrinfo));
		hints.ai_family = domain;

		auto error = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
		if (error != 0) {
			throw SocketError{SocketError::System, "getaddrinfo", gai_strerror(error)};
		}

		if (m_domain == AF_INET6) {
			std::memcpy(&m_sin6, res->ai_addr, sizeof (sockaddr_in6));
		} else {
			std::memcpy(&m_sin, res->ai_addr, sizeof (sockaddr_in));
		}

		freeaddrinfo(res);
	}
}

Ip::Ip(const sockaddr_storage &ss, socklen_t length)
	: m_domain{ss.ss_family}
{
	if (ss.ss_family == AF_INET6) {
		std::memcpy(&m_sin6, &ss, length);
	} else {
		std::memcpy(&m_sin, &ss, length);
	}
}

unsigned Ip::port() const noexcept
{
	return m_domain == AF_INET6 ? ntohs(m_sin6.sin6_port) : ntohs(m_sin.sin_port);
}

std::string Ip::ip() const
{
	bool resolve{false};
	std::string ret{"*"};

	if (m_domain == AF_INET6) {
		if (std::memcmp(&m_sin6.sin6_addr, &in6addr_any, sizeof (in6addr_any)) != 0) {
			ret.resize(INET6_ADDRSTRLEN);
			resolve = true;
		}
	} else {
		if (m_sin.sin_addr.s_addr != INADDR_ANY) {
			ret.resize(INET_ADDRSTRLEN);
			resolve = true;
		}
	}

	if (resolve && inet_ntop(m_domain, &address(), &ret[0], ret.size()) == nullptr) {
		throw SocketError{SocketError::System, "inet_ntop"};
	}

	return ret;
}

SocketAddressInfo Ip::info() const
{
	return SocketAddressInfo{
		{ "type",	(m_domain == AF_INET6) ? "ipv6" : "ipv4"	},
		{ "port",	std::to_string(port())				},
		{ "ip",		ip()						}
	};
}

/* --------------------------------------------------------
 * Unix implementation
 * -------------------------------------------------------- */

#if !defined(_WIN32)

Unix::Unix(std::string path, bool rm)
	: m_path{std::move(path)}
{
	// Silently remove the file even if it fails
	if (rm) {
		::remove(m_path.c_str());
	}

	// Copy the path
	std::memset(m_sun.sun_path, 0, sizeof (m_sun.sun_path));
	std::strncpy(m_sun.sun_path, m_path.c_str(), sizeof (m_sun.sun_path) - 1);

	// Set the parameters
	m_sun.sun_family = AF_UNIX;
}

Unix::Unix(const sockaddr_storage &ss, socklen_t length)
{
	std::memcpy(&m_sun, &ss, length);

	if (ss.ss_family == AF_UNIX) {
		m_path = reinterpret_cast<const sockaddr_un &>(m_sun).sun_path;
	}
}

SocketAddressInfo Unix::info() const
{
	return SocketAddressInfo{
		{ "type",	"unix"	},
		{ "path",	m_path	}
	};
}

#endif // _WIN32

} // !irccd

} // !address
