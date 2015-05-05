/*
 * Timer.h -- JS API timers
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

#ifndef _IRCCD_TIMER_H_
#define _IRCCD_TIMER_H_

/**
 * @file Timer.h
 * @brief Provides interval based timers for JavaScript
 */

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

#include <Signals.h>

namespace irccd {

/**
 * @enum TimerType
 * @brief Type of timer
 */
enum class TimerType {
	Single,			//!< The timer ends after execution
	Repeat			//!< The timer loops
};

/**
 * @class Timer
 * @brief Timer class
 *
 * The timer can be a SingleShot timer, which means that it will be called only
 * once and immediately destroyed after or periodic, which will be destroyed
 * only by user request or when the plugin is dead.
 *
 * The delay is configured in milliseconds and the user has choice to use any
 * delay needed.
 *
 * We use a condition variable to wait for the specified delay unless the timer
 * must be stopped.
 */
class Timer final {
public:
	/**
	 * Signal: onSignal
	 * ------------------------------------------------
	 *
	 * Called when the timeout expires.
	 */
	Signal<> onSignal;

	/**
	 * Signal: onEnd
	 * ------------------------------------------------
	 *
	 * Called when the timeout ends.
	 */
	Signal<> onEnd;

private:
	TimerType m_type;
	int m_delay;

	/* Thread management */
	std::atomic<bool> m_running{false};
	std::mutex m_mutex;
	std::condition_variable m_condition;
	std::thread m_thread;

	void run();

public:
	/**
	 * Timer constructor.
	 *
	 * The timer is not started, use start().
	 *
	 * @param type the timer type
	 * @param delay the delay in milliseconds
	 * @post isRunning() returns false
	 */
	Timer(TimerType type, int delay);

	/**
	 * Destructor, closes the thread.
	 */
	~Timer();

	/**
	 * Start the thread.
	 *
	 * This function should only be called from the irccd's event loop.
	 *
	 * @pre isRunning() must return false
	 * @pre onSignal() must have been called
	 * @pre onEnd() must have been called
	 * @note Not thread-safe
	 * @post isRunning() returns true
	 */
	void start();

	/**
	 * Stop the timer, may be used by the user to stop it.
	 *
	 * @pre isRunning() must return true
	 * @post isRunning() returns false
	 * @note Thread-safe
	 */
	void stop();

	/**
	 * Tells if the timer has still a running thread.
	 *
	 * @return true if still alive
	 * @note Thread-safe
	 */
	inline bool isRunning() const noexcept
	{
		return m_running;
	}
};

using Timers = std::unordered_set<std::shared_ptr<Timer>>;

} // !irccd

#endif // !_IRCCD_TIMER_H_
