/*
 * EventQueue.h -- plugin event queue
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

#ifndef _IRCCD_EVENT_QUEUE_H_
#define _IRCCD_EVENT_QUEUE_H_

/**
 * @file EventQueue.h
 * @brief Plugin event queue
 */

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <thread>

#include <Singleton.h>

#include "event/Event.h"

namespace irccd {

/**
 * @class EventQueue
 * @brief The Lua event queue
 */
class EventQueue : public Singleton<EventQueue> {
private:
	SINGLETON(EventQueue);

	using Ptr	= std::unique_ptr<Event>;
	using Cond	= std::condition_variable;
	using Mutex	= std::mutex;
	using Lock	= std::unique_lock<Mutex>;
	using List	= std::list<Ptr>;
	using Thread	= std::thread;
	using Atomic	= std::atomic_bool;

private:
	Atomic	m_alive { true };
	Mutex	m_mutex;
	Cond	m_cond;
	List	m_list;
	Thread	m_thread;

	void routine();

	EventQueue();

public:
	/**
	 * Destroy the thread
	 */
	~EventQueue();

	/**
	 * Add a function to the event queue.
	 *
	 * @param args the arguments
	 */
	template <typename Evt, typename... Args>
	void add(Args&&... args)
	{
		{
			Lock lock(m_mutex);

			m_list.push_back(std::make_unique<Evt>(std::forward<Args>(args)...));
		}

		m_cond.notify_one();
	}
};

} // !irccd

#endif // !_IRCCD_EVENT_QUEUE_H_
