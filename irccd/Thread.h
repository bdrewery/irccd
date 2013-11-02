/*
 * Thread.h -- thread interface for Lua
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <thread>

#include "Luae.h"

namespace irccd
{

/**
 * @class Thread
 * @brief A thread inside a plugin
 */
class Thread
{
private:
	std::thread m_handle;
	std::mutex m_mutex;
	bool m_waited;
	LuaState m_state;

	Thread();

public:
	using Ptr	= std::shared_ptr<Thread>;
	using Lock	= std::lock_guard<std::mutex>;

	/**
	 * Create a new thread as a shared pointer.
	 *
	 * @return a thread object (nothing running)
	 */
	static Ptr create();

	/**
	 * Kills the threads by running its detach state.
	 */
	~Thread();

	/**
	 * Set the Lua state for the thread.
	 *
	 * @param state the Lua state to move
	 */
	void setState(LuaState &&state);

	/**
	 * Get the current Lua state
	 *
	 * @return the state
	 */
	LuaState &getState();

	/**
	 * Set the function handle for that thread.
	 *
	 * @param handle the thread to move
	 */
	void setHandle(std::thread &&handle);

	/**
	 * Wait the thread to finish.
	 */
	void wait();

	/**
	 * Detach the thread, the object can be safely destroyed.
	 */
	void detach();
};

} // !irccd

#endif // !_THREAD_H_
