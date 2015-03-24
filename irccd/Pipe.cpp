/*
 * Pipe.cpp -- share data between threads
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

#include <common/Logger.h>

#include "Pipe.h"
#include "PipeManager.h"

namespace irccd {

Pipe::Pipe(std::string name)
	: m_name(std::move(name))
{
}

Pipe::~Pipe()
{
	Logger::debug("pipe %s: destroyed", m_name.c_str());
	PipeManager::instance().remove(m_name);
}

void Pipe::push(const LuaeValue &value)
{
	Lock lk(m_mutex);

	m_queue.push(value);
	m_cond.notify_all();
}

LuaeValue Pipe::first()
{
	Lock lk(m_mutex);
	LuaeValue v;

	if (m_queue.size() > 0)
		return m_queue.front();

	return v;
}

LuaeValue Pipe::last()
{
	Lock lk(m_mutex);
	LuaeValue v;

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
	Lock lk(m_mutex);
	bool ret = true;

	if (ms == 0)
		m_cond.wait(lk);
	else
		ret = m_cond.wait_for(lk, std::chrono::milliseconds(ms)) != std::cv_status::timeout;

	return ret;
}

void Pipe::pop()
{
	Lock lk(m_mutex);

	m_queue.pop();
}

} // !irccd
