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

/**
 * @file SocketListener.h
 * @brief Portable synchronous multiplexer
 *
 * Feature detection, multiple implementations may be avaible, for example,
 * Linux has poll, select and epoll.
 *
 * We assume that select(2) is always available.
 *
 * Of course, you can set the variables yourself if you test it with your
 * build system.
 *
 * - **SOCKET_HAVE_POLL**: Defined on all BSD, Linux. Also defined on Windows
 * if _WIN32_WINNT is set to 0x0600 or greater.
 *
 * - **SOCKET_HAVE_KQUEUE**: Defined on all BSD and Apple.
 * - **SOCKET_HAVE_EPOLL**: Defined on Linux only.
 */
#if defined(_WIN32)
#  if _WIN32_WINNT >= 0x0600
#    define SOCKET_HAVE_POLL
#  endif
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#  define SOCKET_HAVE_KQUEUE
#  define SOCKET_HAVE_POLL
#elif defined(__linux__)
#  define SOCKET_HAVE_EPOLL
#  define SOCKET_HAVE_POLL
#endif

/**
 * This sets the default backend to use depending on the system. The following
 * table summaries.
 *
 * The preference priority is ordered from left to right.
 *
 * | System        | Backend                 |
 * |---------------|-------------------------|
 * | Linux         | epoll(7)                |
 * | *BSD          | kqueue(2)               |
 * | Windows       | poll(2), select(2)      |
 * | Mac OS X      | kqueue(2)               |
 */

#if defined(_WIN32)
#  if defined(SOCKET_HAVE_POLL)
#    define SOCKET_DEFAULT_BACKEND backend::Poll
#  else
#    define SOCKET_DEFAULT_BACKEND backend::Select
#  endif
#elif defined(__linux__)
#  include <sys/epoll.h>
#  include <cstring>

#  define SOCKET_DEFAULT_BACKEND backend::Epoll
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#  include <sys/types.h>
#  include <sys/event.h>
#  include <sys/time.h>

#  define SOCKET_DEFAULT_BACKEND backend::Kqueue
#else
#  define SOCKET_DEFAULT_BACKEND backend::Select
#endif

#include <chrono>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "Socket.h"

#if defined(SOCKET_HAVE_POLL) && !defined(_WIN32)
#  include <poll.h>
#endif

namespace irccd {

/**
 * @struct SocketStatus
 * @brief The SocketStatus class
 *
 * Result of a select call, returns the first ready socket found with its
 * flags.
 */
class SocketStatus {
public:
	SocketAbstract &socket;		//!< which socket is ready
	int flags;			//!< the flags
};

/**
 * Table used in the socket listener to store which sockets have been
 * set in which directions.
 */
using SocketTable = std::map<SocketAbstract::Handle, std::pair<SocketAbstract *, int>>;

/**
 * @brief Namespace for predefined backends
 */
namespace backend {

/**
 * @class Select
 * @brief Implements select(2)
 *
 * This class is the fallback of any other method, it is not preferred at all for many reasons.
 */
class Select {
public:
	/**
	 * Backend identifier
	 */
	inline const char *name() const noexcept
	{
		return "select";
	}

	/**
	 * No-op, uses the SocketTable directly.
	 */
	inline void set(const SocketTable &, SocketAbstract &, int, bool) noexcept {}

	/**
	 * No-op, uses the SocketTable directly.
	 */
	inline void unset(const SocketTable &, SocketAbstract &, int, bool) noexcept {}

	/**
	 * Return the sockets
	 */
	std::vector<SocketStatus> wait(const SocketTable &table, int ms);
};

#if defined(SOCKET_HAVE_POLL)

/**
 * @class Poll
 * @brief Implements poll(2)
 *
 * Poll is widely supported and is better than select(2). It is still not the
 * best option as selecting the sockets is O(n).
 */
class Poll {
private:
	std::vector<pollfd> m_fds;

	short topoll(int flags) const noexcept;
	int toflags(short &event) const noexcept;

public:
	void set(const SocketTable &, SocketAbstract &sc, int flags, bool add);
	void unset(const SocketTable &, SocketAbstract &sc, int flags, bool remove);
	std::vector<SocketStatus> wait(const SocketTable &, int ms);

	/**
	 * Backend identifier
	 */
	inline const char *name() const noexcept
	{
		return "poll";
	}
};

#endif

#if defined(SOCKET_HAVE_EPOLL)

class Epoll {
private:
	int m_handle;
	std::vector<struct epoll_event> m_events;

	Epoll(const Epoll &) = delete;
	Epoll &operator=(const Epoll &) = delete;
	Epoll(const Epoll &&) = delete;
	Epoll &operator=(const Epoll &&) = delete;

