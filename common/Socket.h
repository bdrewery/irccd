/*
 * Socket.h -- portable C++ socket wrappers
 *
 * Copyright (c) 2013, David Demelier <markand@malikania.fr>
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
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <sys/types.h>

#  include <arpa/inet.h>

#  include <netinet/in.h>

#  include <fcntl.h>
#  include <netdb.h>
#  include <unistd.h>

#  define ioctlsocket(s, p, a)	::ioctl(s, p, a)
#  define closesocket(s)	::close(s)

#  define gai_strerrorA		gai_strerror

#  define INVALID_SOCKET	-1
#  define SOCKET_ERROR		-1
#endif

namespace irccd
{

class SocketAddress;

/**
 * @class SocketError
 * @brief socket error reporting
 *
 * This class is mainly used in all socket operations that may fail.
 */
class SocketError : public std::exception
{
private:
	std::string m_error;

public:
	SocketError(const std::string &error);

	virtual const char * what(void) const throw();
};

/**
 * @class Socket
 * @brief socket abstraction
 *
 * This class is a big wrapper around sockets functions but portable,
 * there is some functions that helps for getting error reporting.
 */
class Socket
{
public:
#if defined(_WIN32)
	typedef SOCKET		Type;
	typedef const char *	ConstArg;
	typedef char *		Arg;
#else
	typedef int		Type;
	typedef const void *	ConstArg;
	typedef void *		Arg;
#endif

protected:
	Socket::Type m_socket;

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
	 * Constructor with a socket already opened.
	 *
	 * @param sock the socket
	 */
	Socket(Socket::Type sock);

	/**
	 * Default destructor.
	 */
	virtual ~Socket();

	/**
	 * Get the socket.
	 *
	 * @return the socket
	 */
	Type getSocket() const;

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
	 * @param location a IP or Unix location
	 * @throw SocketError error
	 */
	void bind(const SocketAddress &address);

	/**
	 * Try to connect to the specific address
	 *
	 * @param addr the address
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
	 * @param address the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned sendto(const void *data, unsigned dataLen, const SocketAddress &info);

	/**
	 * Send a message to a connection-less socket.
	 *
	 * @param message the message
	 * @param address the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	unsigned sendto(const std::string &message, const SocketAddress &info);

	/**
	 * Close the socket.
	 */
	void close();
};

bool operator==(const Socket &s1, const Socket &s2);

bool operator<(const Socket &s, const Socket &s2);

} // !irccd

#endif // !_SOCKET_H_
