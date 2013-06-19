/*
 * SocketListener.h -- portable wrapper around select()
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

#include <vector>

#include "Socket.h"

namespace irccd {

class SocketListener : public Socket {
public:
	class TimeoutException : public std::exception {
	public:
		virtual const char * what() const throw();
	};

private:
	std::vector<Socket> m_clients;

public:
	/**
	 * Add a socket to listen to.
	 *
	 * @param s the socket
	 */
	void add(Socket &s);

	/**
	 * Remove a socket from the list.
	 *
	 * @param s the socket
	 */
	void remove(Socket &s);

	/**
	 * Remove every sockets in the listener.
	 */
	void clear();

	/**
	 * Get the number of sockets registered for listening.
	 *
	 * @return the number of sockets to listen
	 */
	size_t size();

	/**
	 * Wait for an event in the socket liste.
	 *
	 * @param timeout an optional timeout, 0 means forever
	 * @return the socket ready
	 * @throw Socket::ErrorException on error
	 * @throw SocketListener::TimeoutException on timeout
	 */
	Socket & select(int timeout = 0);
};

} // !irccd

#endif // !_SOCKET_LISTENER_H_