	uint32_t toepoll(int flags) const noexcept;
	int toflags(uint32_t events) const noexcept;
	void update(SocketAbstract &sc, int op, int flags);

public:
	Epoll();
	~Epoll();
	void set(const SocketTable &, SocketAbstract &sc, int flags, bool add);
	void unset(const SocketTable &, SocketAbstract &sc, int flags, bool remove);
	std::vector<SocketStatus> wait(const SocketTable &table, int ms);

	/**
	 * Backend identifier
	 */
	inline const char *name() const noexcept
	{
		return "epoll";
	}
};

#endif

#if defined(SOCKET_HAVE_KQUEUE)

/**
 * @class Kqueue
 * @brief Implements kqueue(2)
 *
 * This implementation is available on all BSD and Mac OS X. It is better than
 * poll(2) because it's O(1), however it's a bit more memory consuming.
 */
class Kqueue {
private:
	std::vector<struct kevent> m_result;
	int m_handle;

	Kqueue(const Kqueue &) = delete;
	Kqueue &operator=(const Kqueue &) = delete;
	Kqueue(Kqueue &&) = delete;
	Kqueue &operator=(Kqueue &&) = delete;

	void update(SocketAbstract &sc, int filter, int flags);

public:
	Kqueue();
	~Kqueue();

	void set(const SocketTable &, SocketAbstract &sc, int flags, bool add);
	void unset(const SocketTable &, SocketAbstract &sc, int flags, bool remove);
	std::vector<SocketStatus> wait(const SocketTable &, int ms);

	/**
	 * Backend identifier
	 */
	inline const char *name() const noexcept
	{
		return "kqueue";
	}
};

#endif

} // !backend

/**
 * @class SocketListenerAbstract
 * @brief Synchronous multiplexing
 *
 * Convenient wrapper around the select() system call.
 *
 * This class is implemented using a bridge pattern to allow different uses
 * of listener implementation.
 *
 * You should not reinstanciate a new SocketListener at each iteartion of your
 * main loop as it can be extremely costly. Instead use the same listener that
 * you can safely modify on the fly.
 *
 * Currently, poll, epoll, select and kqueue are available.
 *
 * To implement the backend, the following functions must be available:
 *
 * # Set
 *
 * @code
 * void set(const SocketTable &, const SocketAbstract &sc, int flags, bool add);
 * @endcode
 *
 * This function, takes the socket to be added and the flags. The flags are
 * always guaranteed to be correct and the function will never be called twice
 * even if the user tries to set the same flag again.
 *
 * An optional add argument is added for backends which needs to do different
 * operation depending if the socket was already set before or if it is the
 * first time (e.g EPOLL_CTL_ADD vs EPOLL_CTL_MOD for epoll(7).
 *
 * # Unset
 *
 * @code
 * void unset(const SocketTable &, const SocketAbstract &sc, int flags, bool remove);
 * @endcode
 *
 * Like set, this function is only called if the flags are actually set and will
 * not be called multiple times.
 *
 * Also like set, an optional remove argument is set if the socket is being
 * completely removed (e.g no more flags are set for this socket).
 *
 * # Wait
 *
 * @code
 * std::vector<SocketStatus> wait(const SocketTable &, int ms);
 * @encode
 *
 * Wait for the sockets to be ready with the specified milliseconds. Must return a list of SocketStatus,
 * may throw any exceptions.
 *
 * # Name
 *
 * @code
 * inline const char *name() const noexcept
 * @endcode
 *
 * Returns the backend name. Usually the class in lower case.
 */
template <typename Backend = SOCKET_DEFAULT_BACKEND>
class SocketListenerAbstract final {
public:
	/**
	 * Mark the socket for read operation.
	 */
	static const int Read;

	/**
	 * Mark the socket for write operation.
	 */
	static const int Write;

private:
	Backend m_backend;
	SocketTable m_table;

public:
	/**
	 * Construct an empty listener.
	 */
	SocketListenerAbstract() = default;

	/**
	 * Get the backend.
	 *
	 * @return the backend
	 */
	inline const Backend &backend() const noexcept
	{
		return m_backend;
	}

