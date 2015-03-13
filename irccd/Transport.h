#ifndef _IRCCD_TRANSPORT_H_
#define _IRCCD_TRANSPORT_H_

/**
 * @file Transport.h
 * @brief Transports for irccd
 */

#include <memory>
#include <string>
#include <type_traits>

#include "SocketAddress.h"
#include "Socket.h"
#include "SocketTcp.h"
#include "TransportClient.h"

namespace irccd {

/**
 * @class TransportAbstract
 * @brief Bring networking between irccd and irccdctl
 *
 * This class contains a master sockets for listening to TCP connections, it is
 * then processed by TransportManager who select() them.
 *
 * The transport class supports the following domains:
 *
 * |-----------------------|---------------|
 * | Domain                | Class         |
 * |-----------------------|---------------|
 * | IPv4, IPv6            | TransportInet |
 * | IPv4, IPv6 over SSL   | TransportSsl  |
 * | Unix (not on Windows) | TransportUnix |
 * |-----------------------|---------------|
 *
 * Note: IPv4 and IPv6 can be combined, using Transport::IPv4 | Transport::IPv6
 * makes the transport available on both domains.
 */
class TransportAbstract {
public:
	enum Domain {
		IPv4	= (1 << 0),
		IPv6	= (1 << 1),
		Unix	= 4
	};

	/**
	 * Destructor defaulted.
	 */
	virtual ~TransportAbstract() = default;

	/**
	 * Retrieve the underlying socket.
	 *
	 * @return the socket
	 */
	virtual SocketAbstractTcp &socket() noexcept = 0;

	/**
	 * Bind to the address.
	 */
	virtual void bind() = 0;

	/**
	 * Accept a new client depending on the domain.
	 *
	 * @return the new client
	 */
	virtual std::unique_ptr<TransportClientAbstract> accept() = 0;
};

/**
 * @class Transport
 * @brief Wrapper for Transport
 *
 * This class contains the underlying socket (SocketTcp or SocketSsl) and
 * provide a destructor that automatically close it.
 *
 * It also provides the accept() function.
 */
template <typename Sock>
class Transport : public TransportAbstract {
private:
	static_assert(std::is_same<Sock, SocketTcp>::value, "Sock must be SocketTcp or SocketSsl");

protected:
	Sock m_socket;

public:
	/**
	 * Construct a socket.
	 *
	 * @param domain the domain (AF_INET, AF_INET6, ...)
	 */
	Transport(int domain)
		: m_socket(domain, 0)
	{
	}

	/**
	 * Destroy the transport and close the socket.
	 */
	~Transport()
	{
		m_socket.close();
	}

	/**
	 * Return the underlying socket.
	 *
	 * @return the socket
	 */
	SocketAbstractTcp &socket() noexcept override
	{
		return m_socket;
	}

	/**
	 * @copydoc Transport::accept
	 */
	std::unique_ptr<TransportClientAbstract> accept() override
	{
		return std::make_unique<TransportClient<Sock>>(m_socket.accept());
	}
};

/**
 * @class TransportAbstractInet
 * @brief Abstract class that provide common operations for internet domain
 *
 * This class is parent of TransportInet and TransportSsl.
 *
 * @see TransportInet
 * @see TransportSsl
 */
template <typename Sock>
class TransportAbstractInet : public Transport<Sock> {
protected:
	int m_domain;
	int m_port;
	std::string m_host;

public:
	/**
	 * Construct an internet domain transport.
	 *
	 * @param domain the domain
	 * @param port the port
	 * @param host the address "*" for any
	 */
	TransportAbstractInet(int domain, int port, std::string host = "*")
		: Transport<Sock>((domain & TransportAbstract::IPv6) ? AF_INET6 : AF_INET)
		, m_domain((domain & TransportAbstract::IPv6) ? AF_INET6 : AF_INET)
		, m_port(port)
		, m_host(std::move(host))
	{
		if (m_domain == AF_INET6) {
			Transport<Sock>::m_socket.set(IPPROTO_IPV6, IPV6_V6ONLY, static_cast<int>(domain == TransportAbstract::IPv6));
		}
	}

	/**
	 * Bind the socket.
	 */
	void bind() override
	{
		Transport<Sock>::m_socket.bind(address::Internet(m_host, m_port, m_domain));
		Transport<Sock>::m_socket.listen();
	}
};

/**
 * @class TransportInet
 * @brief Implementation of clear IPv4 and IPv6 transports
 */
class TransportInet : public TransportAbstractInet<SocketTcp> {
public:
	/**
	 * Inherited constructors.
	 */
	using TransportAbstractInet::TransportAbstractInet;
};

#if !defined(_WIN32)

class TransportUnix : public Transport<SocketTcp> {
private:
	std::string m_path;

public:
	inline TransportUnix(std::string path)
		: Transport<SocketTcp>(AF_UNIX)
		, m_path(std::move(path))
	{
	}

	void bind()
	{
		m_socket.bind(address::Unix(m_path, true));
		m_socket.listen();
	}
};

#endif // !_WIN32

} // !irccd

#endif // !_IRCCD_TRANSPORT_H_
