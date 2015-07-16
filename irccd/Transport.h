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

#ifndef _IRCCD_TRANSPORT_ACCEPTOR_H_
#define _IRCCD_TRANSPORT_ACCEPTOR_H_

/**
 * @file Transport.h
 * @brief Transports for irccd
 */

#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

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
 * then processed by TransportService who select() them.
 *
 * The transport class supports the following domains:
 *
 * | Domain                | Class                 |
 * |-----------------------|-----------------------|
 * | IPv4, IPv6            | TransportServerIp   |
 * | Unix (not on Windows) | TransportServerUnix |
 *
 * Note: IPv4 and IPv6 can be combined, using Transport::IPv4 | Transport::IPv6
 * makes the transport available on both domains.
 *
 * Because this class owns a socket that will be borrowed to a SocketListener, it is not copyable
 * and not movable so that underlying socket will never be invalidated.
 */
class TransportServerAbstract {
public:
	TransportServerAbstract(const TransportServerAbstract &) = delete;
	TransportServerAbstract(TransportServerAbstract &&) = delete;

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
	virtual std::unique_ptr<TransportClientAbstract> accept() = 0;

	/**
	 * Get information about the transport.
	 *
	 * @return the info
	 */
	virtual std::string info() const = 0;

	TransportServerAbstract &operator=(const TransportServerAbstract &) = delete;
	TransportServerAbstract &operator=(TransportServerAbstract &&) = delete;
};

/**
 * @class TransportServer
 * @brief Wrapper for Transport
 *
 * This class contains the underlying socket (SocketTcp or SocketSsl) and
 * provide a destructor that automatically close it.
 *
 * It also provides the accept() function.
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
	std::unique_ptr<TransportClientAbstract> accept() override
	{
		// TODO
		//return std::make_unique<TransportClient<Sock>>(m_socket.accept());
		return nullptr;
	}
};

class TransportServerIpv6 : public TransportServer<address::Ipv6> {
public:
	TransportServerIpv6(std::string host, unsigned port, bool ipv6only = true)
		: TransportServer{AF_INET6, address::Ipv6{std::move(host), port}}
	{
		int v6opt = ipv6only;

		m_socket.set(IPPROTO_IPV6, IPV6_V6ONLY, v6opt);
	}

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override
	{
		return "TODO";
	}
};

class TransportServerIpv4 : public TransportServer<address::Ipv4> {
public:
	TransportServerIpv4(std::string host, unsigned port)
		: TransportServer{AF_INET, address::Ipv4{std::move(host), port}}
	{
	}

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override
	{
		return "TODO";
	}
};

#if !defined(IRCCD_SYSTEM_WINDOWS)

class TransportServerUnix : public TransportServer<address::Unix> {
private:
	std::string m_path;

public:
	inline TransportServerUnix(std::string path)
		: TransportServer{AF_UNIX, address::Unix{path, true}}
		, m_path{std::move(path)}
	{
	}

	~TransportServerUnix()
	{
		::remove(m_path.c_str());
	}

	/**
	 * @copydoc TransportAbstract::info
	 */
	std::string info() const override
	{
		return m_path;
	}
};

#endif // !_WIN32

} // !irccd

#endif // !_IRCCD_TRANSPORT_H_
