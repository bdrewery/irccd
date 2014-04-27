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

/**
 * @file EventQueue.h
 * @brief Plugin event queue
 */

#include <utility>

#include "Plugin.h"

namespace irccd {

/**
 * @class EventInfo
 * @brief An event information
 *
 * Used to check rules in the event queue.
 */
class EventInfo {
private:
	std::string	m_server;		//!< the server name
	std::string	m_channel;		//!< the channel name
	std::string	m_event;		//!< the event name

public:
	/**
	 * Default constructor.
	 */
	EventInfo() = default;

	/**
	 * Constructor with all parameters.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param event the event
	 */
	EventInfo(const std::string &server,
		  const std::string &channel,
		  const std::string &event)
		: m_server(server)
		, m_channel(channel)
		, m_event(event)
	{
	}

	/**
	 * Get the server name.
	 *
	 * @return the server
	 */
	const std::string &server() const;

	/**
	 * Get the channel name.
	 *
	 * @return the server
	 */
	const std::string &channel() const;

	/**
	 * Get the event name.
	 *
	 * @return the server
	 */
	const std::string &event() const;

	/**
	 * Check if the event info is empty.
	 */
	bool empty() const;
};

/**
 * @class EventQueue
 * @brief The Lua event queue
 */
class EventQueue {
public:
	/**
	 * The function to add in the event queue. It takes the plugin
	 * as parameter for the current plugin.
	 */
	using Function	= std::function<void (Plugin::Ptr)>;

private:
	using Pair	= std::pair<Function, EventInfo>;
	using Cond	= std::condition_variable;
	using Mutex	= std::mutex;
	using Lock	= std::unique_lock<Mutex>;
	using Queue	= std::queue<Pair>;
	using Thread	= std::thread;
	using Atomic	= std::atomic_bool;

private:
	static Atomic	alive;
	static Mutex	mutex;
	static Cond	cond;
	static Queue	queue;
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
	 * @param info the event information (for rules)
	 */
	static void add(const Function &event,
			const EventInfo &info = EventInfo());
};

} // !irccd

#endif // !_EVENT_QUEUE_H_
