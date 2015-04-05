/*
 * SocketUdp.h -- portable C++ socket wrappers
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

#ifndef _SOCKET_UDP_NG_H_
#define _SOCKET_UDP_NG_H_

#include "Socket.h"

/**
 * @class SocketUdp
 * @brief UDP implementation for sockets
 */
class SocketUdp : public Socket {
public:
	/**
	 * Construct a UDP socket. The type is automatically set to SOCK_DGRAM.
	 *
	 * @param domain the domain (e.g AF_INET)
	 * @param protocol the protocol (usually 0)
	 */
	SocketUdp(int domain, int protocol);

	/**
	 * Overloaded function.
	 *
	 * @param data the data
	 * @param address the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	inline unsigned sendto(const std::string &data, const SocketAddress &address)
	{
		return sendto(data.c_str(), data.length(), address);
	}

	/**
	 * Overloaded function.
	 *
	 * @param data the data
	 * @param info the client information
	 * @return the string
	 * @throw SocketError on error
	 */
	inline std::string recvfrom(unsigned count, SocketAddress &info)
	{
		std::string result;

		result.resize(count);
		auto n = recvfrom(const_cast<char *>(result.data()), count, info);
		result.resize(n);

		return result;
	}

	/**
	 * Receive data from an end point.
	 *
	 * @param data the destination buffer
	 * @param length the buffer length
	 * @param info the client information
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	virtual unsigned recvfrom(void *data, unsigned length, SocketAddress &info);

	/**
	 * Send data to an end point.
	 *
	 * @param data the buffer
	 * @param length the buffer length
	 * @param address the client address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	virtual unsigned sendto(const void *data, unsigned length, const SocketAddress &address);
};

#endif // !_SOCKET_UDP_NG_H_
