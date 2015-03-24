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
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "SocketListener.h"

/* --------------------------------------------------------
 * Select implementation
 * -------------------------------------------------------- */

namespace {

/**
 * @class SelectMethod
 * @brief Implements select(2)
 *
 * This class is the fallback of any other method, it is not preferred at all for many reasons.
 */
class SelectMethod final : public SocketListenerInterface {
private:
	std::map<Socket::Handle, std::pair<std::reference_wrapper<Socket>, int>> m_table;

public:
	void set(Socket &s, int direction) override
	{
		if (m_table.count(s.handle()) > 0) {
			m_table.at(s.handle()).second |= direction;
		} else {
			m_table.insert({s.handle(), {s, direction}});
		}

	}

	void unset(Socket &s, int direction) override
	{
		if (m_table.count(s.handle()) != 0) {
			m_table.at(s.handle()).second &= ~(direction);

			// If no read, no write is requested, remove it
			if (m_table.at(s.handle()).second == 0) {
				m_table.erase(s.handle());
			}
		}
	}

	void remove(Socket &sc) override
	{
		m_table.erase(sc.handle());
	}

	void clear() override
	{
		m_table.clear();
	}

	SocketStatus select(int ms) override
	{
		auto result = selectMultiple(ms);

		if (result.size() == 0) {
			throw SocketError(SocketError::System, "select", "No socket found");
		}

		return result[0];
	}

	std::vector<SocketStatus> selectMultiple(int ms) override
	{
		timeval maxwait, *towait;
		fd_set readset;
		fd_set writeset;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);

		Socket::Handle max = 0;

		for (auto &s : m_table) {
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
		if (error == Socket::Error) {
			throw SocketError(SocketError::System, "select");
		}
		if (error == 0) {
			throw SocketError(SocketError::Timeout, "select", "Timeout while listening");
		}

		std::vector<SocketStatus> sockets;

		for (auto &c : m_table) {
			if (FD_ISSET(c.first, &readset)) {
				sockets.push_back({ c.second.first, SocketListener::Read });
			}
			if (FD_ISSET(c.first, &writeset)) {
				sockets.push_back({ c.second.first, SocketListener::Write });
			}
		}

		return sockets;
	}
};

} // !namespace

/* --------------------------------------------------------
 * Poll implementation
 * -------------------------------------------------------- */

#if defined(SOCKET_HAVE_POLL)

#if defined(_WIN32)
#  include <Winsock2.h>
#  define poll WSAPoll
#else
#  include <poll.h>
#endif

namespace {

class PollMethod final : public SocketListenerInterface {
private:
	std::vector<pollfd> m_fds;
	std::map<Socket::Handle, std::reference_wrapper<Socket>> m_lookup;

	inline short topoll(int direction)
	{
		short result(0);

		if (direction & SocketListener::Read) {
			result |= POLLIN;
		}
		if (direction & SocketListener::Write) {
			result |= POLLOUT;
		}

		return result;
	}

	inline int todirection(short event)
	{
		int direction{};

		/*
		 * Poll implementations mark the socket differently regarding
		 * the disconnection of a socket.
		 *
		 * At least, even if POLLHUP or POLLIN is set, recv() always
		 * return 0 so we mark the socket as readable.
		 */
		if ((event & POLLIN) || (event & POLLHUP)) {
			direction |= SocketListener::Read;
		}
		if (event & POLLOUT) {
			direction |= SocketListener::Write;
		}

		return direction;
	}

public:
	void set(Socket &s, int direction) override
	{
		auto it = std::find_if(m_fds.begin(), m_fds.end(), [&] (const auto &pfd) { return pfd.fd == s.handle(); });

		// If found, add the new direction, otherwise add a new socket
		if (it != m_fds.end()) {
			it->events |= topoll(direction);
		} else {
			m_lookup.insert({s.handle(), s});
			m_fds.push_back({ s.handle(), topoll(direction), 0 });
		}
	}

	void unset(Socket &s, int direction) override
	{
		for (auto i = m_fds.begin(); i != m_fds.end();) {
			if (i->fd == s.handle()) {
				i->events &= ~(topoll(direction));

				if (i->events == 0) {
					m_lookup.erase(i->fd);
					i = m_fds.erase(i);
				} else {
					++i;
				}
			} else {
				++i;
			}
		}
	}

	void remove(Socket &s) override
	{
		auto it = std::find_if(m_fds.begin(), m_fds.end(), [&] (const auto &pfd) { return pfd.fd == s.handle(); });

		if (it != m_fds.end()) {
			m_fds.erase(it);
			m_lookup.erase(s.handle());
		}
	}

	void clear() override
	{
		m_fds.clear();
		m_lookup.clear();
	}

	SocketStatus select(int ms) override
	{
		auto result = poll(m_fds.data(), m_fds.size(), ms);
		if (result == 0) {
			throw SocketError(SocketError::Timeout, "select", "Timeout while listening");
		}
		if (result < 0) {
			throw SocketError(SocketError::System, "poll");
		}

		for (auto &fd : m_fds) {
			if (fd.revents != 0) {
				return { m_lookup.at(fd.fd), todirection(fd.revents) };
			}
		}

		throw SocketError(SocketError::System, "select", "No socket found");
	}

	std::vector<SocketStatus> selectMultiple(int ms) override
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
				sockets.push_back({ m_lookup.at(fd.fd), todirection(fd.revents) });
			}
		}

		return sockets;
	}
};

} // !namespace

#endif // !_SOCKET_HAVE_POLL

/* --------------------------------------------------------
 * SocketListener
 * -------------------------------------------------------- */

const int SocketListener::Read{1 << 0};
const int SocketListener::Write{1 << 1};

SocketListener::SocketListener(std::initializer_list<std::pair<std::reference_wrapper<Socket>, int>> list)
	: SocketListener()
{
	for (const auto &p : list) {
		set(p.first, p.second);
	}
}

SocketListener::SocketListener(SocketMethod method)
{
#if defined(SOCKET_HAVE_POLL)
	if (method == SocketMethod::Poll)
		m_interface = std::make_unique<PollMethod>();
	else
#endif
		m_interface = std::make_unique<SelectMethod>();

	(void)method;
}

void SocketListener::set(Socket &sc, int flags)
{
	if (m_map.count(sc) > 0) {
		m_map[sc] |= flags;
		m_interface->set(sc, flags);
	} else {
		m_map.insert({sc, flags});
		m_interface->set(sc, flags);
	}
}

void SocketListener::unset(Socket &sc, int flags) noexcept
{
	if (m_map.count(sc) > 0) {
		m_map[sc] &= ~(flags);
		m_interface->unset(sc, flags);

		// No more flags, remove it
		if (m_map[sc] == 0) {
			m_map.erase(sc);
		}
	}
}
