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
 * - **SOCKET_NO_WSA_INIT**:(bool) Set to false if you don't want Socket class to
 * automatically calls WSAStartup() when creating sockets. Otherwise, you will need to call
 * SocketAbstract::init, SocketAbstract::finish yourself.
 *
 */

#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
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

namespace irccd {

class SocketAddress;

/* --------------------------------------------------------
 * Socket types and errors
 * -------------------------------------------------------- */

/**
 * @class SocketError
 * @brief Base class for sockets error
 */
class SocketError : public std::exception {
public:
	/**
	 * @enum Code
	 * @brief Which kind of error
	 */
	enum Code {
		WouldBlockRead,		///!< The operation would block for reading
		WouldBlockWrite,	///!< The operation would block for writing
		Timeout,		///!< The action did timeout
		System			///!< There is a system error
	};

private:
	Code m_code;
	std::string m_function;
	std::string m_error;

public:
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

/* --------------------------------------------------------
 * Generic base sockets
 * -------------------------------------------------------- */

/**
 * @class SocketAbstract
 * @brief Base socket class for socket operations
 */
class SocketAbstract {
public:
	/* {{{ Portable types */

	/*
	 * The following types are defined differently between Unix
	 * and Windows.
	 */
#if defined(_WIN32)
	/**
	 * Socket type, SOCKET.
	 */
	using Handle	= SOCKET;

	/**
	 * Argument to pass to set.
	 */
	using ConstArg	= const char *;

	/**
	 * Argument to pass to get.
	 */
	using Arg	= char *;
#else
	/**
	 * Socket type, int.
	 */
	using Handle	= int;

	/**
	 * Argument to pass to set.
	 */
	using ConstArg	= const void *;

	/**
	 * Argument to pass to get.
	 */
	using Arg	= void *;
#endif

	/* }}} */

	/* {{{ Portable constants */

	/*
	 * The following constants are defined differently from Unix
	 * to Windows.
	 */
#if defined(_WIN32)
	/**
	 * Socket creation failure or invalidation.
	 */
	static const Handle Invalid;

	/**
	 * Socket operation failure.
	 */
	static const int Error;
#else
	/**
	 * Socket creation failure or invalidation.
	 */
	static const int Invalid;

	/**
	 * Socket operation failure.
	 */
	static const int Error;
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
	/**
	 * Calls WSACleanup.
	 */
	static inline void finish() noexcept
	{
		WSACleanup();
	}

	/**
	 * Initialize using WSAStartup.
	 */
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
			atexit(finish);
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
	Handle m_handle{Invalid};			//!< The native handle

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
	 * This create an invalid socket.
	 */
	inline SocketAbstract() noexcept
		: m_handle{Invalid}
	{
	}

	/**
	 * Construct a socket with an already created descriptor.
	 *
	 * @param handle the native descriptor
	 */
	explicit inline SocketAbstract(Handle handle) noexcept
		: m_handle{handle}
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
	SocketAbstract(int domain, int type, int protocol);

	/**
	 * Copy constructor deleted.
	 */
	SocketAbstract(const SocketAbstract &) = delete;

	/**
	 * Transfer ownership from other to this.
	 *
	 * @param other the other socket
	 */
	SocketAbstract(SocketAbstract &&other) noexcept;

	/**
	 * Default destructor.
	 */
	virtual ~SocketAbstract();

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
		if (setsockopt(m_handle, level, name, (SocketAbstract::ConstArg)&arg, sizeof (arg)) == Error)
#else
		if (setsockopt(m_handle, level, name, (SocketAbstract::ConstArg)&arg, sizeof (arg)) < 0)
#endif
			throw SocketError{SocketError::System, "set"};
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
		if (getsockopt(m_handle, level, name, (SocketAbstract::Arg)&desired, &size) == Error)
#else
		if (getsockopt(m_handle, level, name, (SocketAbstract::Arg)&desired, &size) < 0)
#endif
			throw SocketError{SocketError::System, "get"};

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
	 * Set the blocking mode, if set to false, the socket will be marked
	 * **non-blocking**.
	 *
	 * @param block set to false to mark **non-blocking**
	 * @throw SocketError on any error
	 */
	void setBlockMode(bool block);

	/**
	 * Close the socket.
	 *
	 * Automatically called from the destructor.
	 */
	virtual void close();

	/**
	 * Assignment operator forbidden.
	 *
	 * @return *this
	 */
	SocketAbstract &operator=(const SocketAbstract &) = delete;

