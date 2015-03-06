/*
 * SocketListener.cpp -- portable select() wrapper
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

#include <algorithm>

#if !defined(WIN32)
#  define INVALID_SOCKET	-1
#  define SOCKET_ERROR		-1
#endif

#include "SocketListener.h"

namespace irccd {

const char *SocketTimeout::what() const throw()
{
	return "Timeout occured";
}

void SocketListener::add(Socket s)
{
	m_clients.push_back(s);
}

long SocketListener::size() const
{
	return static_cast<long>(m_clients.size());
}

void SocketListener::remove(Socket s)
{
	m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), s), m_clients.end());
}

void SocketListener::clear(void)
{
	m_clients.clear();
}

Socket SocketListener::select(int s, int us)
{
	fd_set fds;
	timeval maxwait, *towait;
	int error;
	int fdmax = m_clients.front().getSocket();

	FD_ZERO(&fds);
	for (Socket &c : m_clients) {
		FD_SET(c.getSocket(), &fds);
		if ((int)c.getSocket() > fdmax)
			fdmax = c.getSocket();
	}

	maxwait.tv_sec = s;
	maxwait.tv_usec = us;

        // Set to NULL for infinite timeout.
	towait = (s == 0 && us == 0) ? nullptr : &maxwait;

	error = ::select(fdmax + 1, &fds, NULL, NULL, towait);
	if (error == SOCKET_ERROR)
		throw SocketError(Socket::getLastSysError());
	if (error == 0)
		throw SocketTimeout();

	for (Socket &c : m_clients)
		if (FD_ISSET(c.getSocket(), &fds))
			return c;

	throw SocketError("No socket found");
}

} // !irccd
