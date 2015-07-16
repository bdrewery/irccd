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

/**
 * @file SocketAddress.h
 * @brief Describe addresses
 *
 * User may set the following variables before compiling these files:
 *
 * SOCKET_HAVE_SUN_LEN	- (bool) Some systems do not have SUN_LEN which is the preferred way to
 *			  compute the address size for a Unix address. Otherwise, sizeof is used.
 *
 * Addresses are used in many places such as bind, recvfrom, accept and such. They describe different
 * parameters depending on the families.
 *
 * For example, when using IPv4, one should use Ipv4 class.
 *
 * All addresses are used directly as template parameter for a stronger type security. To be compatible
 * with the sockets classes, an address must have the following requirements:
 *
 * - Default constructible
 * - Copyable or Moveable
 *
 * Constructors:
 *
 * # With a storage address
 *
 * @code
 * Address(const sockaddr_storage &ss, socklen_t size);
 * @endcode
 *
 * The address is free to use the storage.
 *
 * Member functions:
 *
 * # Address
 *
 * @code
 * inline const sockaddr &address() const noexcept
 * @endcode
 *
 * Return the address converted to the C sockaddr structure.
 *
 * # Length
 *
 * @code
 * inline socklen_t length() const noexcept
 * @endcode
 *
 * Return the length of an address.
 *
 * # Info
 *
 * @code
 * SocketAddressInfo info() const
 * @endcode
 *
 * Return an information table about the address.
 */

#include <memory>
#include <string>
#include <unordered_map>

#if defined(_WIN32)
#  include <Winsock2.h>
#  include <Ws2tcpip.h>
#else
#  include <sys/socket.h>
#  include <sys/un.h>
#  include <arpa/inet.h>
#  include <netinet/in.h>
#endif

namespace irccd {

/**
 * Generic information table for an address.
 */
using SocketAddressInfo = std::unordered_map<std::string, std::string>;

/**
 * @class SocketAddressAbstract
 * @brief Generic base class for addresses
 */
class SocketAddressAbstract {
public:
	/**
	 * Get the domain.
	 *
	 * @return the domain
	 */
	virtual int domain() const noexcept = 0;

	/**
	 * Get the address as base type.
	 *
	 * @return the base address reference
	 */
	virtual const sockaddr &address() const noexcept = 0;

	/**
	 * Get the address length.
	 *
	 * @return the length
	 */
	virtual socklen_t length() const noexcept = 0;
};

/**
 * Compare two socket addresses, std::equal is used.
 *
 * @param addr1 the first address
 * @param addr2 the second address
 * @return true if equals
 */
bool operator==(const SocketAddressAbstract &addr1, const SocketAddressAbstract &addr2) noexcept;

/**
 * Compare two socket addresses, std::lexicographical_compare is used.
 *
 * @param addr1 the first address
 * @param addr2 the second address
 * @return true if addr1 < addr2
 */
bool operator<(const SocketAddressAbstract &addr1, const SocketAddressAbstract &addr2) noexcept;

/**
 * @brief Predefined addresses.
 */
namespace address {

/**
 * @class Ip
 * @brief Generic internet protocol address
 *
 * Create a connect address for internet protocol,
 * using getaddrinfo(3).
 *
 * @see Ipv4
 * @see Ipv6
 */
class Ip : public SocketAddressAbstract {
private:
	union {
		sockaddr_in m_sin;
		sockaddr_in6 m_sin6;
	};

	int m_domain{AF_INET};

public:
	/**
	 * Default constructor.
	 */
	Ip();

	/**
	 * Create an IPv4 or IPV6 end point.
	 *
	 * @param host the hostname
	 * @param port the port
	 * @param domain AF_INET or AF_INET6
	 * @throw SocketError on error
	 */
	Ip(const std::string &host, unsigned port, int domain);

	/**
	 * Construct an internet address from a storage address.
	 *
	 * @param ss the storage
	 * @param length the length
	 */
	Ip(const sockaddr_storage &ss, socklen_t length);

