/*
 * Pipe.h -- share data between threads
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

#ifndef _PIPE_H_
#define _PIPE_H_

/**
 * @file Pipe.h
 * @brief Pipe for sharing data between threads
 */

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "Luae.h"

namespace irccd {

/**
 * @class Pipe
 * @brief Share data between threads
 *
 * This class is an helper to share data between threads.
 */
class Pipe final {
private:
	using Lock	= std::unique_lock<std::mutex>;
	using Mutex	= std::mutex;
	using Cond	= std::condition_variable;
	using Reader	= std::function<void (const LuaeValue &)>;

public:
	/**
	 * The queue of values.
	 */
	using Queue	= std::queue<LuaeValue>;

private:
	std::string	m_name;
	Cond		m_cond;
	Mutex		m_mutex;
	Queue		m_queue;

public:
	/**
	 * Construct a named pipe.
	 *
	 * @param name the name
	 */
	Pipe(std::string name);

	/**
	 * Destroy the pipe.
	 */
	~Pipe();

	/**
	 * Push some data to the pipe.
	 *
	 * @param value the value to push
	 */
	void push(const LuaeValue &value);

	/**
	 * Get the first value.
	 *
	 * @return the first value
	 */
	LuaeValue first();

	/**
	 * Get the last value.
	 *
	 * @return the last value
	 */
	LuaeValue last();

	/**
	 * Completely clear the queue.
	 */
	void clear();

	/**
	 * Wait for a maximum timeout.
	 *
	 * @param millisecond how much to wait (or 0)
	 * @return false if the timeout expired
	 */
	bool wait(unsigned long millisecond = 0);

	/**
	 * List all values, also pop them.
	 *
	 * @param reader the function to be called
	 */
	template <typename Func>
	void list(Func reader)
	{
		Lock lk(m_mutex);

		while (!m_queue.empty()) {
			reader(m_queue.front());
			m_queue.pop();
		}
	}

	/**
	 * Pop one value.
	 */
	void pop();
};

} // !irccd

#endif // !_PIPE_H_
