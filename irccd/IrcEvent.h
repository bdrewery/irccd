/*
 * IrcEvent.cpp -- IRC event passed through plugins
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

#ifndef _IRC_EVENT_H_
#define _IRC_EVENT_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include <lua.hpp>

#include "Plugin.h"
#include "Process.h"

namespace irccd {

/**
 * @class IrcEvent
 * @brief An event passed through Lua
 *
 * This class also handle IRC Event 
 */
class IrcEvent {
public:
	using Ptr	= std::unique_ptr<IrcEvent>;
	using Queue	= std::queue<Ptr>;
	using Thread	= std::thread;
	using Mutex	= std::mutex;
	using Lock	= std::unique_lock<Mutex>;
	using Cond	= std::condition_variable;

private:
	static Thread	m_thread;
	static Queue	m_queue;
	static Mutex	m_mutex;
	static Cond	m_cv;

	/*
	 * Thread routine for event management.
	 */
	static void routine();

	void pushObjects(lua_State *) const
	{
		// Dummy, stop recursion
	}

	template <typename T, typename... Args>
	void pushObjects(lua_State *L, T &value, Args&&... args) const
	{
		push(L, value);
		pushObjects(L, args...);
	}

protected:
	/**
	 * Template for pushing data to Lua.
	 * @param L the Lua state
	 * @param value the value
	 */
	template <typename T>
	void push(lua_State *L, const T &value) const;

	/**
	 * Call a function already on the stack with its parameters.
	 *
	 * @param L the Lua state
	 * @param np number of parameters already pushed
	 * @throw Plugin::ErrorException on error
	 */
	void callFunction(lua_State *L, int np) const;

	/**
	 * Call a function and push arguments before.
	 *
	 * @param L the Lua state
	 * @param func the function to call
	 * @param args the arguments
	 * @throw Plugin::ErrorException on error
	 */
	template <typename... Args>
	void call(lua_State *L, const std::string &func, Args&&... args) const
	{
		lua_getglobal(L, func.c_str());

		if (lua_type(L, -1) != LUA_TFUNCTION) {
			lua_pop(L, 1);
		} else {
			auto before = lua_gettop(L);
			auto after = 0;

			pushObjects(L, std::forward<Args>(args)...);
			after = lua_gettop(L);

			callFunction(L, after - before);
		}
	}

public:
	/**
	 * Start the event management thread.
	 */
	static void start();

	/**
	 * Add an event to the queue. This event will be called on every plugin
	 * so IrcEventLoad, IrcEventReload and IrcEventUnload must not be
	 * pushed.
	 *
	 * @param event the event
	 */
	static void add(Ptr&& event);

	/**
	 * Stop the thread, it ignore every event still in the queue.
	 */
	static void stop();
	
	/**
	 * Virtual destructor for child.
	 */
	virtual ~IrcEvent();

	/**
	 * Action to execute for the plugin.
	 *
	 * @param L the Lua state (from the plugin)
	 */
	virtual void action(lua_State *L) const = 0;
};

} // !irccd

#endif // _IRC_EVENT_H_
