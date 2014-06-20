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

#ifndef _SOCKET_ADDRESS_H_
#define _SOCKET_ADDRESS_H_

/**
 * @file SocketAddress.h
 * @brief Portable C++ socket addresses management
 */

#include "Socket.h"

namespace irccd {

/**
 * @class SocketAddress
 * @brief base class for socket addresses
 *
 * This class is mostly used to bind, connect or getting information
 * on socket clients.
 *
 * @see BindAddressIP
 * @see ConnectAddressIP
 * @see AddressUnix
 */
class SocketAddress {
protected:
	sockaddr_storage m_addr;	//!< the address
	socklen_t m_addrlen;		//!< the address length

public:
	// Friends.
	friend class Socket;

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
	virtual ~SocketAddress();

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
};

/**
 * @class BindAddressIP
 * @brief internet protocol bind class
 *
 * Create a bind address for internet protocol,
 * IPv4 or IPv6.
 */
class BindAddressIP : public SocketAddress
{
private:
	std::string m_host;
	int m_family;
	unsigned m_port;

public:
	/**
	 * Create a bind end point.
	 *
	 * @param addr the interface to bind
	 * @param port the port
	 * @param family AF_INET or AF_INET6
	 * @throw SocketError on error
	 */
	BindAddressIP(const std::string &addr, unsigned port, int family);
};

/**
 * @class ConnectAddressIP
 * @brief internet protocol connect class
 *
 * Create a connect address for internet protocol,
 * using getaddrinfo(3).
 */
class ConnectAddressIP : public SocketAddress
{
public:
	/**
	 * Create a connect end point.
	 *
	 * @param host the hostname
	 * @param port the port
	 * @param family AF_INET, AF_INET6, ...
	 * @param type of socket SOCK_STREAM, SOCK_DGRAM, ...
	 * @throw SocketError on error
	 */
	ConnectAddressIP(const std::string &host, unsigned port, int family, int type = SOCK_STREAM);
};

#if !defined(_WIN32)

/**
 * @class AddressUnix
 * @brief unix family sockets
 *
 * Create an address to a specific path. Only available on Unix.
 */
class AddressUnix : public SocketAddress
{
public:
	/**
	 * Construct an address to a path.
	 *
	 * @param path the path
	 * @param rm remove the file before (default: false)
	 */
	AddressUnix(const std::string &path, bool rm = false);
};

#endif // ! !_WIN32

/**
 * Test equality.
 *
 * @param sa1 the first socket address
 * @param sa2 the second socket address
 * @return true if equals
 */
bool operator==(const irccd::SocketAddress &sa1, const irccd::SocketAddress &sa2);

/**
 * Less operator.
 *
 * @param sa1 the first socket
 * @param sa2 the second socket
 * @return true if sa1 is less
 */
bool operator<(const irccd::SocketAddress &sa1, const irccd::SocketAddress &sa2);

} // !irccd

#endif // !_SOCKET_ADDRESS_H_
