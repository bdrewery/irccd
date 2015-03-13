/*
 * SocketListener.h -- portable select() wrapper
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

#ifndef _SOCKET_LISTENER_NG_H_
#define _SOCKET_LISTENER_NG_H_

#include <chrono>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "Socket.h"

#if defined(_WIN32)
#  if _WIN32_WINNT >= 0x0600
#    define SOCKET_HAVE_POLL
#  endif
#else
#  define SOCKET_HAVE_POLL
#endif

/**
 * @enum SocketMethod
 * @brief The SocketMethod enum
 *
 * Select the method of polling. It is only a preferred method, for example if you
 * request for poll but it is not available, select will be used.
 */
enum class SocketMethod {
	Select,				//!< select(2) method, fallback
	Poll				//!< poll(2), everywhere possible
};

/**
 * @struct SocketStatus
 * @brief The SocketStatus struct
 *
 * Result of a select call, returns the first ready socket found with its
 * direction.
 */
class SocketStatus {
public:
	Socket	&socket;		//!< which socket is ready
	int	 direction;		//!< the direction
};

/**
 * @class SocketListenerInterface
 * @brief Implement the polling method
 */
class SocketListenerInterface {
public:
	/**
	 * Default destructor.
	 */
	virtual ~SocketListenerInterface() = default;

	/**
	 * Add a socket with a specified direction.
	 *
	 * @param s the socket
	 * @param direction the direction
	 */
	virtual void set(Socket &sc, int direction) = 0;

	/**
	 * Remove a socket with a specified direction.
	 *
	 * @param s the socket
	 * @param direction the direction
	 */
	virtual void unset(Socket &sc, int direction) = 0;

	/**
	 * Remove completely a socket.
	 *
	 * @param sc the socket to remove
	 */
	virtual void remove(Socket &sc) = 0;

	/**
	 * Remove all sockets.
	 */
	virtual void clear() = 0;

	/**
	 * Select one socket.
	 *
	 * @param ms the number of milliseconds to wait, -1 means forever
	 * @return the socket status
	 * @throw error::Failure on failure
	 * @throw error::Timeout on timeout
	 */
	virtual SocketStatus select(int ms) = 0;

	/**
	 * Select many sockets.
	 *
	 * @param ms the number of milliseconds to wait, -1 means forever
	 * @return a vector of ready sockets
	 * @throw error::Failure on failure
	 * @throw error::Timeout on timeout
	 */
	virtual std::vector<SocketStatus> selectMultiple(int ms) = 0;
};

/**
 * @class SocketListener
 * @brief Synchronous multiplexing
 *
 * Convenient wrapper around the select() system call.
 *
 * This class is implemented using a bridge pattern to allow different uses
 * of listener implementation.
 *
 * Currently, poll and select() are available.
 *
 * This wrappers takes abstract sockets as non-const reference but it does not
 * own them so you must take care that sockets are still alive until the
 * SocketListener is destroyed.
 */
class SocketListener final {
public:
#if defined(SOCKET_HAVE_POLL)
	static constexpr const SocketMethod PreferredMethod = SocketMethod::Poll;
#else
	static constexpr const SocketMethod PreferredMethod = SocketMethod::Select;
#endif

	static const int Read;
	static const int Write;

	using Map = std::map<std::reference_wrapper<Socket>, int>;
	using Iface = std::unique_ptr<SocketListenerInterface>;

private:
	Map m_map;
	Iface m_interface;

public:
	/**
	 * Move constructor.
	 *
	 * @param other the other object
	 */
	SocketListener(SocketListener &&other) = default;

	/**
	 * Move operator.
	 *
	 * @param other the other object
	 * @return this
	 */
	SocketListener &operator=(SocketListener &&other) = default;

	/**
	 * Create a socket listener.
	 *
	 * @param method the preferred method
	 */
	SocketListener(SocketMethod method = PreferredMethod);

	/**
	 * Create a listener from a list of sockets.
	 *
	 * @param list the list
	 */
	SocketListener(std::initializer_list<std::pair<std::reference_wrapper<Socket>, int>> list);

	/**
	 * Return an iterator to the beginning.
	 *
	 * @return the iterator
	 */
	inline auto begin() noexcept
	{
		return m_map.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto begin() const noexcept
	{
		return m_map.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cbegin() const noexcept
	{
		return m_map.cbegin();
	}

	/**
	 * Return an iterator to the end.
	 *
	 * @return the iterator
	 */
	inline auto end() noexcept
	{
		return m_map.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto end() const noexcept
	{
		return m_map.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline auto cend() const noexcept
	{
		return m_map.cend();
	}

	/**
	 * Add a socket to the listener.
	 *
	 * @param sc the socket
	 * @param direction (may be OR'ed)
	 */
	void set(Socket &sc, int direction);

	/**
	 * Unset a socket from the listener, only the direction is removed
	 * unless the two directions are requested.
	 *
	 * For example, if you added a socket for both reading and writing,
	 * unsetting the write direction will keep the socket for reading.
	 *
	 * @param sc the socket
	 * @param direction the direction (may be OR'ed)
	 * @see remove
	 */
	void unset(Socket &sc, int direction) noexcept;

	/**
	 * Remove completely the socket from the listener.
	 *
	 * @param sc the socket
	 */
	inline void remove(Socket &sc) noexcept
	{
		m_map.erase(sc);
		m_interface->remove(sc);
	}

	/**
	 * Remove all sockets.
	 */
	inline void clear() noexcept
	{
		m_map.clear();
		m_interface->clear();
	}

	/**
	 * Get the number of sockets in the listener.
	 */
	unsigned size() const noexcept
	{
		return m_map.size();
	}

	/**
	 * Select a socket. Waits for a specific amount of time specified as the duration.
	 *
	 * @param duration the duration
	 * @return the socket ready
	 */
	template <typename Rep, typename Ratio>
	inline SocketStatus select(const std::chrono::duration<Rep, Ratio> &duration)
	{
		auto cvt = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

		return m_interface->select(cvt.count());
	}

	/**
	 * Overload with milliseconds.
	 *
	 * @param timeout the optional timeout in milliseconds
	 * @return the socket ready
	 */
	inline SocketStatus select(int timeout = -1)
	{
		return m_interface->select(timeout);
	}

	/**
	 * Select multiple sockets.
	 *
	 * @param duration the duration
	 * @return the socket ready
	 */
	template <typename Rep, typename Ratio>
	inline std::vector<SocketStatus> selectMultiple(const std::chrono::duration<Rep, Ratio> &duration)
	{
		auto cvt = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

		return m_interface->selectMultiple(cvt.count());
	}

	/**
	 * Overload with milliseconds.
	 *
	 * @return the socket ready
	 */
	inline std::vector<SocketStatus> selectMultiple(int timeout = -1)
	{
		return m_interface->selectMultiple(timeout);
	}
};

#endif // !_SOCKET_LISTENER_NG_H_
