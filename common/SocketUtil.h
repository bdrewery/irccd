/*
 * SocketUtil.h -- socket utilities
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

#ifndef _SOCKET_UTIL_H_
#define _SOCKET_UTIL_H_

#include <exception>
#include <string>

#include "SocketClient.h"

namespace irccd {

class SocketUtil {
public:
	class ErrorException : public std::exception {
	private:
		std::string m_error;

	public:
		ErrorException(const std::string &error)
		{
			m_error = error;
		}

		~ErrorException(void)
		{
		}

		virtual const char * what(void) const throw()
		{
			return m_error.c_str();
		}
	};

	/**
	 * Connect to a unix socket.
	 *
	 * @param path the socket path
	 * @return a socket stream
	 * @throw ErrorException if failed
	 */
	static SocketClient * connectUnix(const std::string &path);

	/**
	 * Connect to a internet socket.
	 *
	 * @param host the host
	 * @param port the port
	 * @param family the family (Inet6, Inet4 or both)
	 * @return a socket stream
	 * @throw ErrorException if failed
	 * @see SocketInet
	 */
	static SocketClient * connectInet(const std::string &host, uint16_t port, int family);
};

} // !irccd

#endif // !_SOCKET_UTIL_H_
