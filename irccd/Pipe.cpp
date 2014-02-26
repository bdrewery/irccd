/*
 * Pipe.cpp -- share data between threads
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

#include "Pipe.h"

namespace irccd {

Pipe::Pipes Pipe::pipes;
Pipe::Mutex Pipe::pipesMutex;

Pipe::Ptr Pipe::get(const std::string &name)
{
	Lock lk(pipesMutex);

	if (pipes.find(name) == pipes.end())
		pipes[name] = std::make_shared<Pipe>();

	return pipes[name];
}

void Pipe::push(const LuaValue &value)
{
	Lock lk(m_mutex);

	m_queue.push(value);
	m_cond.notify_all();
}

LuaValue Pipe::first()
{
	Lock lk(m_mutex);
	LuaValue v;

	if (m_queue.size() > 0)
		return m_queue.front();

	return v;
}

LuaValue Pipe::last()
{
	Lock lk(m_mutex);
	LuaValue v;

	if (m_queue.size() > 0)
		return m_queue.back();

	return v;
}

void Pipe::clear()
{
	Lock lk(m_mutex);

	while (!m_queue.empty())
		m_queue.pop();
}

bool Pipe::wait(unsigned long ms)
{
	bool ret = true;

	Lock lk(m_mutex);

	if (ms == 0)
		m_cond.wait(lk);
	else
		ret = m_cond.wait_for(lk, std::chrono::milliseconds(ms)) != std::cv_status::timeout;

	return ret;
}

void Pipe::list(Reader reader)
{
	Lock lk(m_mutex);

	while (!m_queue.empty()) {
		reader(m_queue.front());
		m_queue.pop();
	}
}

void Pipe::pop()
{
	Lock lk(m_mutex);

	m_queue.pop();
}

} // !irccd
