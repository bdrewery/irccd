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

#include "Thread.h"

namespace irccd {

Thread::Ptr Thread::create()
{
	return std::shared_ptr<Thread>(new Thread());
}

Thread::Thread()
	: m_waited(false)
{
}

Thread::~Thread()
{
	detach();
}

void Thread::setState(LuaState &&state)
{
	Lock lk(m_mutex);

	m_state = std::move(state);
}

LuaState &Thread::getState()
{
	Lock lk(m_mutex);

	return m_state;
}

void Thread::setHandle(std::thread &&handle)
{
	Lock lk(m_mutex);

	m_handle = std::move(handle);
}

void Thread::wait()
{
	Lock lk(m_mutex);

	if (!m_waited) {
		m_handle.join();
		m_waited = true;
	}
}

void Thread::detach()
{
	Lock lk(m_mutex);

	try
	{
		m_handle.detach();
	}
	catch (...) { }
}

} // !irccd
