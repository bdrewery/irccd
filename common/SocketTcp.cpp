/*
 * SocketTcp.cpp -- portable C++ socket wrappers
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

#include "SocketAddress.h"
#include "SocketListener.h"
#include "SocketTcp.h"

/* --------------------------------------------------------
 * SocketAbstractTcp
 * -------------------------------------------------------- */

void SocketAbstractTcp::listen(int max)
{
	if (::listen(m_handle, max) == Error) {
		throw SocketError(SocketError::System, "listen");
	}
}

Socket SocketAbstractTcp::standardAccept(SocketAddress &info)
{
	Socket::Handle handle;

	// Store the information
	sockaddr_storage address;
	socklen_t addrlen;

	addrlen = sizeof (sockaddr_storage);
	handle = ::accept(m_handle, reinterpret_cast<sockaddr *>(&address), &addrlen);

	if (handle == Invalid) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockRead, "accept", error);
		}

		throw SocketError(SocketError::System, "accept", error);
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockRead, "accept");
		}

		throw SocketError(SocketError::System, "accept");
#endif
	}

	info = SocketAddress(address, addrlen);

	return Socket(handle);
}

void SocketAbstractTcp::standardConnect(const SocketAddress &address)
{
	if (m_state == SocketState::Connected) {
		return;
	}

	auto &sa = address.address();
	auto addrlen = address.length();

	if (::connect(m_handle, reinterpret_cast<const sockaddr *>(&sa), addrlen) == Error) {
		/*
		 * Determine if the error comes from a non-blocking connect that cannot be
		 * accomplished yet.
		 */
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockWrite, "connect", error);
		}

		throw SocketError(SocketError::System, "connect", error);
#else
		if (errno == EINPROGRESS) {
			throw SocketError(SocketError::WouldBlockWrite, "connect");
		}

		throw SocketError(SocketError::System, "connect");
#endif
	}

	m_state = SocketState::Connected;
}

/* --------------------------------------------------------
 * SocketTcp
 * -------------------------------------------------------- */

SocketTcp SocketTcp::accept()
{
	SocketAddress dummy;

	return accept(dummy);
}

SocketTcp SocketTcp::accept(SocketAddress &info)
{
	return standardAccept(info);
}

void SocketTcp::connect(const SocketAddress &address)
{
	return standardConnect(address);
}

void SocketTcp::waitConnect(const SocketAddress &address, int timeout)
{
	if (m_state == SocketState::Connected) {
		return;
	}

	// Initial try
	try {
		connect(address);
	} catch (const SocketError &ex) {
		if (ex.code() == SocketError::WouldBlockWrite) {
			SocketListener listener{{*this, SocketListener::Write}};

			listener.select(timeout);

			// Socket is writable? Check if there is an error

			int error = get<int>(SOL_SOCKET, SO_ERROR);

			if (error) {
				throw SocketError(SocketError::System, "connect", error);
			}
		} else {
			throw;
		}
	}

	m_state = SocketState::Connected;
}

SocketTcp SocketTcp::waitAccept(int timeout)
{
	SocketAddress dummy;

	return waitAccept(dummy, timeout);
}

SocketTcp SocketTcp::waitAccept(SocketAddress &info, int timeout)
{
	SocketListener listener{{*this, SocketListener::Read}};

	listener.select(timeout);

	return accept(info);
}

unsigned SocketTcp::recv(void *data, unsigned dataLen)
{
	int nbread;

	nbread = ::recv(m_handle, (Socket::Arg)data, dataLen, 0);
	if (nbread == Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockRead, "recv", error);
		}

		throw SocketError(SocketError::System, "recv", error);
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockRead, "recv");
		}

		throw SocketError(SocketError::System, "recv");
#endif
	} else if (nbread == 0) {
		m_state = SocketState::Closed;
	}

	return (unsigned)nbread;
}

unsigned SocketTcp::waitRecv(void *data, unsigned length, int timeout)
{
	SocketListener listener{{*this, SocketListener::Read}};

	listener.select(timeout);

	return recv(data, length);
}

unsigned SocketTcp::send(const void *data, unsigned length)
{
	int nbsent;

	nbsent = ::send(m_handle, (Socket::ConstArg)data, length, 0);
	if (nbsent == Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockWrite, "send", error);
		}

		throw SocketError(SocketError::System, "send", error);
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError(SocketError::WouldBlockWrite, "send");
		}

		throw SocketError(SocketError::System, "send");
#endif
	}

	return (unsigned)nbsent;
}

unsigned SocketTcp::waitSend(const void *data, unsigned length, int timeout)
{
	SocketListener listener{{*this, SocketListener::Write}};

	listener.select(timeout);

	return send(data, length);
}
