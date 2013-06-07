/*
 * Socket.cpp -- abstract base socket class
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

#include "Socket.h"

using namespace irccd;
using namespace std;

/* --------------------------------------------------------
 * BindPointIP implementation
 * -------------------------------------------------------- */

BindPointIP::BindPointIP(const string &iface, unsigned port, int family)
	:m_host(iface), m_family(family), m_port(port)
{
	if (m_family == AF_INET6) {
		sockaddr_in6 *ptr = (sockaddr_in6 *)&m_addr;

		memset(ptr, 0, sizeof (sockaddr_in6));
		ptr->sin6_family = AF_INET6;
		ptr->sin6_port = htons(m_port);

		if (m_host == "*")
			ptr->sin6_addr = in6addr_any;
		else if (inet_pton(AF_INET6, m_host.c_str(), &ptr->sin6_addr) <= 0) {
			throw Socket::ErrorException(Socket::getLastSysError());
		}

		m_addrlen = sizeof (sockaddr_in6);
	} else {
		sockaddr_in *ptr = (sockaddr_in *)&m_addr;

		memset(ptr, 0, sizeof (sockaddr_in));
		ptr->sin_family = AF_INET;
		ptr->sin_port = htons(m_port);

		if (m_host == "*")
			ptr->sin_addr.s_addr = INADDR_ANY;
		else if (inet_pton(AF_INET, m_host.c_str(), &ptr->sin_addr) <= 0) {
			throw Socket::ErrorException(Socket::getLastSysError());
		}

		m_addrlen = sizeof (sockaddr_in);
	}
}

/* --------------------------------------------------------
 * ConnectPointIP implementation
 * -------------------------------------------------------- */

ConnectPointIP::ConnectPointIP(const string &host, unsigned port, int family)
{
	addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof (hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	error = getaddrinfo(host.c_str(), to_string(port).c_str(), &hints, &res);
	if (error)
		throw Socket::ErrorException(gai_strerrorA(error));

	memcpy(&m_addr, res->ai_addr, res->ai_addrlen);
	m_addrlen = res->ai_addrlen;

	freeaddrinfo(res);
}

/* --------------------------------------------------------
 * UnixPoint implementation
 * -------------------------------------------------------- */

#if !defined(_WIN32)
UnixPoint::UnixPoint(const string &path)
{
	sockaddr_un *sun = (sockaddr_un *)&m_addr;		

	memset(sun, 0, sizeof (sockaddr_un));
	memset(sun->sun_path, 0, sizeof (sun->sun_path));
	strncpy(sun->sun_path, path.c_str(), sizeof (sun->sun_path) - 1);

	sun->sun_family = AF_UNIX;
	m_addrlen = SUN_LEN(sun);
}
#endif

/* --------------------------------------------------------
 * EndPoint implementation
 * -------------------------------------------------------- */

EndPoint::EndPoint()
{
	memset(&m_addr, 0, sizeof (m_addr));
}

EndPoint::EndPoint(const sockaddr_storage & addr, unsigned addrlen)
{
	m_addr = addr;
	m_addrlen = addrlen;
}

const sockaddr_storage & EndPoint::getAddr() const
{
	return m_addr;
}

unsigned EndPoint::getAddrlen()
{
	return m_addrlen;
}

/* --------------------------------------------------------
 * Socket::ErrorExecption implementation
 * -------------------------------------------------------- */

Socket::ErrorException::ErrorException(const string &error)
{
	m_error = error;
}

const char * Socket::ErrorException::what() const throw()
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

string Socket::getLastSysError(void)
{
	LPTSTR pMsgBuf;
	string errmsg;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&pMsgBuf, 0, NULL);

#if defined(UNICODE)
	do {
		wstring tmp = (pMsgBuf);
		errmsg = string(tmp.begin(), tmp.end());
	} while (/* CONSTCOND */ 0);
#else
	errmsg = string(pMsgBuf);
#endif
	LocalFree(pMsgBuf);

	return errmsg;
}

#else

string Socket::getLastSysError(void)
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
{
}

Socket::Socket(socket_t sock, EndPoint endPoint)
	:m_socket(sock), m_endPoint(endPoint)
{
}

Socket::~Socket()
{
}

Socket::socket_t Socket::getSocket() const
{
	return m_socket;
}

void Socket::set(int level, int name, const void *arg, unsigned argLen)
{
	if (setsockopt(m_socket, level, name, (carg_t)arg, argLen) == SOCKET_ERROR)
		throw ErrorException(getLastSysError());
}

void Socket::bind(EndPoint location)
{
	const sockaddr_storage & addr = location.getAddr();
	unsigned addrlen = location.getAddrlen();

	if (::bind(m_socket, (sockaddr *)&addr, addrlen) == SOCKET_ERROR)
		throw ErrorException(getLastSysError());
}

void Socket::close()
{
	(void)closesocket(m_socket);
}

bool irccd::operator==(const Socket &s1, const Socket &s2)
{
	return s1.getSocket() == s2.getSocket();
}
