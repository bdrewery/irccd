/*
 * ServerManager.cpp -- manage servers
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

#include <chrono>

#include <common/Logger.h>

#include "Irccd.h"
#include "ServerManager.h"

using namespace std::literals::chrono_literals;

namespace irccd {

void ServerManager::cleaner()
{
	// cleanup dead servers
	while (Irccd::instance().isRunning()) {
		{
			Lock lk(m_lock);

			for (auto s = std::begin(m_servers); s != std::end(m_servers); ) {
				if (s->second->isDead()) {
					Logger::debug("server %s: removed", s->second->info().name.c_str());
					s = m_servers.erase(s);
				} else
					s++;
			}
		}

		std::this_thread::sleep_for(50ms);
	}
}

ServerManager::ServerManager()
	: m_thread(&ServerManager::cleaner, this)
{
}

ServerManager::~ServerManager()
{
	try {
		m_thread.join();
	} catch (const std::exception &ex) {
		Logger::warn("irccd: %s", ex.what());
	}
}

void ServerManager::add(std::shared_ptr<Server> &&server)
{
	Lock lk(m_lock);
	auto name = server->info().name;

	m_servers[name] = std::move(server);
	m_servers[name]->start();
}

void ServerManager::remove(const std::shared_ptr<Server> &server)
{
	Lock lk(m_lock);

	m_servers.erase(server->info().name);
}

bool ServerManager::has(const std::string &name) const
{
	Lock lk(m_lock);

	return m_servers.count(name) > 0;
}

std::shared_ptr<Server> ServerManager::get(const std::string &name) const
{
	Lock lk(m_lock);

	if (m_servers.count(name) == 0)
		throw std::out_of_range("server" + name + " not found");

	return m_servers.at(name);
}

} // !irccd
