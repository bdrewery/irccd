/*
 * CommandQueue.cpp -- client command queue
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

#include "CommandQueue.h"

namespace irccd {

void CommandQueue::routine()
{
	while (m_alive) {
		Ptr *command = nullptr;

		{
			Lock lock(m_mutex);

			m_cond.wait(lock, [&] () -> bool {
				return !m_alive || m_cmds.size() > 0;
			});

			if (!m_alive)
				continue;

			command = &m_cmds.front();
		}

		/*
		 * IF RuleManager::shouldEncode(io)
		 *
		 * io->encode()
		 */

		if ((*command)->call()) {
			Lock lock(m_mutex);

			m_cmds.pop_front();
		}
	}
}

CommandQueue::CommandQueue()
{
	m_alive = true;
	m_thread = Thread(&CommandQueue::routine, this);
}

CommandQueue::~CommandQueue()
{
	m_alive = false;
	m_cond.notify_one();

	try {
		m_thread.join();
	} catch (...) { }
}

void CommandQueue::clear()
{
	Lock lock(m_mutex);

	m_cmds.clear();
}

} // !irccd
