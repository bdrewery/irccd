/*
 * SocketListener.h -- portable select() wrapper
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

#ifndef _SOCKET_LISTENER_H_
#define _SOCKET_LISTENER_H_

/**
 * @file SocketListener.h
 * @brief Portable C++ select(2) wrapper
 */

#include <vector>

#include "Socket.h"

namespace irccd {

/**
 * @class SocketTimeout
 * @brief Thrown when a timeout occured
 */
class SocketTimeout : public std::exception {
public:
	/**
	 * Get the error.
	 *
	 * @return the error
	 */
	virtual const char * what() const throw();
};

/**
 * @class SocketListener
 * @brief A select(2) wrapper
 */
class SocketListener
{
private:
	std::vector<Socket> m_clients;

public:
	/**
	 * Add a socket to listen to.
	 *
	 * @param s the socket
	 */
	void add(Socket s);

	/**
	 * Get the number of client in the listener.
	 *
	 * @return the number
	 */
	long size() const;

	/**
	 * Remove a socket from the list.
	 *
	 * @param s the socket
	 */
	void remove(Socket s);

	/**
	 * Remove every sockets in the listener.
	 */
	void clear();

	/**
	 * Wait for an event in the socket list. If both s and us are set to 0 then
	 * it waits indefinitely.
	 *
	 * @param s the timeout in seconds
	 * @param us the timeout in milliseconds
	 * @return the socket ready
	 * @throw SocketError on error
	 * @throw SocketTimeout on timeout
	 */
	Socket select(int s = 0, int us = 0);
};

} // !irccd

#endif // !_SOCKET_LISTENER_H_
