/*
 * Socket.h -- portable C++ socket wrappers
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

#ifndef _SOCKET_NG_H_
#define _SOCKET_NG_H_

/**
 * @file Socket.h
 * @brief Portable socket abstraction
 *
 * User may set the following variables before compiling these files:
 *
 * SOCKET_NO_WSA_INIT	- (bool) Set to false if you don't want Socket class to
 *			  automatically calls WSAStartup() when creating sockets.
 *
 *			  Otherwise, you will need to call Socket::init,
 *			  Socket::finish yourself.
 *
 * SOCKET_NO_SSL_INIT	- (bool) Set to false if you don't want OpenSSL to be
 *			  initialized when the first SocketSsl object is created.
 *
 * SOCKET_HAVE_POLL	- (bool) Set to true if poll(2) function is available.
 *
 *			  Note: on Windows, this is automatically set if the
 *			  _WIN32_WINNT variable is greater or equal to 0x0600.
 */

#include <cstring>
#include <exception>
#include <string>

#if defined(_WIN32)
#  include <atomic>
#  include <cstdlib>
#  include <mutex>

#  include <WinSock2.h>
#  include <WS2tcpip.h>
#else
#  include <cerrno>

#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <sys/types.h>

#  include <arpa/inet.h>

#  include <netinet/in.h>

#  include <fcntl.h>
#  include <netdb.h>
#  include <unistd.h>
#endif

class SocketAddress;

/**
 * @class SocketError
 * @brief Base class for sockets error
 */
class SocketError : public std::exception {
public:
	enum Code {
		WouldBlockRead,		///!< The operation would block for reading
		WouldBlockWrite,	///!< The operation would block for writing
		Timeout,		///!< The action did timeout
		System			///!< There is a system error
	};

	Code m_code;
	std::string m_function;
	std::string m_error;

	/**
	 * Constructor that use the last system error.
	 *
	 * @param code which kind of error
	 * @param function the function name
	 */
	SocketError(Code code, std::string function);

	/**
	 * Constructor that use the system error set by the user.
	 *
	 * @param code which kind of error
	 * @param function the function name
	 * @param error the error
	 */
	SocketError(Code code, std::string function, int error);

	/**
	 * Constructor that set the error specified by the user.
	 *
	 * @param code which kind of error
	 * @param function the function name
	 * @param error the error
	 */
	SocketError(Code code, std::string function, std::string error);

	/**
	 * Get which function has triggered the error.
	 *
	 * @return the function name (e.g connect)
	 */
	inline const std::string &function() const noexcept
	{
		return m_function;
	}

	/**
	 * The error code.
	 *
	 * @return the code
	 */
	inline Code code() const noexcept
	{
		return m_code;
	}

	/**
	 * Get the error (only the error content).
	 *
	 * @return the error
	 */
	const char *what() const noexcept
	{
		return m_error.c_str();
	}
};

/**
 * @enum SocketState
 * @brief Category of error
 */
enum class SocketState {
	Opened,				///!< Socket is opened
	Closed,				///!< Socket has been closed
	Bound,				///!< Socket is bound to address
	Connected,			///!< Socket is connected to an end point
	Disconnected,			///!< Socket is disconnected
	Timeout				///!< Timeout has occured in a waiting operation
};

/**
 * @class Socket
 * @brief Base socket class for socket operations
 */
class Socket {
public:
	/* {{{ Portable types */

	/*
	 * The following types are defined differently between Unix
	 * and Windows.
	 */
#if defined(_WIN32)
	using Handle	= SOCKET;
	using ConstArg	= const char *;
	using Arg	= char *;
#else
	using Handle	= int;
	using ConstArg	= const void *;
	using Arg	= void *;
#endif

	/* }}} */

	/* {{{ Portable constants */

	/*
	 * The following constants are defined differently from Unix
	 * to Windows.
	 */
#if defined(_WIN32)
	static constexpr const Handle Invalid	= INVALID_SOCKET;
	static constexpr const int Error	= SOCKET_ERROR;
#else
	static constexpr const int Invalid	= -1;
	static constexpr const int Error	= -1;
#endif

	/* }}} */

	/* {{{ Portable initialization */

