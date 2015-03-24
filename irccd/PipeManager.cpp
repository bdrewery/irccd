/*
 * PipeManager.cpp -- manage pipes
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

#include "PipeManager.h"
#include "Pipe.h"

namespace irccd {

std::shared_ptr<Pipe> PipeManager::get(const std::string &name)
{
	Lock lk(m_lock);

	if (m_pipes.find(name) == m_pipes.end())
		m_pipes[name] = std::make_shared<Pipe>(name);

	return m_pipes[name];
}

void PipeManager::remove(const std::string &name)
{
	Lock lk(m_lock);

	m_pipes.erase(name);
}

} // !irccd
