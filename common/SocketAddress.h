/*
 * SocketAddress.h -- socket addresses management
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

#ifndef _SOCKET_ADDRESS_NG_H_
#define _SOCKET_ADDRESS_NG_H_

#include <string>

#if defined(_WIN32)
#  include <Winsock2.h>
#  include <Ws2tcpip.h>
#else
#  include <sys/socket.h>
#endif

/**
 * @class SocketAddress
 * @brief base class for socket addresses
 *
 * This class is mostly used to bind, connect or getting information
 * on socket clients.
 *
 * @see Internet
 * @see Unix
 */
class SocketAddress {
protected:
	sockaddr_storage m_addr;
	socklen_t m_addrlen;

public:
	/**
	 * Default constructor.
	 */
	SocketAddress();

	/**
	 * Constructor with address and size.
	 *
	 * @param addr the address
	 * @param length the address length
	 */
	SocketAddress(const sockaddr_storage &addr, socklen_t length);

	/**
	 * Default destructor.
	 */
	virtual ~SocketAddress() = default;

	/**
	 * Get the address length
	 *
	 * @return the length
	 */
	socklen_t length() const;

	/**
	 * Get the address.
	 *
	 * @return the address
	 */
	const sockaddr_storage &address() const;

	/**
	 * Compare the addresses. The check is lexicographical.
	 *
	 * @param s1 the first address
	 * @param s2 the second address
	 * @return true if s1 is less than s2
	 */
	friend bool operator<(const SocketAddress &s1, const SocketAddress &s2);

	/**
	 * Compare the addresses.
	 *
	 * @param s1 the first address
	 * @param s2 the second address
	 * @return true if s1 == s2
	 */
	friend bool operator==(const SocketAddress &s1, const SocketAddress &s2);
};

namespace address {

/**
 * @class Internet
 * @brief internet protocol connect class
 *
 * Create a connect address for internet protocol,
 * using getaddrinfo(3).
 */
class Internet : public SocketAddress {
public:
	/**
	 * Create an IPv4 or IPV6 end point.
	 *
	 * @param host the hostname
	 * @param port the port
	 * @param family AF_INET, AF_INET6, ...
	 * @throw SocketError on error
	 */
	Internet(const std::string &host, unsigned port, int family);
};

#if !defined(_WIN32)

/**
 * @class Unix
 * @brief unix family sockets
 *
 * Create an address to a specific path. Only available on Unix.
 */
class Unix : public SocketAddress {
public:
	/**
	 * Construct an address to a path.
	 *
	 * @param path the path
	 * @param rm remove the file before (default: false)
	 */
	Unix(const std::string &path, bool rm = false);
};

#endif // ! !_WIN32

} // !address

#endif // !_SOCKET_ADDRESS_NG_H_
