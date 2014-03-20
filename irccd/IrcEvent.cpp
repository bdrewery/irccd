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

#include <vector>
#include <string>

#include "lua/LuaServer.h"

#include "Logger.h"
#include "Irccd.h"
#include "IrcEvent.h"
#include "Luae.h"
#include "Server.h"

namespace irccd {

/* ---------------------------------------------------------
 * Private static members
 * --------------------------------------------------------- */

IrcEvent::Thread	IrcEvent::m_thread;
IrcEvent::Queue		IrcEvent::m_queue;
IrcEvent::Mutex		IrcEvent::m_mutex;
IrcEvent::Cond		IrcEvent::m_cv;

void IrcEvent::routine()
{
	auto &irccd = Irccd::getInstance();

	while (irccd.isRunning()) {
		Ptr event;

		{
			Lock lk(m_mutex);

			m_cv.wait(lk, [&] {
				return !irccd.isRunning() || m_queue.size() > 0;
			});

			/*
			 * If we stop, there is probably some Server::Ptr in
			 * events, we clear the queue so they are deleted if
			 * needed.
			 */ 
			if (!irccd.isRunning()) {
				while (m_queue.size() > 0)
					m_queue.pop();

				continue;
			}

			event = std::move(m_queue.front());
			m_queue.pop();
		}

		Plugin::forAll([&] (Plugin::Ptr &p) {
			try {
				event->action(p->getState());
			} catch (Plugin::ErrorException ex) {
				Logger::warn("plugin %s: %s",
				    ex.which().c_str(), ex.error().c_str());
			}
		});
	}
}

/* ---------------------------------------------------------
 * Protected members
 * --------------------------------------------------------- */

template <>
void IrcEvent::push<int>(lua_State *L, const int &value) const
{
	lua_pushinteger(L, value);
}

template <>
void IrcEvent::push<std::string>(lua_State *L, const std::string &value) const
{
	lua_pushlstring(L, value.c_str(), value.length());
}

template <>
void IrcEvent::push<std::vector<std::string>>(lua_State *L,
					      const std::vector<std::string> &value) const
{
	int i = 0;

	lua_createtable(L, value.size(), 0);
	for (const auto &s : value) {
		lua_pushlstring(L, s.c_str(), s.length());
		lua_rawseti(L, -2, ++i);
	}
}

template <>
void IrcEvent::push<Server::Ptr>(lua_State *L, const Server::Ptr &server) const
{
	Luae::pushShared<Server>(L, server, ServerType);
}

void IrcEvent::callFunction(lua_State *L, int np) const
{
	if (lua_pcall(L, np, 0, 0) != LUA_OK) {
		auto error = lua_tostring(L, -1);
		lua_pop(L, 1);
		
		throw Plugin::ErrorException(Process::info(L).name, error);
	}
}

/* ---------------------------------------------------------
 * Public static members
 * --------------------------------------------------------- */

void IrcEvent::start()
{
	m_thread = std::thread(routine);
}

void IrcEvent::add(Ptr&& event)
{
	m_mutex.lock();
	m_queue.push(std::move(event));
	m_mutex.unlock();
	m_cv.notify_all();
}

void IrcEvent::stop()
{
	m_cv.notify_all();
	m_thread.join();
}

/* ---------------------------------------------------------
 * Public members
 * --------------------------------------------------------- */

IrcEvent::~IrcEvent()
{
}

} // !irccd
