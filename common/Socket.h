/*
 * Socket.h -- abstract base socket class
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <exception>
#include <memory>
#include <string>

#if defined(_WIN32)
#  include <WinSock2.h>
#  include <WS2tcpip.h>
#else
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/un.h>

#  include <arpa/inet.h>

#  include <netinet/in.h>

#  include <netdb.h>
#  include <unistd.h>

#  define ioctlsocket(s)	::ioctl(s)
#  define closesocket(s)	::close(s)

#  define gai_strerrorA		gai_strerror

#  define INVALID_SOCKET	-1
#  define SOCKET_ERROR		-1
#endif

namespace irccd {

/**
 * @class EndPoint
 * @brief class for generic bind() or connect() calls
 */
class EndPoint {
protected:
	sockaddr_storage m_addr;
	unsigned m_addrlen;

public:
	EndPoint();

	EndPoint(const sockaddr_storage & addr, unsigned addrlen);

	const sockaddr_storage & getAddr() const;

	unsigned getAddrlen();
};

/**
 * @class BindPointIP
 * @brief internet protocol bind class
 *
 * Create a bind address for internet protocol,
 * IPv4 or IPv6.
 */
class BindPointIP : public EndPoint {
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
	 * @throw Socket::ErrorException on error
	 */
	BindPointIP(const std::string &addr, unsigned port, int family);
};

/**
 * @class ConnectPointIP
 * @brief internet protocol connect class
 *
 * Create a connect address for internet protocol,
 * using getaddrinfo(3).
 */
class ConnectPointIP : public EndPoint {
public:
	/**
	 * Create a connect end point.
	 *
	 * @param host the hostname
	 * @param port the port
	 * @param family AF_INET or AF_INET6
	 * @throw Socket::ErrorException on error
	 */
	ConnectPointIP(const std::string &host, unsigned port, int family);
};

#if !defined(_WIN32)
/**
 * @class UnixPoint
 * @brief unix domain end point
 *
 * Can be used for both bind() and connect().
 */
class UnixPoint : public EndPoint {
public:
	/**
	 * Create a UnixPoint.
	 *
	 * @param path the socket file path
	 */
	UnixPoint(const std::string &path);
};
#endif

/**
 * @class Socket
 * @brief socket abstraction
 *
 * This class is a big wrapper around sockets functions but portable,
 * there is some functions that helps for getting error reporting.
 */
class Socket {
public:
#if defined(_WIN32)
	typedef SOCKET		socket_t;
	typedef const char *	carg_t;
	typedef char *		arg_t;
#else
	typedef int		socket_t;
	typedef const void *	carg_t;
	typedef void *		arg_t;
#endif

	class ErrorException : public std::exception {
	private:
		std::string m_error;

	public:
		ErrorException(const std::string &error);

		virtual const char * what() const throw();
	};

protected:
	socket_t m_socket;
	EndPoint m_endPoint;

public:
	/**
	 * To be called before any socket operation.
	 */
	static void init();

	/**
	 * Get the last socket system error.
	 *
	 * @return a string message
	 */
	static std::string getLastSysError();

	/**
	 * To be called before exiting.
	 */
	static void finish();

	/**
	 * Default constructor.
	 */
	Socket();

	/**
	 * Create a socket with a specified sock and location.
	 *
	 * @param sock the socket
	 * @param endPoint the endPoint
	 */
	Socket(socket_t sock, EndPoint endPoint);

	/**
	 * Default destructor.
	 */
	virtual ~Socket();

	/**
	 * Get the socket.
	 *
	 * @return the socket
	 */
	socket_t getSocket() const;

	/**
	 * Set an option for the socket.
	 *
	 * @param level the setting level
	 * @param name the name
	 * @param arg the value
	 * @param argLen the argument length
	 * @throw Socket::ErrorException on error
	 */
	void set(int level, int name, const void *arg, unsigned argLen);

	/**
	 * Bind the socket.
	 *
	 * @param location a IP or Unix location
	 * @throw Socket::ErrorException on error
	 */
	void bind(EndPoint location);

	/**
	 * Close the socket.
	 */
	void close();
};

bool operator==(const Socket &s1, const Socket &s2);

} // !irccd

#endif // !_SOCKET_H_
