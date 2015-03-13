/*
 * SocketSsl.h -- OpenSSL extension for sockets
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

#ifndef _SOCKET_SSL_NG_H_
#define _SOCKET_SSL_NG_H_

#include <atomic>
#include <mutex>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#include "SocketTcp.h"

/**
 * @class SocketSslOptions
 * @brief Options for SocketSsl
 */
class SocketSslOptions {
public:
	/**
	 * @brief Method
	 */
	enum {
		SSLv3	= (1 << 0),
		TLSv1	= (1 << 1),
		All	= (0xf)
	};

	int		method{All};		//!< The method
	std::string	certificate;		//!< The certificate path
	std::string	privateKey;		//!< The private key file
	bool		verify{false};		//!< Verify or not

	/**
	 * Default constructor.
	 */
	SocketSslOptions() = default;

	/**
	 * More advanced constructor.
	 *
	 * @param method the method requested
	 * @param certificate the certificate file
	 * @param key the key file
	 * @param verify set to true to verify
	 */
	SocketSslOptions(int method, std::string certificate, std::string key, bool verify = false)
		: method(method)
		, certificate(std::move(certificate))
		, privateKey(std::move(key))
		, verify(verify)
	{
	}
};

/**
 * @class SocketSsl
 * @brief SSL interface for sockets
 *
 * This class derives from SocketAbstractTcp and provide SSL support through OpenSSL.
 */
class SocketSsl : public SocketAbstractTcp {
public:
	using ContextHandle = std::unique_ptr<SSL_CTX, void (*)(SSL_CTX *)>;
	using SslHandle = std::unique_ptr<SSL, void (*)(SSL *)>;

private:
	static std::mutex s_sslMutex;
	static std::atomic<bool> s_sslInitialized;

	ContextHandle m_context{nullptr, nullptr};
	SslHandle m_ssl{nullptr, nullptr};
	SocketSslOptions m_options;

public:
	using SocketAbstractTcp::recv;
	using SocketAbstractTcp::waitRecv;
	using SocketAbstractTcp::send;
	using SocketAbstractTcp::waitSend;

	/**
	 * Close OpenSSL library.
	 */
	static inline void sslTerminate()
	{
		ERR_free_strings();
	}

	/**
	 * Open SSL library.
	 */
	static inline void sslInitialize()
	{
		std::lock_guard<std::mutex> lock(s_sslMutex);

		if (!s_sslInitialized) {
			s_sslInitialized = true;

			SSL_library_init();
			SSL_load_error_strings();

			std::atexit(sslTerminate);
		}
	}

	/**
	 * Create a SocketSsl from an already created one.
	 *
	 * @param handle the native handle
	 * @param context the context
	 * @param ssl the ssl object
	 */
	SocketSsl(Socket::Handle handle, SSL_CTX *context, SSL *ssl);

	/**
	 * Open a SSL socket with the specified family. Automatically
	 * use SOCK_STREAM as the type.
	 *
	 * @param family the family
	 * @param options the options
	 */
	SocketSsl(int family, int protocol, SocketSslOptions options = {});

	/**
	 * Accept a SSL TCP socket.
	 *
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketSsl accept();

	/**
	 * Accept a SSL TCP socket.
	 *
	 * @param info the client information
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketSsl accept(SocketAddress &info);

	/**
	 * Accept a SSL TCP socket.
	 *
	 * @param timeout the maximum timeout in milliseconds
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketSsl waitAccept(int timeout);

	/**
	 * Accept a SSL TCP socket.
	 *
	 * @param info the client information
	 * @param timeout the maximum timeout in milliseconds
	 * @return the socket
	 * @throw SocketError on error
	 */
	SocketSsl waitAccept(SocketAddress &info, int timeout);

	/**
	 * Connect to an end point.
	 *
	 * @param address the address
	 * @throw SocketError on error
	 */
	void connect(const SocketAddress &address);

	/**
	 * Connect to an end point.
	 *
	 * @param timeout the maximum timeout in milliseconds
	 * @param address the address
	 * @throw SocketError on error
	 */
	void waitConnect(const SocketAddress &address, int timeout);

	/**
	 * @copydoc SocketAbstractTcp::recv
	 */
	unsigned recv(void *data, unsigned length) override;

	/**
	 * @copydoc SocketAbstractTcp::recv
	 */
	unsigned waitRecv(void *data, unsigned length, int timeout) override;

	/**
	 * @copydoc SocketAbstractTcp::recv
	 */
	unsigned send(const void *data, unsigned length) override;

	/**
	 * @copydoc SocketAbstractTcp::recv
	 */
	unsigned waitSend(const void *data, unsigned length, int timeout) override;
};

#endif // !_SOCKET_SSL_NG_H_
