/*
 * SocketTcp.h -- portable C++ socket wrappers
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

#ifndef _SOCKET_TCP_NG_H_
#define _SOCKET_TCP_NG_H_

#include "Socket.h"

/**
 * @class SocketAbstractTcp
 * @brief Base class for TCP sockets
 *
 * This abstract class provides standard TCP functions for both clear
 * and SSL implementation.
 *
 * It does not contain default accept() and connect() because they varies too
 * much between standard and SSL. Also, the accept() function return different
 * types.
 */
class SocketAbstractTcp : public Socket {
protected:
	Socket standardAccept(SocketAddress &address);
	void standardConnect(const SocketAddress &address);

public:
	/**
	 * Construct an abstract socket from an already made socket.
	 *
	 * @param s the socket
	 */
	inline SocketAbstractTcp(Socket s)
		: Socket(s)
	{
	}

	/**
	 * Construct a standard TCP socket. The type is automatically
	 * set to SOCK_STREAM.
	 *
	 * @param domain the domain
	 * @param protocol the protocol
	 * @throw SocketError on error
	 */
	inline SocketAbstractTcp(int domain, int protocol)
		: Socket(domain, SOCK_STREAM, protocol)
	{
	}

	/**
	 * Listen for pending connection.
	 *
	 * @param max the maximum number
	 */
	void listen(int max = 128);

	/**
	 * Overloaded function.
	 *
	 * @param count the number of bytes to receive
	 * @return the string
	 * @throw SocketError on error
	 */
	inline std::string recv(unsigned count)
	{
		std::string result;

		result.resize(count);
		auto n = recv(const_cast<char *>(result.data()), count);
		result.resize(n);

		return result;
	}

	/**
	 * Overloaded function.
	 *
	 * @param count the number of bytes to receive
	 * @param timeout the maximum timeout in milliseconds
	 * @return the string
	 * @throw SocketError on error
	 */
	inline std::string waitRecv(unsigned count, int timeout)
	{
		std::string result;

		result.resize(count);
		auto n = waitRecv(const_cast<char *>(result.data()), count, timeout);
		result.resize(n);

		return result;
	}

	/**
	 * Overloaded function.
	 *
	 * @param data the string to send
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	inline unsigned send(const std::string &data)
	{
		return send(data.c_str(), data.size());
	}

	/**
	 * Overloaded function.
	 *
	 * @param data the string to send
	 * @param timeout the maximum timeout in milliseconds
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	inline unsigned waitSend(const std::string &data, int timeout)
	{
		return waitSend(data.c_str(), data.size(), timeout);
	}

	/**
	 * Receive data.
	 *
	 * @param data the destination buffer
	 * @param length the buffer length
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	virtual unsigned recv(void *data, unsigned length) = 0;

	/**
	 * Receive data.
	 *
	 * @param data the destination buffer
	 * @param length the buffer length
	 * @param timeout the maximum timeout in milliseconds
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	virtual unsigned waitRecv(void *data, unsigned length, int timeout) = 0;

	/**
	 * Send data.
	 *
	 * @param data the buffer
	 * @param length the buffer length
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	virtual unsigned send(const void *data, unsigned length) = 0;

	/**
	 * Send data.
	 *
	 * @param data the buffer
	 * @param length the buffer length
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	virtual unsigned waitSend(const void *data, unsigned length, int timeout) = 0;
};

/**
 * @class SocketTcp
 * @brief End-user class for TCP sockets
 */
class SocketTcp : public SocketAbstractTcp {
public:
	using SocketAbstractTcp::SocketAbstractTcp;
	using SocketAbstractTcp::recv;
	using SocketAbstractTcp::waitRecv;
	using SocketAbstractTcp::send;
	using SocketAbstractTcp::waitSend;

	/**
	 * Accept a clear TCP socket.
	 *
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketTcp accept();

	/**
	 * Accept a clear TCP socket.
	 *
	 * @param info the client information
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketTcp accept(SocketAddress &info);

	/**
	 * Accept a clear TCP socket.
	 *
	 * @param timeout the maximum timeout in milliseconds
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketTcp waitAccept(int timeout);

	/**
	 * Accept a clear TCP socket.
	 *
	 * @param info the client information
	 * @param timeout the maximum timeout in milliseconds
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketTcp waitAccept(SocketAddress &info, int timeout);

	/**
	 * Connect to an end point.
	 *
	 * @param address the address
	 * @throw SocketError on error
	 */
	void connect(const SocketAddress &address);

	/**
	 * Connect to an end point.
	 *
	 * @param timeout the maximum timeout in milliseconds
	 * @param address the address
	 * @throw SocketError on error
	 */
	void waitConnect(const SocketAddress &address, int timeout);

	/**
	 * @copydoc SocketAbstractTcp::recv
	 */
	unsigned recv(void *data, unsigned length) override;

	/**
	 * @copydoc SocketAbstractTcp::waitRecv
	 */
	unsigned waitRecv(void *data, unsigned length, int timeout) override;

	/**
	 * @copydoc SocketAbstractTcp::send
	 */
	unsigned send(const void *data, unsigned length) override;

	/**
	 * @copydoc SocketAbstractTcp::waitSend
	 */
	unsigned waitSend(const void *data, unsigned length, int timeout) override;
};

#endif // !_SOCKET_TCP_NG_H_
