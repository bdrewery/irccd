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

#include <cassert>
#include <cstdio>
#include <string>

#include <Logger.h>

#include "Service.h"

using namespace std::string_literals;

namespace irccd {

namespace {

constexpr const char CharReload = 'r';
constexpr const char CharStop = 's';

} // !namespace

Service::Service(std::string path)
#if defined(_WIN32)
	: m_signal(AF_INET, 0)
#else
	: m_signal(AF_LOCAL, 0)
#endif
{
	m_signal.set(SOL_SOCKET, SO_REUSEADDR, 1);

#if defined(IRCCD_SYSTEM_WINDOWS)
	m_signal.bind(address::Internet("127.0.0.1", 0, AF_INET));

	// Get the port
	auto port = ntohs(reinterpret_cast<const sockaddr_in &>(m_signal.address()).sin_port);
	m_address = address::Internet("127.0.0.1", port, AF_INET);

	// path not needed
	(void)path;
#else
	m_path = std::move(path);
	m_address = address::Unix(m_path, true);
	m_signal.bind(m_address);
#endif
}

Service::~Service()
{
	assert(!isRunning());

	m_signal.close();

#if !defined(IRCCD_SYSTEM_WINDOWS)
	remove(m_path.c_str());
#endif
}

bool Service::isService(Socket &s) const noexcept
{
	return m_signal.handle() == s.handle();
}

bool Service::isRunning() const noexcept
{
	return m_running;
}

Socket &Service::socket() noexcept
{
	return m_signal;
}

ServiceAction Service::action()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	char command;
	SocketAddress dummy;

	m_signal.recvfrom(&command, sizeof (char), dummy);

	switch (command) {
	case CharReload:
		Logger::debug() << "service: reloading" << std::endl;
		return ServiceAction::Reload;
	case CharStop:
		Logger::debug() << "service: stopping" << std::endl;
		return ServiceAction::Stop;
	default:
		break;
	}

	throw std::invalid_argument("unknown service command: '"s + command + "'"s);
}

void Service::reload()
{
	assert(isRunning());

	std::lock_guard<std::mutex> lock(m_mutex);

	m_signal.sendto(&CharReload, sizeof (char), m_address);
}

void Service::start()
{
	assert(!m_running);

	m_running = true;
	m_thread = std::thread(std::bind(&Service::run, this));
}

void Service::stop()
{
	assert(m_running);

	/*
	 * Try to tell the thread to stop by sending the appropriate
	 * stop command. If it succeed, the select will stops
	 * immediately and there will be no lag.
	 *
	 * If it fails, stop manually the thread, it will requires
	 * to wait that the listener timeout.
	 */
	try {
		std::lock_guard<std::mutex> lock(m_mutex);

		m_signal.sendto(&CharStop, sizeof (char), m_address);
	} catch (const std::exception &ex) {
		Logger::debug() << "irccd: failed to send: " << ex.what();
	}

	/* Join the thread */
	try {
		m_running = false;
		m_thread.join();
	} catch (const std::exception &ex) {
		Logger::debug() << "irccd: thread error: " << ex.what();
	}
}

} // !irccd
