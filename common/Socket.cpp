/*
 * Socket.cpp -- base abstract socket class
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
#include <sstream>

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Socket.h"

using namespace irccd;
using namespace std;

Socket::Socket(void)
{
}

Socket::~Socket(void)
{
}

int Socket::getSock(void) const
{
	return m_sock;
}

void Socket::setSock(int sock)
{
	m_sock = sock;
}

void Socket::setFamily(int family)
{
	m_family = family;
}

const string & Socket::getErrorMessage(void) const
{
	return m_error;
}

bool Socket::create(void)
{
	ostringstream oss;

	if ((m_sock = ::socket(m_family, SOCK_STREAM, 0)) < 0) {
		oss << "socket: " << strerror(errno);
		m_error = oss.str();
		return false;
	}

	return true;
}

unsigned Socket::send(const void *data, unsigned dataLength)
{
	int length = ::send(m_sock, data, dataLength, 0);

	if (length <= 0)
		throw Socket::Exception(strerror(errno));

	return length;
}

unsigned Socket::receive(void *data, unsigned dataLength)
{
	int length = ::recv(m_sock, data, dataLength, 0);

	if (length == 0)
		throw Socket::Exception("client disconnected", true);
	else if (length < 0)
		throw Socket::Exception(strerror(errno));

	return length;
}

void Socket::close(void)
{
	::close(m_sock);
}