	/*
	 * Initialization stuff.
	 *
	 * The function init and finish are threadsafe.
	 */
#if defined(_WIN32)
private:
	static std::mutex s_mutex;
	static std::atomic<bool> s_initialized;

public:
	static inline void finish() noexcept
	{
		WSACleanup();
	}

	static inline void initialize() noexcept
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		if (!s_initialized) {
			s_initialized = true;

			WSADATA wsa;
			WSAStartup(MAKEWORD(2, 2), &wsa);

			/*
			 * If SOCKET_WSA_NO_INIT is not set then the user
			 * must also call finish himself.
			 */
#if !defined(SOCKET_WSA_NO_INIT)
			std::atexit(finish);
#endif
		}
	}
#else
public:
	/**
	 * no-op.
	 */
	static inline void initialize() noexcept {}

	/**
	 * no-op.
	 */
	static inline void finish() noexcept {}
#endif

	/* }}} */

protected:
	Handle m_handle;
	SocketState m_state{SocketState::Opened};

public:
	/**
	 * Get the last socket system error. The error is set from errno or from
	 * WSAGetLastError on Windows.
	 *
	 * @return a string message
	 */
	static std::string syserror();

	/**
	 * Get the last system error.
	 *
	 * @param errn the error number (errno or WSAGetLastError)
	 * @return the error
	 */
	static std::string syserror(int errn);

	/**
	 * Construct a socket with an already created descriptor.
	 *
	 * @param handle the native descriptor
	 */
	inline Socket(Handle handle)
		: m_handle(handle)
		, m_state(SocketState::Opened)
	{
	}

	/**
	 * Create a socket handle.
	 *
	 * @param domain the domain AF_*
	 * @param type the type SOCK_*
	 * @param protocol the protocol
	 * @throw SocketError on failures
	 */
	Socket(int domain, int type, int protocol);

	/**
	 * Default destructor.
	 */
	virtual ~Socket() = default;

	/**
	 * Get the local name. This is a wrapper of getsockname().
	 *
	 * @return the address
	 * @throw SocketError on failures
	 */
	SocketAddress address() const;

	/**
	 * Set an option for the socket.
	 *
	 * @param level the setting level
	 * @param name the name
	 * @param arg the value
	 * @throw SocketError on error
	 */
	template <typename Argument>
	inline void set(int level, int name, const Argument &arg)
	{
#if defined(_WIN32)
		if (setsockopt(m_handle, level, name, (Socket::ConstArg)&arg, sizeof (arg)) == Error)
#else
		if (setsockopt(m_handle, level, name, (Socket::ConstArg)&arg, sizeof (arg)) < 0)
#endif
			throw SocketError(SocketError::System, "set");
	}

	/**
	 * Get an option for the socket.
	 *
	 * @param level the setting level
	 * @param name the name
	 * @throw SocketError on error
	 */
	template <typename Argument>
	inline Argument get(int level, int name)
	{
		Argument desired, result{};
		socklen_t size = sizeof (result);

#if defined(_WIN32)
		if (getsockopt(m_handle, level, name, (Socket::Arg)&desired, &size) == Error)
#else
		if (getsockopt(m_handle, level, name, (Socket::Arg)&desired, &size) < 0)
#endif
			throw SocketError(SocketError::System, "get");

		std::memcpy(&result, &desired, size);

		return result;
	}

	/**
	 * Get the native handle.
	 *
	 * @return the handle
	 * @warning Not portable
	 */
	inline Handle handle() const noexcept
	{
		return m_handle;
	}

	/**
	 * Get the socket state.
	 *
	 * @return
	 */
	inline SocketState state() const noexcept
	{
		return m_state;
	}

	/**
	 * Bind to an address.
	 *
	 * @param address the address
	 * @throw SocketError on any error
	 */
	void bind(const SocketAddress &address);

	/**
	 * Set the blocking mode, if set to false, the socket will be marked
	 * **non-blocking**.
	 *
	 * @param block set to false to mark **non-blocking**
	 * @throw SocketError on any error
	 */
	void setBlockMode(bool block);

	/**
	 * Close the socket.
	 */
	virtual void close();
};

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if they equals
 */
bool operator==(const Socket &s1, const Socket &s2);

/**
 * Compare two sockets, ideal for putting in a std::map.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 < s2
 */
bool operator<(const Socket &s1, const Socket &s2);

#endif // !_SOCKET_NG_H_
