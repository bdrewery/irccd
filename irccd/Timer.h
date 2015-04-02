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

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

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
private:
	TimerType m_type;
	int m_delay;
	int m_reference;

	std::function<void ()> m_onSignal;
	std::function<void ()> m_onEnd;

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
	 * @param reference the JS function reference
	 * @post isRunning() returns false
	 */
	Timer(TimerType type, int delay);

	/**
	 * Destructor, closes the thread.
	 */
	~Timer();

	/**
	 * Set the reference.
	 *
	 * @param reference the JS function reference
	 * @pre isRunning() must return false
	 * @note Not thread-safe
	 */
	inline void setReference(int reference)
	{
		assert(!m_running);

		m_reference = reference;
	}

	/**
	 * Set the onSignal event, called when the timer expires.
	 *
	 * @param func the function
	 * @pre isRunning() must return false
	 * @note Not thread-safe
	 */
	inline void onSignal(std::function<void ()> func) noexcept
	{
		assert(!m_running);

		m_onSignal = std::move(func);
	}

	/**
	 * Set the onSignal event, called when the timer ends.
	 *
	 * @param func the function
	 * @pre isRunning() must return false
	 * @note Not thread-safe
	 */
	inline void onEnd(std::function<void ()> func) noexcept
	{
		assert(!m_running);

		m_onEnd = std::move(func);
	}

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
	 * Get the ECMAScript reference.
	 *
	 * @return the reference
	 * @note Thread-safe
	 */
	inline int reference() const noexcept
	{
		return m_reference;
	}

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
