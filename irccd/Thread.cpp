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

#include "Thread.h"

namespace irccd {

Thread::Ptr Thread::create()
{
	return std::shared_ptr<Thread>(new Thread);
}

void Thread::start(Thread::Ptr thread, int np)
{
	thread->m_thread = std::thread([=] () {
		lua_State *L = *thread->m_process;

		if (lua_pcall(L, np, 0, 0) != LUA_OK) {
			Logger::warn("thread: %s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	});
}

Thread::Thread()
	: m_joined(false)
{
	m_process = Process::create();
}

Thread::~Thread()
{
	Logger::debug("thread: destructor called");
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

Process::Ptr Thread::process() const
{
	return m_process;
}

Thread::operator lua_State *()
{
	return static_cast<lua_State *>(*m_process);
}

} // !irccd