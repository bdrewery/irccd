/*
 * SocketSsl.cpp -- OpenSSL extension for sockets
 *
 * Copyright (c) 2013, David Demelier <markand@malikania.fr>
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
#include "SocketSsl.h"

namespace {

const SSL_METHOD *sslMethod(int mflags)
{
	if (mflags & SocketSslOptions::All)
		return SSLv23_method();
	if (mflags & SocketSslOptions::SSLv3)
		return SSLv3_method();
	if (mflags & SocketSslOptions::TLSv1)
		return TLSv1_method();

	return SSLv23_method();
}

inline std::string sslError(int error)
{
	return ERR_reason_error_string(error);
}

inline int toDirection(int error)
{
	if (error == SocketError::WouldBlockRead)
		return SocketListener::Read;
	if (error ==  SocketError::WouldBlockWrite)
		return SocketListener::Write;

	return 0;
}

} // !namespace

std::mutex SocketSsl::s_sslMutex;
std::atomic<bool> SocketSsl::s_sslInitialized{false};

SocketSsl::SocketSsl(Socket::Handle handle, SSL_CTX *context, SSL *ssl)
	: SocketAbstractTcp(handle)
	, m_context(context, SSL_CTX_free)
	, m_ssl(ssl, SSL_free)
{
#if !defined(SOCKET_NO_SSL_INIT)
	if (!s_sslInitialized) {
		sslInitialize();
	}
#endif
}

SocketSsl::SocketSsl(int family, int protocol, SocketSslOptions options)
	: SocketAbstractTcp(family, protocol)
	, m_options(std::move(options))
{
#if !defined(SOCKET_NO_SSL_INIT)
	if (!s_sslInitialized) {
		sslInitialize();
	}
#endif
}

void SocketSsl::connect(const SocketAddress &address)
{
	standardConnect(address);

	// Context first
	auto context = SSL_CTX_new(sslMethod(m_options.method));

	m_context = ContextHandle(context, SSL_CTX_free);

	// SSL object then
	auto ssl = SSL_new(context);

	m_ssl = SslHandle(ssl, SSL_free);

	SSL_set_fd(ssl, m_handle);

	auto ret = SSL_connect(ssl);

	if (ret <= 0) {
		auto error = SSL_get_error(ssl, ret);

		if (error == SSL_ERROR_WANT_READ) {
			throw SocketError(SocketError::WouldBlockRead, "connect", "Operation in progress");
		} else if (error == SSL_ERROR_WANT_WRITE) {
			throw SocketError(SocketError::WouldBlockWrite, "connect", "Operation in progress");
		} else {
			throw SocketError(SocketError::System, "connect", sslError(error));
		}
	}

	m_state = SocketState::Connected;
}

void SocketSsl::waitConnect(const SocketAddress &address, int timeout)
{
	try {
		// Initial try
		connect(address);
	} catch (const SocketError &ex) {
		if (ex.code() == SocketError::WouldBlockRead || ex.code() == SocketError::WouldBlockWrite) {
			SocketListener listener{{*this, toDirection(ex.code())}};

			listener.select(timeout);

			// Second try
			connect(address);
		} else {
			throw;
		}
	}
}

SocketSsl SocketSsl::accept()
{
	SocketAddress dummy;

	return accept(dummy);
}

SocketSsl SocketSsl::accept(SocketAddress &info)
{
	auto client = standardAccept(info);
	auto context = SSL_CTX_new(sslMethod(m_options.method));

	if (m_options.certificate.size() > 0)
		SSL_CTX_use_certificate_file(context, m_options.certificate.c_str(), SSL_FILETYPE_PEM);
	if (m_options.privateKey.size() > 0)
		SSL_CTX_use_PrivateKey_file(context, m_options.privateKey.c_str(), SSL_FILETYPE_PEM);
	if (m_options.verify && !SSL_CTX_check_private_key(context)) {
		client.close();
		throw SocketError(SocketError::System, "accept", "certificate failure");
	}

	// SSL object
	auto ssl = SSL_new(context);

	SSL_set_fd(ssl, client.handle());

	auto ret = SSL_accept(ssl);

	if (ret <= 0) {
		auto error = SSL_get_error(ssl, ret);

		if (error == SSL_ERROR_WANT_READ) {
			throw SocketError(SocketError::WouldBlockRead, "accept", "Operation would block");
		} else if (error == SSL_ERROR_WANT_WRITE) {
			throw SocketError(SocketError::WouldBlockWrite, "accept", "Operation would block");
		} else {
			throw SocketError(SocketError::System, "accept", sslError(error));
		}
	}

	return SocketSsl(client.handle(), context, ssl);
}

unsigned SocketSsl::recv(void *data, unsigned len)
{
	auto nbread = SSL_read(m_ssl.get(), data, len);

	if (nbread <= 0) {
		auto error = SSL_get_error(m_ssl.get(), nbread);

		if (error == SSL_ERROR_WANT_READ) {
			throw SocketError(SocketError::WouldBlockRead, "recv", "Operation would block");
		} else if (error == SSL_ERROR_WANT_WRITE) {
			throw SocketError(SocketError::WouldBlockWrite, "recv", "Operation would block");
		} else {
			throw SocketError(SocketError::System, "recv", sslError(error));
		}
	}

	return nbread;
}

unsigned SocketSsl::waitRecv(void *data, unsigned len, int timeout)
{
	SocketListener listener{{*this, SocketListener::Read}};

	listener.select(timeout);

	return recv(data, len);
}

unsigned SocketSsl::send(const void *data, unsigned len)
{
	auto nbread = SSL_write(m_ssl.get(), data, len);

	if (nbread <= 0) {
		auto error = SSL_get_error(m_ssl.get(), nbread);

		if (error == SSL_ERROR_WANT_READ) {
			throw SocketError(SocketError::WouldBlockRead, "send", "Operation would block");
		} else if (error == SSL_ERROR_WANT_WRITE) {
			throw SocketError(SocketError::WouldBlockWrite, "send", "Operation would block");
		} else {
			throw SocketError(SocketError::System, "send", sslError(error));
		}
	}

	return nbread;
}

unsigned SocketSsl::waitSend(const void *data, unsigned len, int timeout)
{
	SocketListener listener{{*this, SocketListener::Write}};

	listener.select(timeout);

	return send(data, len);
}

