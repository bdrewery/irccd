/*
 * SocketListener.cpp -- portable wrapper around select()
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

#include <algorithm>

#include "SocketListener.h"

using namespace irccd;

const char * SocketListener::TimeoutException::what(void) const throw()
{
	return "Timeout occured";
}

void SocketListener::add(Socket &s)
{
	m_clients.push_back(s);
}

void SocketListener::remove(Socket &s)
{
	m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), s), m_clients.end());
}

void SocketListener::clear()
{
	m_clients.clear();
}

size_t SocketListener::size()
{
	return m_clients.size();
}

Socket & SocketListener::select(int timeout)
{
	fd_set fds;
	timeval maxwait, *towait;
	int error;
	int fdmax;

	if (m_clients.size() == 0)
		throw Socket::ErrorException("No socket to listen to");
	
	fdmax = m_clients.front().getSocket();

	FD_ZERO(&fds);
	for (Socket &c : m_clients) {
		FD_SET(c.getSocket(), &fds);
		if ((int)c.getSocket() > fdmax)
			fdmax = c.getSocket();
	}

	maxwait.tv_sec = timeout;
	maxwait.tv_usec = 0;

        // Set to NULL for infinite timeout.
	towait = (timeout == 0) ? nullptr : &maxwait;

	error = ::select(fdmax + 1, &fds, NULL, NULL, towait);
	if (error == SOCKET_ERROR) 
		throw ErrorException(getLastSysError());
	if (error == 0)
		throw SocketListener::TimeoutException();

	for (Socket &c : m_clients)
		if (FD_ISSET(c.getSocket(), &fds))
			return c;

	throw ErrorException("No socket found");
}
