/*
 * CommandQueue.h -- client command queue
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

#ifndef _COMMAND_QUEUE_H_
#define _COMMAND_QUEUE_H_

/**
 * @file CommandQueue.h
 * @brief Client command queue
 */

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <list>
#include <thread>

#include "command/Command.h"

namespace irccd {

/**
 * @class CommandQueue
 * @brief Queue of client events
 *
 * This class keeps all client events such as message, joining and such in a
 * queue, so if the user sends too much data quickly. The outgoing buffer
 * may be full, then we send the message again until it is sent.
 */
class CommandQueue {
public:
	using Ptr	= std::unique_ptr<Command>;

private:
	using Cond	= std::condition_variable;
	using Mutex	= std::mutex;
	using Lock	= std::unique_lock<Mutex>;
	using Queue	= std::list<Ptr>;
	using Thread	= std::thread;
	using Atomic	= std::atomic_bool;

private:
	Queue		m_cmds;
	Thread		m_thread;
	Cond		m_cond;
	Mutex		m_mutex;
	Atomic		m_alive;

	void routine();

public:
	/**
	 * Create a command queue with its thread.
	 */
	CommandQueue();

	/**
	 * Destroy the command queue and its thread.
	 */
	~CommandQueue();

	/**
	 * Add a new event in the queue.
	 *
	 * @param command the command
	 */
	template <typename Cmd>
	void add(Cmd &&command)
	{
		{
			Lock lock(m_mutex);

			m_cmds.push_back(std::unique_ptr<Cmd>(new Cmd(std::move(command))));
		}

		m_cond.notify_one();
	}

	/**
	 * Clear all commands.
	 */
	void clear();
};

} // !irccd

#endif // !_COMMAND_QUEUE_H_
