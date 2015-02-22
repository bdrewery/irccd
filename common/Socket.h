/*
 * Socket.h -- portable C++ socket wrappers
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

#ifndef _IRCCD_SOCKET_H_
#define _IRCCD_SOCKET_H_

/**
 * @file Socket.h
 * @brief Portable C++ low level sockets
 */

#include <exception>
#include <memory>
#include <string>

#if defined(_WIN32)
#  include <WinSock2.h>
#  include <WS2tcpip.h>
#else
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <sys/types.h>

#  include <arpa/inet.h>

#  include <netinet/in.h>
#  include <netinet/tcp.h>

#  include <fcntl.h>
#  include <netdb.h>
#  include <unistd.h>
#endif

namespace irccd {

class SocketAddress;

/**
 * @class SocketError
 * @brief socket error reporting
 *
 * This class is mainly used in all socket operations that may fail.
 */
class SocketError : public std::exception {
private:
	std::string m_error;

public:
	/**
	 * Constructor with error.
	 *
	 * @param error the error
	 */
	SocketError(const std::string &error);

	/**
	 * Get the error.
	 *
	 * @return the error
	 */
	virtual const char * what() const throw();
};

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
	typedef SOCKET		Type;		//!< the socket type
	typedef const char *	ConstArg;	//!< the const argument
	typedef char *		Arg;		//!< the argument
#else
	typedef int		Type;		//!< the socket type
	typedef const void *	ConstArg;	//!< the const argument
	typedef void *		Arg;		//!< the argument
#endif

private:
	Socket::Type m_socket;
	int m_domain;
	int m_type;
	int m_protocol;

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
	 * Constructor to create a new socket.
	 *
	 * @param domain the domain
	 * @param type the type
	 * @param protocol the protocol
	 * @throw SocketError on error
	 */
	Socket(int domain, int type, int protocol);

	/**
	 * Get the socket.
	 *
	 * @return the socket
	 */
	Type getSocket() const;

	/**
	 * Get the domain.
	 *
	 * @return the domain
	 */
	int getDomain() const;

	/**
	 * Get the type of socket.
	 *
	 * @return the type
	 */
	int getType() const;

	/**
	 * Get the protocol.
	 *
	 * @return the protocol
	 */
	int getProtocol() const;

	/**
	 * Set an option for the socket.
	 *
	 * @param level the setting level
	 * @param name the name
	 * @param arg the value
	 * @param argLen the argument length
	 * @throw SocketError on error
	 */
	void set(int level, int name, const void *arg, unsigned argLen);

	/**
	 * Enable or disable blocking mode.
	 *
	 * @param block the mode
	 */
	void blockMode(bool block = true);

	/**
	 * Bind the socket.
	 *
	 * @param address a IP or Unix location
	 * @throw SocketError error
	 */
	void bind(const SocketAddress &address);

	/**
	 * Try to connect to the specific address
	 *
	 * @param address the address
	 * @throw SocketError on error
	 */
	void connect(const SocketAddress &address);

	/**
	 * Accept a client without getting its info.
	 *
	 * @return a client ready to use
	 * @throw SocketError on error
	 */
	Socket accept();

	/**
	 * Accept a client.
	 *
	 * @param info the optional client info
	 * @return a client ready to use
	 * @throw SocketError on error
	 */
	Socket accept(SocketAddress &info);

	/**
	 * Listen to a specific number of pending connections.
	 *
	 * @param max the max number of clients
	 * @throw SocketError on error
	 */
	void listen(int max);

	/**
	 * Receive some data.
	 *
	 * @param data the destination pointer
	 * @param dataLen max length to receive
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	unsigned recv(void *data, unsigned dataLen);

	/**
	 * Send some data.
	 *
	 * @param data the data to send
	 * @param dataLen the data length
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned send(const void *data, unsigned dataLen);

	/**
	 * Send a message as a string.
	 *
	 * @param message the message
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned send(const std::string &message);

	/**
	 * Receive from a connection-less socket without getting
	 * client information.
	 *
	 * @param data the destination pointer
	 * @param dataLen max length to receive
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	unsigned recvfrom(void *data, unsigned dataLen);

	/**
	 * Receive from a connection-less socket and get the client
	 * information.
	 *
	 * @param data the destination pointer
	 * @param dataLen max length to receive
	 * @param info the client info
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	unsigned recvfrom(void *data, unsigned dataLen, SocketAddress &info);

	/**
	 * Send some data to a connection-less socket.
	 *
	 * @param data the data to send
	 * @param dataLen the data length
	 * @param info the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned sendto(const void *data, unsigned dataLen, const SocketAddress &info);

	/**
	 * Send a message to a connection-less socket.
	 *
	 * @param message the message
	 * @param info the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned sendto(const std::string &message, const SocketAddress &info);

	/**
	 * Close the socket.
	 */
	void close();
};

/**
 * Test equality.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if equals
 */
bool operator==(const Socket &s1, const Socket &s2);

/**
 * Less operator.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 is less
 */
bool operator<(const Socket &s1, const Socket &s2);

} // !irccd

#endif // !_IRCCD_SOCKET_H_
