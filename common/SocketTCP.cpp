/*
 * SocketTCP.cpp -- stream based sockets
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

#include "SocketTCP.h"

using namespace irccd;

SocketTCP::SocketTCP()
{
}

SocketTCP::SocketTCP(socket_t sock, EndPoint endPoint)
	:Socket(sock, endPoint)
{
}

SocketTCP::~SocketTCP()
{
}

void SocketTCP::create(int family)
{
	m_socket = socket(family, SOCK_STREAM, 0);
	if (m_socket == INVALID_SOCKET)
		throw ErrorException(getLastSysError());
}

void SocketTCP::connect(EndPoint endPoint)
{
	const sockaddr_storage & addr = endPoint.getAddr();
	unsigned addrlen = endPoint.getAddrlen();

	if (::connect(m_socket, (sockaddr *)&addr, addrlen) == SOCKET_ERROR)
		throw ErrorException(getLastSysError());
}

void SocketTCP::listen(int max)
{
	if (::listen(m_socket, max) == SOCKET_ERROR)
		throw ErrorException(getLastSysError());
}

SocketTCP SocketTCP::accept()
{
	Socket::socket_t sock;
	sockaddr_storage addr;	
	socklen_t addrlen = sizeof (sockaddr_storage);

	memset(&addr, 0, sizeof (sockaddr_storage));
	sock = ::accept(m_socket, (sockaddr *)&addr, &addrlen);

	if (sock == INVALID_SOCKET)
		throw ErrorException(getLastSysError());

	return SocketTCP(sock, EndPoint(addr, addrlen));
}

unsigned SocketTCP::recv(void *data, unsigned dataLen)
{
	int nbread;

	nbread = ::recv(m_socket, (arg_t)data, dataLen, 0);
	if (nbread == SOCKET_ERROR) 
		throw ErrorException(getLastSysError());

	return (unsigned)nbread;
}

unsigned SocketTCP::send(const void *data, unsigned dataLen)
{
	int nbsent;

	nbsent = ::send(m_socket, (carg_t)data, dataLen, 0);
	if (nbsent == SOCKET_ERROR)
		throw ErrorException(getLastSysError());

	return (unsigned)nbsent;
}

bool irccd::operator<(const SocketTCP &s1, const SocketTCP &s2)
{
	return s1.getSocket() < s2.getSocket();
}

bool irccd::operator==(const SocketTCP &s1, const SocketTCP &s2)
{
	return s1.getSocket() == s2.getSocket();
}