	/**
	 * Get the non-modifiable table.
	 *
	 * @return the table
	 */
	inline const SocketTable &table() const noexcept
	{
		return m_table;
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline SocketTable::const_iterator begin() const noexcept
	{
		return m_table.begin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline SocketTable::const_iterator cbegin() const noexcept
	{
		return m_table.cbegin();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline SocketTable::const_iterator end() const noexcept
	{
		return m_table.end();
	}

	/**
	 * Overloaded function.
	 *
	 * @return the iterator
	 */
	inline SocketTable::const_iterator cend() const noexcept
	{
		return m_table.cend();
	}

	/**
	 * Add or update a socket to the listener.
	 *
	 * If the socket is already placed with the appropriate flags, the
	 * function is a no-op.
	 *
	 * If incorrect flags are passed, the function does nothing.
	 *
	 * @param sc the socket
	 * @param flags (may be OR'ed)
	 * @throw SocketError if the backend failed to set
	 */
	void set(SocketAbstract &sc, int flags);

	/**
	 * Unset a socket from the listener, only the flags is removed
	 * unless the two flagss are requested.
	 *
	 * For example, if you added a socket for both reading and writing,
	 * unsetting the write flags will keep the socket for reading.
	 *
	 * @param sc the socket
	 * @param flags the flags (may be OR'ed)
	 * @see remove
	 */
	void unset(SocketAbstract &sc, int flags);

	/**
	 * Remove completely the socket from the listener.
	 *
	 * It is a shorthand for unset(sc, SocketListener::Read | SocketListener::Write);
	 *
	 * @param sc the socket
	 */
	inline void remove(SocketAbstract &sc)
	{
		unset(sc, Read | Write);
	}

	/**
	 * Remove all sockets.
	 */
	inline void clear()
	{
		while (!m_table.empty()) {
			remove(m_table.begin()->second.first);
		}
	}

	/**
	 * Get the number of sockets in the listener.
	 */
	inline SocketTable::size_type size() const noexcept
	{
		return m_table.size();
	}

	/**
	 * Select a socket. Waits for a specific amount of time specified as the duration.
	 *
	 * @param duration the duration
	 * @return the socket ready
	 */
	template <typename Rep, typename Ratio>
	inline SocketStatus wait(const std::chrono::duration<Rep, Ratio> &duration)
	{
		auto cvt = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

		return m_backend.wait(m_table, cvt.count())[0];
	}

	/**
	 * Overload with milliseconds.
	 *
	 * @param timeout the optional timeout in milliseconds
	 * @return the socket ready
	 */
	inline SocketStatus wait(int timeout = -1)
	{
		return wait(std::chrono::milliseconds(timeout));
	}

	/**
	 * Select multiple sockets.
	 *
	 * @param duration the duration
	 * @return the socket ready
	 */
	template <typename Rep, typename Ratio>
	inline std::vector<SocketStatus> waitMultiple(const std::chrono::duration<Rep, Ratio> &duration)
	{
		auto cvt = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

		return m_backend.wait(m_table, cvt.count());
	}

	/**
	 * Overload with milliseconds.
	 *
	 * @return the socket ready
	 */
	inline std::vector<SocketStatus> waitMultiple(int timeout = -1)
	{
		return waitMultiple(std::chrono::milliseconds(timeout));
	}
};

template <typename Backend>
void SocketListenerAbstract<Backend>::set(SocketAbstract &sc, int flags)
{
	/* Invalid or useless flags */
	if (flags == 0 || flags > 0x3)
		return;

	auto it = m_table.find(sc.handle());

	/*
	 * Do not update the table if the backend failed to add
	 * or update.
	 */
	if (it == m_table.end()) {
		m_backend.set(m_table, sc, flags, true);
		m_table.emplace(sc.handle(), std::make_pair(std::addressof(sc), flags));
	} else {
		if ((flags & Read) && (it->second.second & Read)) {
			flags &= ~(Read);
		}
		if ((flags & Write) && (it->second.second & Write)) {
			flags &= ~(Write);
		}

		/* Still need a call? */
		if (flags != 0) {
			m_backend.set(m_table, sc, flags, false);
			it->second.second |= flags;
		}
	}
}

template <typename Backend>
void SocketListenerAbstract<Backend>::unset(SocketAbstract &sc, int flags)
{
	auto it = m_table.find(sc.handle());

	/* Invalid or useless flags */
	if (flags == 0 || flags > 0x3 || it == m_table.end())
		return;

	/*
	 * Like set, do not update if the socket is already at the appropriate
	 * state.
	 */
	if ((flags & Read) && !(it->second.second & Read)) {
		flags &= ~(Read);
	}
	if ((flags & Write) && !(it->second.second & Write)) {
		flags &= ~(Write);
	}

	if (flags != 0) {
		/* Determine if it's a complete removal */
		bool removal = ((it->second.second) & ~(flags)) == 0;

		m_backend.unset(m_table, sc, flags, removal);

		if (removal) {
			m_table.erase(it);
		} else {
			it->second.second &= ~(flags);
		}
	}
}

/**
 * Helper to use the default.
 */
using SocketListener = SocketListenerAbstract<>;

template <typename Backend>
const int SocketListenerAbstract<Backend>::Read{1 << 0};

template <typename Backend>
const int SocketListenerAbstract<Backend>::Write{1 << 1};

} // !irccd

#endif // !_SOCKET_LISTENER_NG_H_
