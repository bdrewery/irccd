/*
 * Socket.cpp -- portable C++ socket wrappers
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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
#include <cstring>

#include "Socket.h"
#include "SocketAddress.h"

namespace irccd
{

/* --------------------------------------------------------
 * SocketError implementation
 * -------------------------------------------------------- */

SocketError::SocketError(const std::string &error)
{
	m_error = error;
}

const char *SocketError::what() const throw()
{
	return m_error.c_str();
}

/* --------------------------------------------------------
 * Socket implementation
 * -------------------------------------------------------- */

void Socket::init()
{
#if defined(_WIN32)
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
}

/* --------------------------------------------------------
 * System dependent code
 * -------------------------------------------------------- */

#if defined(_WIN32)

std::string Socket::getLastSysError()
{
	LPSTR str = nullptr;
	std::string errmsg = "Unknown error";

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&str, 0, NULL);


	if (str)
	{
		errmsg = std::string(str);
		LocalFree(str);
	}

	return errmsg;
}

#else

std::string Socket::getLastSysError()
{
	return strerror(errno);
}

#endif

void Socket::finish()
{
#if defined(_WIN32)
	WSACleanup();
#endif
}

Socket::Socket()
	: m_domain(0)
	, m_type(0)
	, m_protocol(0)
{
}

Socket::Socket(int domain, int type, int protocol)
	: m_domain(domain)
	, m_type(type)
	, m_protocol(protocol)
{
	m_socket = socket(domain, type, protocol);

	if (m_socket == INVALID_SOCKET)
		throw SocketError(getLastSysError());
}

Socket::Type Socket::getSocket() const
{
	return m_socket;
}

int Socket::getDomain() const
{
	return m_domain;
}

int Socket::getType() const
{
	return m_type;
}

int Socket::getProtocol() const
{
	return m_protocol;
}

void Socket::set(int level, int name, const void *arg, unsigned argLen)
{
	if (setsockopt(m_socket, level, name, (Socket::ConstArg)arg, argLen) == SOCKET_ERROR)
		throw SocketError(getLastSysError());
}

void Socket::blockMode(bool block)
{
#if defined(O_NONBLOCK) && !defined(_WIN32)
	int flags;

	if ((flags = fcntl(m_socket, F_GETFL, 0)) == -1)
		flags = 0;

	if (!block)
		flags &= ~(O_NONBLOCK);
	else
		flags |= O_NONBLOCK;

	if (fcntl(m_socket, F_SETFL, flags) == -1)
		throw SocketError(getLastSysError());
#else
	unsigned long flags = (block) ? 0 : 1;

	if (ioctlsocket(m_socket, FIONBIO, &flags) == SOCKET_ERROR)
		throw SocketError(getLastSysError());
#endif
}

void Socket::bind(const SocketAddress &addr)
{
	const sockaddr_storage &sa = addr.address();
	size_t addrlen = addr.length();

	if (::bind(m_socket, (sockaddr *)&sa, addrlen) == SOCKET_ERROR)
		throw SocketError(getLastSysError());
}

void Socket::connect(const SocketAddress &addr)
{
	const sockaddr_storage &sa = addr.address();
	size_t addrlen = addr.length();

	if (::connect(m_socket, (sockaddr *)&sa, addrlen) == SOCKET_ERROR)
		throw SocketError(getLastSysError());
}

Socket Socket::accept()
{
	SocketAddress dummy;

	return accept(dummy);
}

Socket Socket::accept(SocketAddress &info)
{
	Socket s;

	info.m_addrlen = sizeof (sockaddr_storage);
	s.m_socket = ::accept(m_socket, (sockaddr *)&info.m_addr, &info.m_addrlen);

	// Usually accept works only with SOCK_STREAM
	s.m_domain	= info.m_addr.ss_family;
	s.m_type	= SOCK_STREAM;

	if (s.m_socket == INVALID_SOCKET)
		throw SocketError(getLastSysError());

	return s;
}

void Socket::listen(int max)
{
	if (::listen(m_socket, max) == SOCKET_ERROR)
		throw SocketError(getLastSysError());
}

unsigned Socket::recv(void *data, unsigned dataLen)
{
	int nbread;

	nbread = ::recv(m_socket, (Socket::Arg)data, dataLen, 0);
	if (nbread == SOCKET_ERROR)
		throw SocketError(getLastSysError());

	return (unsigned)nbread;
}

unsigned Socket::send(const void *data, unsigned dataLen)
{
	int nbsent;

	nbsent = ::send(m_socket, (Socket::ConstArg)data, dataLen, 0);
	if (nbsent == SOCKET_ERROR)
		throw SocketError(getLastSysError());

	return (unsigned)nbsent;
}

unsigned Socket::send(const std::string &message)
{
	return Socket::send(message.c_str(), message.length());
}

unsigned Socket::recvfrom(void *data, unsigned dataLen)
{
	SocketAddress dummy;

	return recvfrom(data, dataLen, dummy);
}

unsigned Socket::recvfrom(void *data, unsigned dataLen, SocketAddress &info)
{
	int nbread;

	info.m_addrlen = sizeof (struct sockaddr_storage);
	nbread = ::recvfrom(m_socket, (Socket::Arg)data, dataLen, 0,
	    (sockaddr *)&info.m_addr, &info.m_addrlen);

	if (nbread == SOCKET_ERROR)
		throw SocketError(getLastSysError());

	return (unsigned)nbread;
}

unsigned Socket::sendto(const void *data, unsigned dataLen, const SocketAddress &info)
{
	int nbsent;

	nbsent = ::sendto(m_socket, (Socket::ConstArg)data, dataLen, 0,
	    (const sockaddr *)&info.m_addr, info.m_addrlen);
	if (nbsent == SOCKET_ERROR)
		throw SocketError(getLastSysError());

	return (unsigned)nbsent;
}

unsigned Socket::sendto(const std::string &message, const SocketAddress &info)
{
	return sendto(message.c_str(), message.length(), info);
}

void Socket::close()
{
	(void)closesocket(m_socket);
}

bool operator==(const Socket &s1, const Socket &s2)
{
	return s1.getSocket() == s2.getSocket();
}

bool operator<(const Socket &s1, const Socket &s2)
{
	return s1.getSocket() < s2.getSocket();
}

} // !irccd