	/**
	 * Transfer ownership from other to this. The other socket is left
	 * invalid and will not be closed.
	 *
	 * @param other the other socket
	 * @return this
	 */
	SocketAbstract &operator=(SocketAbstract &&other) noexcept;
};

/**
 * @class Socket
 * @brief Generic socket implementation
 *
 * This class can be used to bind a socket and access its address.
 *
 * @see SocketAbstractTcp
 * @see SocketAbstractUdp
 */
template <typename Address>
class Socket : public SocketAbstract {
public:
	/**
	 * Inherited constructors.
	 */
	using SocketAbstract::SocketAbstract;

	/**
	 * Default constructor.
	 */
	Socket() = default;

	/**
	 * Bind to an address.
	 *
	 * @param address the address
	 * @throw SocketError on any error
	 */
	inline void bind(const Address &address)
	{
		const auto &sa = address.address();
		const auto addrlen = address.length();

		if (::bind(m_handle, reinterpret_cast<const sockaddr *>(&sa), addrlen) == Error) {
			throw SocketError{SocketError::System, "bind"};
		}
	}

	/**
	 * Get the local name.
	 *
	 * @return the address
	 * @throw SocketError on failures
	 */
	inline Address getsockname() const
	{
		socklen_t length = sizeof (sockaddr_storage);
		sockaddr_storage ss;

		if (::getsockname(m_handle, (sockaddr *)&ss, &length) == Error) {
			throw SocketError{SocketError::System, "getsockname"};
		}

		return Address(ss, length);
	}
};

/* --------------------------------------------------------
 * TCP Sockets
 * -------------------------------------------------------- */

/**
 * @class SocketAbstractTcp
 * @brief Base class for TCP sockets
 * @see SocketTcp
 * @see SocketSsl
 */
template <typename Address>
class SocketAbstractTcp : public Socket<Address> {
protected:
	/**
	 * Do standard accept.
	 *
	 * @param info the address
	 * @return the connected handle
	 * @throw SocketError on error
	 */
	SocketAbstract::Handle standardAccept(Address &info);

	/**
	 * Do standard connect.
	 *
	 * @param info the address
	 * @throw SocketError on error
	 */
	void standardConnect(const Address &info);

public:
	/**
	 * Inherited constructors.
	 */
	using Socket<Address>::Socket;

	/**
	 * Default constructor.
	 */
	SocketAbstractTcp() = default;

	/**
	 * Construct a standard TCP socket. The type is automatically
	 * set to SOCK_STREAM.
	 *
	 * @param domain the domain
	 * @param protocol the protocol
	 * @throw SocketError on error
	 */
	inline SocketAbstractTcp(int domain, int protocol)
		: Socket<Address>(domain, SOCK_STREAM, protocol)
	{
	}

	/**
	 * Listen for pending connection.
	 *
	 * @param max the maximum number
	 */
	inline void listen(int max = 128)
	{
		if (::listen(Socket<Address>::m_handle, max) == SocketAbstract::Error) {
			throw SocketError{SocketError::System, "listen"};
		}
	}

	/**
	 * Receive some data.
	 *
	 * @param data the destination buffer
	 * @param length the buffer length
	 * @throw SocketError on error
	 */
	virtual unsigned recv(void *data, unsigned length) = 0;

	/**
	 * Send some data.
	 *
	 * @param data the data buffer
	 * @param length the buffer length
	 * @throw SocketError on error
	 */
	virtual unsigned send(const void *data, unsigned length) = 0;

	/**
	 * Overloaded function.
	 *
	 * @param count the number of bytes to receive
	 * @return the string
	 * @throw SocketError on error
	 */
	inline std::string recv(unsigned count)
	{
		std::string result;

		result.resize(count);
		auto n = recv(const_cast<char *>(result.data()), count);
		result.resize(n);

		return result;
	}

	/**
	 * Overloaded function.
	 *
	 * @param data the string to send
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	inline unsigned send(const std::string &data)
	{
		return send(data.c_str(), data.size());
	}
};

template <typename Address>
SocketAbstract::Handle SocketAbstractTcp<Address>::standardAccept(Address &info)
{
	SocketAbstract::Handle handle;

	// Store the information
	sockaddr_storage address;
	socklen_t addrlen;

	addrlen = sizeof (sockaddr_storage);
	handle = ::accept(SocketAbstract::m_handle, reinterpret_cast<sockaddr *>(&address), &addrlen);

	if (handle == SocketAbstract::Invalid) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "accept", error};
		}

		throw SocketError{SocketError::System, "accept", error};
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "accept"};
		}

		throw SocketError{SocketError::System, "accept"};
#endif
	}

	info = Address{address, addrlen};

	//return SocketTcp{handle};
	return handle;
}

template <typename Address>
void SocketAbstractTcp<Address>::standardConnect(const Address &address)
{
	auto &sa = address.address();
	auto addrlen = address.length();

	if (::connect(SocketAbstract::m_handle, reinterpret_cast<const sockaddr *>(&sa), addrlen) == SocketAbstract::Error) {
		/*
		 * Determine if the error comes from a non-blocking connect that cannot be
		 * accomplished yet.
		 */
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockWrite, "connect", error};
		}

