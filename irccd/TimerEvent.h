/*
 * TimerEvent.h -- timer event queue'ed to the main loop
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

#ifndef _IRCCD_TIMER_EVENT_H_
#define _IRCCD_TIMER_EVENT_H_

/**
 * @file TimerEvent.h
 * @brief Timer event in the event queue
 */

#include <cstdint>
#include <memory>

#include "Plugin.h"
#include "Timer.h"

namespace irccd {

/**
 * @enum TimerEventType
 * @brief Type of event for this timer
 */
enum class TimerEventType {
	Signal,
	End
};

/**
 * @class TimerEvent
 * @brief Provide an event in the irccd main event loop
 */
class TimerEvent {
private:
	std::shared_ptr<Plugin> m_plugin;
	std::shared_ptr<Timer> m_timer;
	TimerEventType m_type;

public:
	/**
	 * Construct a timer event.
	 *
	 * @param plugin which plugin
	 * @param timer which timer
	 * @param type which type of event
	 */
	TimerEvent(std::shared_ptr<Plugin> plugin,
		   std::shared_ptr<Timer> timer,
		   TimerEventType type = TimerEventType::Signal) noexcept;


	/**
	 * Get the associated plugin.
	 *
	 * @return the plugin
	 */
	inline const std::shared_ptr<Plugin> &plugin() const noexcept
	{
		return m_plugin;
	}

	/**
	 * Get the timer type.
	 *
	 * @return the type
	 */
	inline TimerEventType type() const noexcept
	{
		return m_type;
	}

	/**
	 * Execute the timer event.
	 */
	void call() noexcept;
};

} // !irccd

#endif // !_IRCCD_TIMER_EVENT_H_