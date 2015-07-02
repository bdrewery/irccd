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

namespace irccd {

/* --------------------------------------------------------
 * System dependent code
 * -------------------------------------------------------- */

#if defined(_WIN32)
const Socket::Handle SocketAbstract::Invalid{INVALID_SOCKET};
const int SocketAbstract::Error{SOCKET_ERROR};
#else
const int SocketAbstract::Invalid{-1};
const int SocketAbstract::Error{-1};
#endif

#if defined(_WIN32)

std::string SocketAbstract::syserror(int errn)
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

std::string SocketAbstract::syserror(int errn)
{
	return strerror(errn);
}

#endif

std::string SocketAbstract::syserror()
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
	: m_code{code}
	, m_function{std::move(function)}
	, m_error{SocketAbstract::syserror()}
{
}

SocketError::SocketError(Code code, std::string function, int error)
	: m_code{code}
	, m_function{std::move(function)}
	, m_error{SocketAbstract::syserror(error)}
{
}

SocketError::SocketError(Code code, std::string function, std::string error)
	: m_code{code}
	, m_function{std::move(function)}
	, m_error{std::move(error)}
{
}

/* --------------------------------------------------------
 * SocketAbstract class
 * -------------------------------------------------------- */

#if defined(_WIN32)
std::mutex SocketAbstract::s_mutex;
std::atomic<bool> SocketAbstract::s_initialized{false};
#endif

SocketAbstract::SocketAbstract(int domain, int type, int protocol)
{
#if defined(_WIN32) && !defined(SOCKET_NO_WSA_INIT)
	if (!s_initialized) {
		initialize();
	}
#endif

	m_handle = ::socket(domain, type, protocol);

	if (m_handle == Invalid) {
		throw SocketError{SocketError::System, "socket"};
	}

	m_state = SocketState::Opened;
}

SocketAbstract::SocketAbstract(SocketAbstract &&other) noexcept
{
	m_handle = other.m_handle;
	m_state = other.m_state;

	// Invalidate other
	other.m_handle = -1;
	other.m_state = SocketState::Closed;
}

SocketAbstract::~SocketAbstract()
{
	close();
}

void SocketAbstract::close()
{
	if (m_state != SocketState::Closed) {
#if defined(_WIN32)
		::closesocket(m_handle);
#else
		::close(m_handle);
#endif
		m_handle = Invalid;
		m_state = SocketState::Closed;
	}
}

void SocketAbstract::setBlockMode(bool block)
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
		throw SocketError{SocketError::System, "setBlockMode"};
	}
#else
	unsigned long flags = (block) ? 0 : 1;

	if (ioctlsocket(m_handle, FIONBIO, &flags) == Error) {
		throw SocketError{SocketError::System, "setBlockMode"};
	}
#endif
}

SocketAbstract &SocketAbstract::operator=(SocketAbstract &&other) noexcept
{
	m_handle = other.m_handle;
	m_state = other.m_state;

	// Invalidate other
	other.m_handle = Invalid;
	other.m_state = SocketState::Closed;

	return *this;
}

bool operator==(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() == s2.handle();
}

bool operator!=(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() != s2.handle();
}

bool operator<(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() < s2.handle();
}

bool operator>(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() > s2.handle();
}

bool operator<=(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() <= s2.handle();
}

bool operator>=(const SocketAbstract &s1, const SocketAbstract &s2)
{
	return s1.handle() >= s2.handle();
}

} // !irccd
