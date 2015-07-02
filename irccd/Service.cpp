/*
 * Service.cpp -- provide interruptible select(2) based services
 *
 * Copyright (c) 2013, 2014, 2015 David Demelier <markand@malikania.fr>
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

#include <cstdio>
#include <string>

#include <Logger.h>

#include "Service.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace irccd {

void Service::run()
{
	while (m_state != ServiceState::Stopped) {
		/* 1. Wait for being ready */
		{
			std::unique_lock<std::mutex> lock(m_mutexState);

			m_condition.wait(lock, [&] () -> bool {
				return m_state != ServiceState::Paused;
			});
		}

		/*
		 * It is possible that the machine goes from Paused to Stopped, in that way we are here
		 * so be sure to break if needed.
		 */
		if (m_state == ServiceState::Stopped) {
			continue;
		}

		/* 2. Lock the listener in case someone wants to edit it */
		std::unique_ptr<SocketStatus> status;
		std::unique_ptr<Owner> whichOwner;
		{
			std::unique_lock<std::mutex> lock(m_mutexListener);

			try {
				status = std::make_unique<SocketStatus>(m_listener.wait(m_timeout));
				whichOwner = std::make_unique<Owner>(owner(status->socket));
			} catch (const SocketError &error) {
				if (error.code() == SocketError::Timeout) {
					onTimeout();
				} else {
					onError(error);
				}

				/* In any case, no more process */
				continue;
			}
		}

		switch (*whichOwner) {
		case Owner::Service:
			flush();
			break;
		case Owner::Acceptor:
			onAcceptor(status->socket);
			break;
		case Owner::Client:
			if (status->flags & SocketListener::Read) {
				onIncoming(status->socket);
			}
			if (status->flags & SocketListener::Write) {
				onOutgoing(status->socket);
			}
		default:
			break;
		}
	}
}

void Service::flush()
{
	{
		std::unique_lock<std::mutex> lock(m_mutexInterface);

		m_interface->flush();
	}
}

void Service::notify()
{
	{
		std::unique_lock<std::mutex> lock(m_mutexInterface);

		m_interface->notify();
	}
}

/*
 * TODO: bring back to life for Windows.
 */
Service::Service(int timeout, std::string name, std::string path)
#if defined(IRCCD_SYSTEM_WINDOWS)
	: m_interface(/* TODO */)
#else
	: m_timeout{timeout}
	, m_interface{std::make_unique<ServiceSocketUnix>(std::move(path))}
#endif
	, m_servname{std::move(name)}
{
	/* Do not forget to add signal socket */
	m_listener.set(m_interface->socket(), SocketListener::Read);
}

Service::~Service()
{
	assert(m_state == ServiceState::Stopped);
}

void Service::start()
{
	assert(m_state == ServiceState::Stopped);

	m_state = ServiceState::Running;
	m_thread = std::thread([this] () { run(); });
}

void Service::pause()
{
	assert(m_state == ServiceState::Running);

	{
		std::unique_lock<std::mutex> lock(m_mutexState);

		m_state = ServiceState::Paused;
	}

	// lock m_mutexInterface
	notify();
}

void Service::resume()
{
	assert(m_state == ServiceState::Paused);

	{
		std::unique_lock<std::mutex> lock(m_mutexState);

		m_state = ServiceState::Running;
	}

	m_condition.notify_one();
}

void Service::stop()
{
	assert(m_state == ServiceState::Running || m_state == ServiceState::Paused);

	// 1. Change the state to stopped and notify if it's currently paused
	ServiceState oldState = m_state;

	{
		std::unique_lock<std::mutex> lock(m_mutexState);

		m_state = ServiceState::Stopped;
	}

	// 1.5. Unblock the condition variable if it was paused.
	if (oldState == ServiceState::Paused) {
		m_condition.notify_one();
	}

	// 2. Notify the socket. (lock m_mutexInterface)
	m_interface->notify();

	// 3. Join the thread so it can be reused.
	try {
		m_thread.join();
	} catch (const std::exception &) {
	}
}

} // !irccd
