/*
 * SocketListener.cpp -- portable select() wrapper
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

#include <algorithm>
#include <set>

#include "SocketListener.h"

namespace irccd {

/* --------------------------------------------------------
 * Select implementation
 * -------------------------------------------------------- */

namespace backend {

std::vector<SocketStatus> Select::wait(const SocketTable &table, int ms)
{
	timeval maxwait, *towait;
	fd_set readset;
	fd_set writeset;

	FD_ZERO(&readset);
	FD_ZERO(&writeset);

	SocketAbstract::Handle max = 0;

	for (const auto &s : table) {
		if (s.second.second & SocketListener::Read) {
			FD_SET(s.first, &readset);
		}
		if (s.second.second & SocketListener::Write) {
			FD_SET(s.first, &writeset);
		}

		if (s.first > max) {
			max = s.first;
		}
	}

	maxwait.tv_sec = 0;
	maxwait.tv_usec = ms * 1000;

	// Set to nullptr for infinite timeout.
	towait = (ms < 0) ? nullptr : &maxwait;

	auto error = ::select(max + 1, &readset, &writeset, nullptr, towait);
	if (error == SocketAbstract::Error) {
		throw SocketError(SocketError::System, "select");
	}
	if (error == 0) {
		throw SocketError(SocketError::Timeout, "select", "Timeout while listening");
	}

	std::vector<SocketStatus> sockets;

	for (auto &c : table) {
		if (FD_ISSET(c.first, &readset)) {
			sockets.push_back(SocketStatus{*c.second.first, SocketListener::Read});
		}
		if (FD_ISSET(c.first, &writeset)) {
			sockets.push_back(SocketStatus{*c.second.first, SocketListener::Write});
		}
	}

	return sockets;
}

/* --------------------------------------------------------
 * Poll implementation
 * -------------------------------------------------------- */

#if defined(SOCKET_HAVE_POLL)

#if defined(_WIN32)
#  define poll WSAPoll
#endif

short Poll::topoll(int flags) const noexcept
{
	short result(0);

	if (flags & SocketListener::Read) {
		result |= POLLIN;
	}
	if (flags & SocketListener::Write) {
		result |= POLLOUT;
	}

	return result;
}

int Poll::toflags(short &event) const noexcept
{
	int flags = 0;

	/*
	 * Poll implementations mark the socket differently regarding
	 * the disconnection of a socket.
	 *
	 * At least, even if POLLHUP or POLLIN is set, recv() always
	 * return 0 so we mark the socket as readable.
	 */
	if ((event & POLLIN) || (event & POLLHUP)) {
		flags |= SocketListener::Read;
	}
	if (event & POLLOUT) {
		flags |= SocketListener::Write;
	}

	// Reset event for safety
	event = 0;

	return flags;
}

void Poll::set(const SocketTable &, SocketAbstract &s, int flags, bool add)
{
	if (add) {
		m_fds.push_back(pollfd{s.handle(), topoll(flags), 0});
	} else {
		auto it = std::find_if(m_fds.begin(), m_fds.end(), [&] (const struct pollfd &pfd) {
			return pfd.fd == s.handle();
		});

		it->events |= topoll(flags);
	}
}

void Poll::unset(const SocketTable &, SocketAbstract &s, int flags, bool remove)
{
	auto it = std::find_if(m_fds.begin(), m_fds.end(), [&] (const struct pollfd &pfd) {
		return pfd.fd == s.handle();
	});

	if (remove) {
		m_fds.erase(it);
	} else {
		it->events &= ~(topoll(flags));
	}
}

std::vector<SocketStatus> Poll::wait(const SocketTable &table, int ms)
{
	auto result = poll(m_fds.data(), m_fds.size(), ms);
	if (result == 0) {
		throw SocketError(SocketError::Timeout, "select", "Timeout while listening");
	}
	if (result < 0) {
		throw SocketError(SocketError::System, "poll");
	}

	std::vector<SocketStatus> sockets;
	for (auto &fd : m_fds) {
		if (fd.revents != 0) {
			sockets.push_back(SocketStatus{*table.at(fd.fd).first, toflags(fd.revents)});
		}
	}

	return sockets;
}

#endif // !SOCKET_HAVE_POLL

/* --------------------------------------------------------
 * Epoll implementation
 * -------------------------------------------------------- */

#if defined(SOCKET_HAVE_EPOLL)

uint32_t Epoll::toepoll(int flags) const noexcept
{
	uint32_t events = 0;

	if (flags & SocketListener::Read) {
		events |= EPOLLIN;
	}
	if (flags & SocketListener::Write) {
		events |= EPOLLOUT;
	}

	return events;
}

int Epoll::toflags(uint32_t events) const noexcept
{
	int flags = 0;

	if ((events & EPOLLIN) || (events & EPOLLHUP)) {
		flags |= SocketListener::Read;
	}
	if (events & EPOLLOUT) {
		flags |= SocketListener::Write;
	}

	return flags;
}

void Epoll::update(SocketAbstract &sc, int op, int flags)
{
	struct epoll_event ev;

	std::memset(&ev, 0, sizeof (struct epoll_event));

	ev.events = flags;
	ev.data.fd = sc.handle();

	if (epoll_ctl(m_handle, op, sc.handle(), &ev) < 0) {
		throw SocketError{SocketError::System, "epoll_ctl"};
	}
}

Epoll::Epoll()
	: m_handle(epoll_create1(0))
{
	if (m_handle < 0) {
		throw SocketError(SocketError::System, "epoll_create");
	}
}

Epoll::~Epoll()
{
	close(m_handle);
}

/*
 * Add a new epoll_event or just update it.
 */
void Epoll::set(const SocketTable &, SocketAbstract &sc, int flags, bool add)
{
	update(sc, add ? EPOLL_CTL_ADD : EPOLL_CTL_MOD, toepoll(flags));

	if (add) {
		m_events.resize(m_events.size() + 1);
	}
}

/*
 * Unset is a bit complicated case because SocketListener tells us which
 * flag to remove but to update epoll descriptor we need to pass
 * the effective flags that we want to be applied.
 *
 * So we put the same flags that are currently effective and remove the
 * requested one.
 */
void Epoll::unset(const SocketTable &table, SocketAbstract &sc, int flags, bool remove)
{
	if (remove) {
		update(sc, EPOLL_CTL_DEL, 0);
		m_events.resize(m_events.size() - 1);
	} else {
		update(sc, EPOLL_CTL_MOD, table.at(sc.handle()).second & ~(toepoll(flags)));
	}
}

std::vector<SocketStatus> Epoll::wait(const SocketTable &table, int ms)
{
	int ret = epoll_wait(m_handle, m_events.data(), m_events.size(), ms);
	std::vector<SocketStatus> result;

	if (ret == 0) {
		throw SocketError(SocketError::Timeout, "epoll_wait");
	}
	if (ret < 0) {
		throw SocketError(SocketError::System, "epoll_wait");
	}

	for (int i = 0; i < ret; ++i) {
		result.push_back(SocketStatus{*table.at(m_events[i].data.fd).first, toflags(m_events[i].events)});
	}

	return result;
}

#endif // !SOCKET_HAVE_EPOLL

/* --------------------------------------------------------
 * Kqueue implementation
 * -------------------------------------------------------- */

#if defined(SOCKET_HAVE_KQUEUE)

Kqueue::Kqueue()
	: m_handle(kqueue())
{
	if (m_handle < 0) {
		throw SocketError(SocketError::System, "kqueue");
	}
}

Kqueue::~Kqueue()
{
	close(m_handle);
}

void Kqueue::update(SocketAbstract &sc, int filter, int flags)
{
	struct kevent ev;

	EV_SET(&ev, sc.handle(), filter, flags, 0, 0, nullptr);

	if (kevent(m_handle, &ev, 1, nullptr, 0, nullptr) < 0) {
		throw SocketError(SocketError::System, "kevent");
	}
}

void Kqueue::set(const SocketTable &, SocketAbstract &sc, int flags, bool add)
{
	if (flags & SocketListener::Read) {
		update(sc, EVFILT_READ, EV_ADD | EV_ENABLE);
	}
	if (flags & SocketListener::Write) {
		update(sc, EVFILT_WRITE, EV_ADD | EV_ENABLE);
	}

	if (add) {
		m_result.resize(m_result.size() + 1);
	}
}

void Kqueue::unset(const SocketTable &, SocketAbstract &sc, int flags, bool remove)
{
	if (flags & SocketListener::Read) {
		update(sc, EVFILT_READ, EV_DELETE);
	}
	if (flags & SocketListener::Write) {
		update(sc, EVFILT_WRITE, EV_DELETE);
	}

	if (remove) {
		m_result.resize(m_result.size() - 1);
	}
}

std::vector<SocketStatus> Kqueue::wait(const SocketTable &table, int ms)
{
	std::vector<SocketStatus> sockets;
	timespec ts = { 0, 0 };
	timespec *pts = (ms <= 0) ? nullptr : &ts;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;

	int nevents = kevent(m_handle, nullptr, 0, &m_result[0], m_result.capacity(), pts);

	if (nevents == 0) {
		throw SocketError(SocketError::Timeout, "kevent");
	}
	if (nevents < 0) {
		throw SocketError(SocketError::System, "kevent");
	}

	for (int i = 0; i < nevents; ++i) {
		Socket *sc = table.at(m_result[i].ident).first;
		int flags = m_result[i].filter == EVFILT_READ ? SocketListener::Read : SocketListener::Write;

		sockets.push_back(SocketStatus{*sc, flags});
	}

	return sockets;
}

#endif // !SOCKET_HAVE_KQUEUE

} // !backend

} // !irccd