		throw SocketError{SocketError::System, "connect", error};
#else
		if (errno == EINPROGRESS) {
			throw SocketError{SocketError::WouldBlockWrite, "connect"};
		}

		throw SocketError{SocketError::System, "connect"};
#endif
	}
}

/**
 * @class SocketTcp
 * @brief Standard implementation of TCP sockets
 *
 * This class is the basic implementation of TCP sockets.
 */
template <typename Address>
class SocketTcp : public SocketAbstractTcp<Address> {
public:
	/**
	 * Inherited constructors.
	 */
	using SocketAbstractTcp<Address>::SocketAbstractTcp;

	/**
	 * Default constructor.
	 */
	SocketTcp() = default;

	/**
	 * Connect to an end point.
	 *
	 * @param address the address
	 * @throw SocketError on error
	 */
	inline void connect(const Address &address)
	{
		SocketAbstractTcp<Address>::standardConnect(address);
	}

	/**
	 * Overloaded function.
	 */
	inline SocketTcp accept()
	{
		Address dummy;

		return accept(dummy);
	}

	/**
	 * Accept a clear TCP socket.
	 *
	 * @param info the client information
	 * @return the socket
	 * @throw SocketError on error
	 */
	inline SocketTcp accept(Address &info)
	{
		return SocketTcp{SocketAbstractTcp<Address>::standardAccept(info)};
	}

	/**
	 * @copydoc SocketAbstractTcp<Address>::recv
	 */
	using SocketAbstractTcp<Address>::recv;

	/**
	 * @copydoc SocketAbstractTcp<Address>::send
	 */
	using SocketAbstractTcp<Address>::send;

	/**
	 * @copydoc SocketAbstractTcp<Address>::recv
	 */
	unsigned recv(void *data, unsigned length) override;

	/**
	 * @copydoc SocketAbstractTcp<Address>::send
	 */
	unsigned send(const void *data, unsigned length) override;
};

template <typename Address>
unsigned SocketTcp<Address>::recv(void *data, unsigned dataLen)
{
	int nbread;

	nbread = ::recv(SocketAbstract::m_handle, (SocketAbstract::Arg)data, dataLen, 0);
	if (nbread == SocketAbstract::Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "recv", error};
		}

		throw SocketError{SocketError::System, "recv", error};
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "recv"};
		}

		throw SocketError{SocketError::System, "recv"};
#endif
	}

	return static_cast<unsigned>(nbread);
}

template <typename Address>
unsigned SocketTcp<Address>::send(const void *data, unsigned length)
{
	int nbsent;

	nbsent = ::send(SocketAbstract::m_handle, (SocketAbstract::ConstArg)data, length, 0);
	if (nbsent == SocketAbstract::Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockWrite, "send", error};
		}

		throw SocketError{SocketError::System, "send", error};
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockWrite, "send"};
		}

		throw SocketError{SocketError::System, "send"};
#endif
	}

	return static_cast<unsigned>(nbsent);
}

/* --------------------------------------------------------
 * UDP Sockets
 * -------------------------------------------------------- */

/**
 * @class SocketAbstractUdp
 * @brief Base class for UDP sockets
 * @see SocketUdp
 */
template <typename Address>
class SocketAbstractUdp : public Socket<Address> {
public:
	/**
	 * Inherited constructors.
	 */
	using Socket<Address>::Socket;

	/**
	 * Default constructor.
	 */
	SocketAbstractUdp() = default;

	/**
	 * Construct a UDP socket. The type is automatically set to SOCK_DGRAM.
	 *
	 * @param domain the domain (e.g AF_INET)
	 * @param protocol the protocol (usually 0)
	 */
	inline SocketAbstractUdp(int domain, int protocol)
		: Socket<Address>(domain, SOCK_DGRAM, protocol)
	{
	}

