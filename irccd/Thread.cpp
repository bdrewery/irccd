/*
 * Thread.cpp -- thread interface for Lua
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

#include <Logger.h>

#include "Plugin.h"

namespace irccd {

Thread::Ptr Thread::create()
{
	return std::shared_ptr<Thread>(new Thread);
}

void Thread::start(Thread::Ptr thread, int np)
{
	thread->m_thread = std::thread([=] () {
		if (lua_pcall(thread->m_state, np, 0, 0) != LUA_OK) {
			Logger::warn("thread: %s", lua_tostring(thread->m_state, -1));
			lua_pop(thread->m_state, 1);
		}
	});
}

Thread::Thread()
	: m_joined(false)
{
}

Thread::~Thread()
{
	Logger::debug("thread: destructor called");
}

void Thread::setState(LuaState &&state)
{
	m_state = std::move(state);
}

bool Thread::hasJoined() const
{
	return m_joined;
}

void Thread::join()
{
	m_thread.join();
	m_joined = true;
}

void Thread::detach()
{
	m_thread.detach();
	m_joined = true;
}

} // !irccd
