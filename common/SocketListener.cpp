/*
 * SocketListener.cpp -- socket listener
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

#include <cerrno>

#include <poll.h>

#include "SocketListener.h"

using namespace irccd;

SocketListener::SocketListener(void)
{
}

SocketListener::~SocketListener(void)
{
}

void SocketListener::addClient(Socket *client)
{
	m_clients.push_back(client);
}

void SocketListener::removeClient(Socket *client)
{
	m_clients.erase(std::remove(m_clients.begin(), m_clients.end(), client),
	    m_clients.end());
}

Socket *SocketListener::select(int timeout)
{
	Socket *client = nullptr;
	pollfd *fds;
	int ret, realTimeout;

	if (m_clients.size() == 0)
		throw EmptyException();

	fds = new pollfd[m_clients.size()];

	for (size_t i = 0; i < m_clients.size(); ++i) {
		fds[i].fd = m_clients[i]->getSock();
		fds[i].events = POLLIN;
	}

	realTimeout = (timeout == 0) ? -1 : timeout * 1000;
	ret = poll(fds, m_clients.size(), realTimeout);

	if (ret == -1)
		throw ErrorException(strerror(errno));
	if (ret == 0)
		throw TimeoutException();

	for (size_t i = 0; i < m_clients.size(); ++i)
		if (fds[i].revents & POLLIN) {
			client = m_clients[i];		// keep same order as fds
			break;
		}

	delete [] fds;

	return client;
}
