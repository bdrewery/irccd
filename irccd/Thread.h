/*
 * Thread.h -- thread interface for Lua
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

#ifndef _IRCCD_THREAD_H_
#define _IRCCD_THREAD_H_

/**
 * @file Thread.h
 * @brief Irccd thread interface
 */

#include <thread>

#include "Luae.h"
#include "Process.h"

namespace irccd {

/**
 * @class Thread
 * @brief A thread inside a plugin
 */
class Thread final {
private:
	friend class Plugin;

	std::thread m_thread;
	std::shared_ptr<Process> m_process;
	bool m_joined;

public:
	/**
	 * Start a thread by calling the function already pushed
	 * with its parameters.
	 *
	 * @param thread the thread to start
	 * @param np the number of parameters pushed
	 */
	static void start(std::shared_ptr<Thread> &thread, int np);

	/**
	 * Create a new thread as a shared pointer.
	 *
	 * @return a thread object (nothing running)
	 */
	Thread();

	/**
	 * Default destructor.
	 */
	~Thread();

	/**
	 * Check if the thread has been joined or detached.
	 *
	 * @return true if any
	 */
	bool hasJoined() const;

	/**
	 * Wait the thread to finish.
	 */
	void join();

	/**
	 * Detach the thread, the object can be safely destroyed.
	 */
	void detach();

	/**
	 * Get the process.
	 *
	 * @return the process
	 */
	std::shared_ptr<Process> process() const;

	/**
	 * Convert to lua_State *
	 */
	operator lua_State *();
};

} // !irccd

#endif // !_IRCCD_THREAD_H_
