/*
 * SocketListener.h -- socket listener
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

#ifndef _SOCKET_LISTENER_H_
#define _SOCKET_LISTENER_H_

#include <exception>
#include <sstream>
#include <string>
#include <vector>

#include "Socket.h"

namespace irccd {

class SocketListener {
public:
	class TimeoutException : public std::exception {
	public:
		virtual const char *what(void) const throw() {
			return "Request timeout";
		}
	};

	class EmptyException : public std::exception {
	public:
		virtual const char *what(void) const throw() {
			return "No socket to wait";
		}
	};

	class ErrorException : public std::exception {
		std::string m_error;
	public:
		ErrorException(const std::string &error)
			:m_error(error)
		{
		}

		virtual const char *what(void) const throw() {
			std::ostringstream oss;

			oss << "Error occured: ";
			oss << m_error;

			return oss.str().c_str();
		}

	};

private:
	std::vector<Socket *> m_clients;

public:
	SocketListener(void);
	~SocketListener(void);

	/**
	 * Add a client to listen to.
	 *
	 * @param client the client socket
	 */
	void addClient(Socket *client);

	/**
	 * Remove a client from the list.
	 *
	 * @param client the client
	 */
	void removeClient(Socket *client);

	/**
	 * Wait until a socket is ready for something.
	 *
	 * @param timeout an optional timeout (0 == forever)
	 * @return the ready socket
	 * @throw TimeoutException if no one is ready within the allowed time
	 * @throw EmptyException if there the client list is empty
	 */
	Socket *select(int timeout = 30);
};

} // !irrcd_

#endif // !_SOCKET_LISTENER_H_
