/*
 * Pipe.h -- share data between threads
 * 
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>

#include "Luae.h"

namespace irccd
{

/**
 * @class Pipe
 * @brief Share data between threads
 *
 * This class is an helper to share data between threads.
 */
class Pipe
{
private:
	using Pipes	= std::unordered_map<
				std::string,
				std::shared_ptr<Pipe>
			  >;

	using Lock	= std::unique_lock<std::mutex>;
	using Mutex	= std::mutex;
	using Cond	= std::condition_variable;
	using Reader	= std::function<void (const LuaValue &)>;

public:
	using Ptr	= std::shared_ptr<Pipe>;
	using Queue	= std::queue<LuaValue>;

private:
	static Pipes pipes;
	static Mutex pipesMutex;

	Cond m_cond;
	Mutex m_mutex;
	Queue m_queue;

public:
	/**
	 * Get (or create) a pipe.
	 *
	 * @param name the name
	 * @return the pipe ready to be used
	 */
	static Ptr get(const std::string &name);

	/**
	 * Push some data to the pipe.
	 *
	 * @param value the value to push
	 */
	void push(const LuaValue &value);

	/**
	 * Get the first value.
	 *
	 * @return the first value
	 */
	const LuaValue &first();

	/**
	 * Get the last value.
	 *
	 * @return the last value
	 */
	const LuaValue &last();

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
	void list(Reader reader);

	/**
	 * Pop one value.
	 */
	void pop();
};

} // !irccd

#endif // !_PIPE_H_
