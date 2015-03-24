/*
 * ServerManager.cpp -- manage servers
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

#include <Logger.h>
#include <Socket.h>

#include "ServerManager.h"

namespace irccd {

void ServerManager::run() noexcept
{
	while (m_running) {
		fd_set setinput;
		fd_set setoutput;
		int max = 0;

		FD_ZERO(&setinput);
		FD_ZERO(&setoutput);

		// Protect the list of servers while preparing the set.
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			// 1. Update server states and flush their pending commands
			for (auto &pair : m_servers) {
				pair.second->update();
				pair.second->flush();
			}

			// 2. Remove dead servers
			for (auto it = m_servers.begin(); it != m_servers.end(); ) {
				if (it->second->state() == ServerState::Dead) {
					Logger::debug("server %s: destroyed", it->second->info().name.c_str());
					it = m_servers.erase(it);
				} else {
					++ it;
				}
			}

			// 3. Update the sets
			for (auto &pair : m_servers) {
				pair.second->prepare(setinput, setoutput, max);
			}
		}

		// 4. Do the selection
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 250000;

		int error = select(max + 1, &setinput, &setoutput, nullptr, &tv);

		// Protect the set while processing the sessions.
		{
			std::lock_guard<std::mutex> lock(m_mutex);

			if (error < 0) {
				Logger::warn("irccd: %s", Socket::syserror().c_str());
			} else {
				for (auto &pair : m_servers) {
					pair.second->process(setinput, setoutput);
				}
			}
		}
	}
}

void ServerManager::onChannelNotice(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string notice)
{
	Logger::debug("server %s: onChannelNotice: origin=%s, channel=%s, notice=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), notice.c_str());
}

void ServerManager::onConnect(std::shared_ptr<Server> server)
{
	Logger::debug("server %s: onConnect", server->info().name.c_str());
}

void ServerManager::onInvite(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target)
{
	Logger::debug("server %s: onInvite: origin=%s, channel=%s, target=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), target.c_str());
}

void ServerManager::onJoin(std::shared_ptr<Server> server, std::string origin, std::string channel)
{
	Logger::debug("server %s: onJoin: origin=%s, channel=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str());
}

void ServerManager::onKick(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string target, std::string reason)
{
	Logger::debug("server %s: onKick: origin=%s, channel=%s, target=%s, reason=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), target.c_str(), reason.c_str());
}

void ServerManager::onMessage(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string message)
{
	Logger::debug("server %s: onMessage: origin=%s, channel=%s, message=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), message.c_str());
}

void ServerManager::onMe(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message)
{
	Logger::debug("server %s: onMe: origin=%s, target=%s, message=%s",
		      server->info().name.c_str(), origin.c_str(), target.c_str(), message.c_str());
}

void ServerManager::onMode(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string mode, std::string arg)
{
	Logger::debug("server %s: onMode: origin=%s, channel=%s, mode=%s, argument=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), mode.c_str(), arg.c_str());
}

void ServerManager::onNick(std::shared_ptr<Server> server, std::string origin, std::string nickname)
{
	Logger::debug("server %s: onNick: origin=%s, nickname=%s",
		      server->info().name.c_str(), origin.c_str(), nickname.c_str());
}

void ServerManager::onNotice(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug("server %s: onNotice: origin=%s, message=%s",
		      server->info().name.c_str(), origin.c_str(), message.c_str());
}

void ServerManager::onPart(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string reason)
{
	Logger::debug("server %s: onPart: origin=%s, channel=%s, reason=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), reason.c_str());
}

void ServerManager::onQuery(std::shared_ptr<Server> server, std::string origin, std::string message)
{
	Logger::debug("server %s: onQuery: origin=%s, message=%s",
		      server->info().name.c_str(), origin.c_str(), message.c_str());
}

void ServerManager::onTopic(std::shared_ptr<Server> server, std::string origin, std::string channel, std::string topic)
{
	Logger::debug("server %s: onTopic: origin=%s, channel=%s, topic=%s",
		      server->info().name.c_str(), origin.c_str(), channel.c_str(), topic.c_str());
}

void ServerManager::onUserMode(std::shared_ptr<Server>, std::string, std::string)
{
}

ServerManager::ServerManager()
	: m_thread(std::bind(&ServerManager::run, this))
{
}

ServerManager::~ServerManager()
{
	m_running = false;

	try {
		m_thread.join();
	} catch (const std::exception &) {
		// TODO, show debug error
	}
}

} // !irccd

#if 0

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

#endif
