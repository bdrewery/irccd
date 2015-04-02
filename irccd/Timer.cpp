/*
 * Timer.cpp -- Lua timers
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

#include <chrono>

#include <Logger.h>

#include "Timer.h"

namespace irccd {

void Timer::run()
{
	while (m_running) {
		std::unique_lock<std::mutex> lock(m_mutex);

		// Wait the timer delay or the interrupt
		m_condition.wait_for(lock, std::chrono::milliseconds(m_delay), [&] () {
			return m_running == false;
		});

		if (m_running) {
			// Signal process
			m_onSignal();

			if (m_type == TimerType::Single) {
				m_running = false;
			}
		}
	}

	// Finished
	m_onEnd();
}

Timer::Timer(TimerType type, int delay)
	: m_type(type)
	, m_delay(delay)
	, m_reference(-1)
{
	assert(!m_running);
}

Timer::~Timer()
{
	Logger::debug() << "timer: destroyed" << std::endl;

	try {
		if (m_running) {
			m_running = false;
			m_condition.notify_one();
		}

		m_thread.join();
	} catch (const std::exception &ex) {
		Logger::debug() << "timer: error: " << ex.what() << std::endl;
	}
}

void Timer::start()
{
	assert(!m_running);
	assert(m_onSignal);
	assert(m_onEnd);

	m_running = true;
	m_thread = std::thread(std::bind(&Timer::run, this));

	assert(m_running);
}

void Timer::stop()
{
	assert(m_running);

	m_running = false;
	m_condition.notify_one();

	assert(!m_running);
}

} // !irccd
