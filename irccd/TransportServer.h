/*
 * TransportServer.h -- I/O for irccd clients (acceptors)
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

#ifndef _IRCCD_TRANSPORT_SERVER_H_
#define _IRCCD_TRANSPORT_SERVER_H_

/**
 * @file Transport.h
 * @brief Transports for irccd
 */

#include <memory>
#include <string>

#include <IrccdConfig.h>

#include "SocketAddress.h"
#include "Socket.h"
#include "TransportClient.h"

namespace irccd {

/**
 * @class TransportServerAbstract
 * @brief Bring networking between irccd and irccdctl
 *
 * This class contains a master sockets for listening to TCP connections, it is
 * then processed by irccd.
 *
 * The transport class supports the following domains:
 *
 * | Domain                | Class                 |
 * |-----------------------|-----------------------|
 * | IPv4, IPv6            | TransportServerIp     |
 * | Unix (not on Windows) | TransportServerUnix   |
 *
 * Note: IPv4 and IPv6 can be combined, using TransportServer::IPv6 and its option.
 */
class TransportServerAbstract {
private:
	TransportServerAbstract(const TransportServerAbstract &) = delete;
	TransportServerAbstract(TransportServerAbstract &&) = delete;

	TransportServerAbstract &operator=(const TransportServerAbstract &) = delete;
	TransportServerAbstract &operator=(TransportServerAbstract &&) = delete;

public:
	/**
	 * Default constructor.
	 */
	TransportServerAbstract() = default;

	/**
	 * Destructor defaulted.
	 */
	virtual ~TransportServerAbstract() = default;

	/**
	 * Retrieve the underlying socket.
	 *
	 * @return the socket
	 */
	virtual SocketAbstract &socket() noexcept = 0;

	/**
	 * Accept a new client depending on the domain.
	 *
	 * @return the new client
	 */
	virtual std::shared_ptr<TransportClientAbstract> accept() = 0;

	/**
	 * Get information about the transport.
	 *
	 * @return the info
	 */
	virtual std::string info() const = 0;
};

/**
 * @class TransportServer
 * @brief Wrapper for Transport
 *
 * Base template class for TransportServer's, automatically binds and accept sockets.
 */
template <typename Address>
class TransportServer : public TransportServerAbstract {
protected:
	SocketTcp<Address> m_socket;

public:
	/**
	 * Construct a socket.
	 *
	 * @param domain the domain (AF_INET, AF_INET6, ...)
	 */
	TransportServer(int domain, const Address &address)
		: m_socket{domain, 0}
	{
		m_socket.set(SOL_SOCKET, SO_REUSEADDR, 1);
		m_socket.bind(address);
		m_socket.listen();
	}

	/**
	 * Return the underlying socket.
	 *
	 * @return the socket
	 */
	SocketAbstract &socket() noexcept override
	{
		return m_socket;
	}

	/**
	 * @copydoc Transport::accept
	 */
	std::shared_ptr<TransportClientAbstract> accept() override
	{
		return std::make_shared<TransportClient<Address>>(m_socket.accept());
	}
};

/**
 * @class TransportServerIpv6
 * @brief Implementation of transports using IPv6
 */
class TransportServerIpv6 : public TransportServer<address::Ipv6> {
public:
	/**
	 * Create an IPv6 and optionally IPv4 transport.
	 *
	 * @param address the address (or * for any)
	 * @param port the port
	 * @param ipv6only set to true to make IPv6 only
	 */
	TransportServerIpv6(std::string address, unsigned port, bool ipv6only = true);

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override;
};

/**
 * @class TransportServerIpv4
 * @brief Implementation of transports using IPv6
 */
class TransportServerIpv4 : public TransportServer<address::Ipv4> {
public:
	/**
	 * Create an IPv4 transport.
	 *
	 * @param address the address (or * for any)
	 * @param port the port
	 */
	TransportServerIpv4(std::string host, unsigned port);

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override;
};

#if !defined(IRCCD_SYSTEM_WINDOWS)

/**
 * @class TransportServerUnix
 * @brief Implementation of transports for Unix sockets
 */
class TransportServerUnix : public TransportServer<address::Unix> {
private:
	std::string m_path;

public:
	/**
	 * Create a Unix transport.
	 *
	 * @param path the path
	 */
	TransportServerUnix(std::string path);

	/**
	 * Destroy the transport and remove the file.
	 */
	~TransportServerUnix();

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override;
};

#endif // !_WIN32

} // !irccd

#endif // !_IRCCD_TRANSPORT_SERVER_H_