	/**
	 * Get the port.
	 *
	 * @return the port
	 */
	unsigned port() const noexcept;

	/**
	 * Get the ip address.
	 *
	 * @return the address in text form
	 */
	std::string ip() const;

	/**
	 * @copydoc SocketAddressAbstract::domain
	 */
	inline int domain() const noexcept
	{
		return m_domain;
	}

	/**
	 * @copydoc SocketAddressAbstract::address
	 */
	const sockaddr &address() const noexcept override
	{
		// Can't get a ternary operator to work here.
		if (m_domain == AF_INET6)
			return reinterpret_cast<const sockaddr &>(m_sin6);

		return reinterpret_cast<const sockaddr &>(m_sin);
	}

	/**
	 * @copydoc SocketAddressAbstract::length
	 */
	socklen_t length() const noexcept override
	{
		return (m_domain == AF_INET6) ? sizeof (sockaddr_in6) : sizeof (sockaddr_in);
	}

	/**
	 * @copydoc SocketAddressAbstract::info
	 */
	SocketAddressInfo info() const;
};

/**
 * @class Ipv6
 * @brief Convenient helper for IPv6 protocol
 */
class Ipv6 : public Ip {
public:
	/**
	 * Default constructor.
	 */
	Ipv6() = default;

	/**
	 * Construct an IPv6 address from storage.
	 *
	 * @param ss the storage
	 * @param length the length
	 */
	inline Ipv6(const sockaddr_storage &ss, socklen_t length)
		: Ip(ss, length)
	{
	}

	/**
	 * Construct an IPv6 address.
	 *
	 * @param host the host
	 * @param port the port
	 * @throw SocketError on error
	 */
	inline Ipv6(const std::string &host, unsigned port)
		: Ip(host, port, AF_INET6)
	{
	}
};

/**
 * @class Ipv4
 * @brief Convenient helper for IPv4 protocol
 */
class Ipv4 : public Ip {
public:
	/**
	 * Default constructor.
	 */
	Ipv4() = default;

	/**
	 * Construct an IPv4 address from storage.
	 *
	 * @param ss the storage
	 * @param length the length
	 */
	inline Ipv4(const sockaddr_storage &ss, socklen_t length)
		: Ip(ss, length)
	{
	}

	/**
	 * Construct an IPv4 address.
	 *
	 * @param host the host
	 * @param port the port
	 * @throw SocketError on error
	 */
	inline Ipv4(const std::string &host, unsigned port)
		: Ip(host, port, AF_INET)
	{
	}
};

#if !defined(_WIN32)

/**
 * @class Unix
 * @brief unix family sockets
 *
 * Create an address to a specific path. Only available on Unix.
 */
class Unix : public SocketAddressAbstract {
private:
	sockaddr_un m_sun;
	std::string m_path;

public:
	/**
	 * Default constructor.
	 */
	Unix() = default;

	/**
	 * Construct an address to a path.
	 *
	 * @param path the path
	 * @param rm remove the file before (default: false)
	 */
	Unix(std::string path, bool rm = false);

	/**
	 * Construct an unix address from a storage address.
	 *
	 * @param ss the storage
	 * @param length the length
	 */
	Unix(const sockaddr_storage &ss, socklen_t length);

	inline int domain() const noexcept
	{
		return AF_LOCAL;
	}

	/**
	 * @copydoc SocketAddress::address
	 */
	inline const sockaddr &address() const noexcept override
	{
		return reinterpret_cast<const sockaddr &>(m_sun);
	}

	/**
	 * @copydoc SocketAddress::length
	 */
	inline socklen_t length() const noexcept override
	{
#if defined(SOCKET_HAVE_SUN_LEN)
		return SUN_LEN(&m_sun);
#else
		return sizeof (m_sun);
#endif
	}

	/**
	 * @copydoc SocketAddress::info
	 */
	SocketAddressInfo info() const;
};

#endif // !_WIN32

} // !address

} // !irccd

#endif // !_SOCKET_ADDRESS_NG_H_
