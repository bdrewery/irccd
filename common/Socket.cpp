/*
 * Socket.cpp -- portable C++ socket wrappers
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#include <cstring>

#include "Socket.h"
#include "SocketAddress.h"

/* --------------------------------------------------------
 * System dependent code
 * -------------------------------------------------------- */

#if defined(_WIN32)

std::string Socket::syserror(int errn)
{
	LPSTR str = nullptr;
	std::string errmsg = "Unknown error";

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		errn,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&str, 0, NULL);


	if (str) {
		errmsg = std::string(str);
		LocalFree(str);
	}

	return errmsg;
}

#else

#include <cerrno>

std::string Socket::syserror(int errn)
{
	return strerror(errn);
}

#endif

std::string Socket::syserror()
{
#if defined(_WIN32)
	return syserror(WSAGetLastError());
#else
	return syserror(errno);
#endif
}

/* --------------------------------------------------------
 * SocketError class
 * -------------------------------------------------------- */

SocketError::SocketError(Code code, std::string function)
	: m_code(code)
	, m_function(std::move(function))
	, m_error(Socket::syserror())
{
}

SocketError::SocketError(Code code, std::string function, int error)
	: m_code(code)
	, m_function(std::move(function))
	, m_error(Socket::syserror(error))
{
}

SocketError::SocketError(Code code, std::string function, std::string error)
	: m_code(code)
	, m_function(std::move(function))
	, m_error(std::move(error))
{
}

/* --------------------------------------------------------
 * Socket class
 * -------------------------------------------------------- */

#if defined(_WIN32)
std::mutex Socket::s_mutex;
std::atomic<bool> Socket::s_initialized{false};
#endif

Socket::Socket(int domain, int type, int protocol)
{
#if defined(_WIN32) && !defined(SOCKET_NO_WSA_INIT)
	if (!s_initialized) {
		initialize();
	}
#endif

	m_handle = ::socket(domain, type, protocol);

	if (m_handle == Invalid) {
		throw SocketError(SocketError::System, "socket");
	}

	m_state = SocketState::Opened;
}

SocketAddress Socket::address() const
{
#if defined(_WIN32)
	int length;
#else
	socklen_t length;
#endif

	sockaddr_storage ss;

	if (getsockname(m_handle, (sockaddr *)&ss, &length) == Error)
		throw SocketError(SocketError::System, "getsockname");

	return SocketAddress(ss, length);
}

void Socket::bind(const SocketAddress &address)
{
	const auto &sa = address.address();
	const auto addrlen = address.length();

	if (::bind(m_handle, reinterpret_cast<const sockaddr *>(&sa), addrlen) == Error) {
		throw SocketError(SocketError::System, "bind");
	}

	m_state = SocketState::Bound;
}

void Socket::close()
{
#if defined(_WIN32)
	::closesocket(m_handle);
#else
	::close(m_handle);
#endif

	m_state = SocketState::Closed;
}

void Socket::setBlockMode(bool block)
{
#if defined(O_NONBLOCK) && !defined(_WIN32)
	int flags;

	if ((flags = fcntl(m_handle, F_GETFL, 0)) == -1) {
		flags = 0;
	}

	if (block) {
		flags &= ~(O_NONBLOCK);
	} else {
		flags |= O_NONBLOCK;
	}

	if (fcntl(m_handle, F_SETFL, flags) == Error) {
		throw SocketError(SocketError::System, "setBlockMode");
	}
#else
	unsigned long flags = (block) ? 0 : 1;

	if (ioctlsocket(m_handle, FIONBIO, &flags) == Error) {
		throw SocketError(SocketError::System, "setBlockMode");
	}
#endif
}

bool operator==(const Socket &s1, const Socket &s2)
{
	return s1.handle() == s2.handle();
}

bool operator<(const Socket &s1, const Socket &s2)
{
	return s1.handle() < s2.handle();
}