	/**
	 * Overloaded function.
	 *
	 * @param data the data
	 * @param address the address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	inline unsigned sendto(const std::string &data, const Address &address)
	{
		return sendto(data.c_str(), data.length(), address);
	}

	/**
	 * Overloaded function.
	 *
	 * @param count the maximum number of bytes to receive
	 * @param info the client information
	 * @return the string
	 * @throw SocketError on error
	 */
	inline std::string recvfrom(unsigned count, Address &info)
	{
		std::string result;

		result.resize(count);
		auto n = recvfrom(const_cast<char *>(result.data()), count, info);
		result.resize(n);

		return result;
	}

	/**
	 * Receive data from an end point.
	 *
	 * @param data the destination buffer
	 * @param length the buffer length
	 * @param info the client information
	 * @return the number of bytes received
	 * @throw SocketError on error
	 */
	virtual unsigned recvfrom(void *data, unsigned length, Address &info) = 0;

	/**
	 * Send data to an end point.
	 *
	 * @param data the buffer
	 * @param length the buffer length
	 * @param address the client address
	 * @return the number of bytes sent
	 * @throw SocketError on error
	 */
	virtual unsigned sendto(const void *data, unsigned length, const Address &address) = 0;
};

/**
 * @class SocketUdp
 * @brief Standard implementation of UDP sockets
 *
 * This class is the basic implementation of UDP sockets.
 */
template <typename Address>
class SocketUdp : public SocketAbstractUdp<Address> {
public:
	/**
	 * Inherited constructors.
	 */
	using SocketAbstractUdp<Address>::SocketAbstractUdp;

	/**
	 * Default constructor.
	 */
	SocketUdp() = default;

	/**
	 * @copydoc SocketAbstractUdp<Address>::recv
	 */
	using SocketAbstractUdp<Address>::recvfrom;

	/**
	 * @copydoc SocketAbstractUdp<Address>::send
	 */
	using SocketAbstractUdp<Address>::sendto;

	/**
	 * @copydoc SocketAbstractUdp<Address>::recvfrom
	 */
	unsigned recvfrom(void *data, unsigned length, Address &info) override;

	/**
	 * @copydoc SocketAbstractUdp<Address>::sendto
	 */
	unsigned sendto(const void *data, unsigned length, const Address &address) override;
};

template <typename Address>
unsigned SocketUdp<Address>::recvfrom(void *data, unsigned length, Address &info)
{
	int nbread;

	// Store information
	sockaddr_storage address;
	socklen_t addrlen;

	addrlen = sizeof (struct sockaddr_storage);
	nbread = ::recvfrom(SocketAbstract::m_handle, (SocketAbstract::Arg)data, length, 0, (sockaddr *)&address, &addrlen);

	info = Address{address, addrlen};

	if (nbread == SocketAbstract::Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "recvfrom", error};
		}

		throw SocketError{SocketError::System, "recvfrom", error};
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockRead, "recvfrom"};
		}

		throw SocketError{SocketError::System, "recvfrom"};
#endif
	}

	return static_cast<unsigned>(nbread);
}

template <typename Address>
unsigned SocketUdp<Address>::sendto(const void *data, unsigned length, const Address &info)
{
	int nbsent;

	nbsent = ::sendto(SocketAbstract::m_handle, (SocketAbstract::ConstArg)data, length, 0, (const sockaddr *)&info.address(), info.length());
	if (nbsent == SocketAbstract::Error) {
#if defined(_WIN32)
		int error = WSAGetLastError();

		if (error == WSAEWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockWrite, "sendto", error};
		}

		throw SocketError{SocketError::System, "sendto", error};
#else
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			throw SocketError{SocketError::WouldBlockWrite, "sendto"};
		}

		throw SocketError{SocketError::System, "sendto"};
#endif
	}

	return static_cast<unsigned>(nbsent);
}

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if they equals
 */
bool operator==(const SocketAbstract &s1, const SocketAbstract &s2);

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if they are different
 */
bool operator!=(const SocketAbstract &s1, const SocketAbstract &s2);

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 < s2
 */
bool operator<(const SocketAbstract &s1, const SocketAbstract &s2);

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 > s2
 */
bool operator>(const SocketAbstract &s1, const SocketAbstract &s2);

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 <= s2
 */
bool operator<=(const SocketAbstract &s1, const SocketAbstract &s2);

/**
 * Compare two sockets.
 *
 * @param s1 the first socket
 * @param s2 the second socket
 * @return true if s1 >= s2
 */
bool operator>=(const SocketAbstract &s1, const SocketAbstract &s2);

} // !irccd

#endif // !_SOCKET_NG_H_
