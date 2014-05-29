/*
 * EventQueue.h -- plugin event queue
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

#ifndef _EVENT_QUEUE_H_
#define _EVENT_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <thread>

/**
 * @file EventQueue.h
 * @brief Plugin event queue
 */

#include "event/Event.h"

namespace irccd {

/**
 * @class EventQueue
 * @brief The Lua event queue
 */
class EventQueue {
public:
	using Ptr	= std::unique_ptr<Event>;

private:
	using Cond	= std::condition_variable;
	using Mutex	= std::mutex;
	using Lock	= std::unique_lock<Mutex>;
	using List	= std::list<Ptr>;
	using Thread	= std::thread;
	using Atomic	= std::atomic_bool;

private:
	static Atomic	alive;
	static Mutex	mutex;
	static Cond	cond;
	static List	list;
	static Thread	thread;

	static void routine();

public:
	/**
	 * Start the event queue.
	 */
	static void start();

	/**
	 * Stop the event queue.
	 */
	static void stop();

	/**
	 * Add a function to the event queue.
	 *
	 * @param event the event function
	 */
	template <typename Evt>
	static void add(Evt &&event)
	{
		{
			Lock lock(mutex);

			list.push_back(std::unique_ptr<Evt>(new Evt(std::move(event))));
		}

		cond.notify_one();
	}

};

} // !irccd

#endif // !_EVENT_QUEUE_H_
