/*
 * ServerManager.h -- manage servers
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

#ifndef _IRCCD_SERVER_MANAGER_H_
#define _IRCCD_SERVER_MANAGER_H_

#include <cassert>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "Server.h"

namespace irccd {

class ServerEvent;

class ServerManager {
private:
	std::function<void (std::unique_ptr<ServerEvent>)> m_onEvent;
	std::unordered_map<std::string, std::shared_ptr<Server>> m_servers;
	std::atomic<bool> m_running{false};
	std::thread m_thread;
	mutable std::mutex m_mutex;

	void run() noexcept;

	// Converters from Server callbacks to our event loop
	void onChannelNotice(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onConnect(std::shared_ptr<Server>);
	void onInvite(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onJoin(std::shared_ptr<Server>, std::string, std::string);
	void onKick(std::shared_ptr<Server>, std::string, std::string, std::string, std::string);
	void onMessage(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onMe(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onMode(std::shared_ptr<Server>, std::string, std::string, std::string, std::string);
	void onNick(std::shared_ptr<Server>, std::string, std::string);
	void onNotice(std::shared_ptr<Server>, std::string, std::string);
	void onPart(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onQuery(std::shared_ptr<Server>, std::string, std::string);
	void onTopic(std::shared_ptr<Server>, std::string, std::string, std::string);
	void onUserMode(std::shared_ptr<Server>, std::string, std::string);

public:
	/**
	 * Default constructor, does nothing.
	 */
	ServerManager() = default;

	/**
	 * Destructor, close the thread.
	 */
	~ServerManager();

	/**
	 * Start the thread.
	 */
	void start();

	/**
	 * Set the event handler.
	 *
	 * @param func the function
	 * @pre the thread must not be started
	 */
	inline void setOnEvent(std::function<void (std::unique_ptr<ServerEvent>)> func) noexcept
	{
		assert(!m_running);

		m_onEvent = std::move(func);
	}

	/**
	 * Add a server. Pass exactly the same arguments as you would pass
	 * to the Server constructor.
	 *
	 * @param args the arguments to pass to Server constructor
	 * @throw std::exception on failures
	 */
	template <typename... Args>
	void add(Args&&... args)
	{
		using namespace std;
		using namespace std::placeholders;

		lock_guard<mutex> lock(m_mutex);
		shared_ptr<Server> server = make_shared<Server>(forward<Args>(args)...);

		server->onChannelNotice(bind(&ServerManager::onChannelNotice, this, server, _1, _2, _3));
		server->onConnect(bind(&ServerManager::onConnect, this, server));
		server->onInvite(bind(&ServerManager::onInvite, this, server, _1, _2, _3));
		server->onJoin(bind(&ServerManager::onJoin, this, server, _1, _2));
		server->onKick(bind(&ServerManager::onKick, this, server, _1, _2, _3, _4));
		server->onMessage(bind(&ServerManager::onMessage, this, server, _1, _2, _3));
		server->onMe(bind(&ServerManager::onMe, this, server, _1, _2, _3));
		server->onMode(bind(&ServerManager::onMode, this, server, _1, _2, _3, _4));
		server->onNick(bind(&ServerManager::onNick, this, server, _1, _2));
		server->onNotice(bind(&ServerManager::onNotice, this, server, _1, _2));
		server->onPart(bind(&ServerManager::onPart, this, server, _1, _2, _3));
		server->onQuery(bind(&ServerManager::onQuery, this, server, _1, _2));
		server->onTopic(bind(&ServerManager::onTopic, this, server, _1, _2, _3));
		server->onUserMode(bind(&ServerManager::onUserMode, this, server, _1, _2));

		m_servers.emplace(server->info().name, std::move(server));
	}
};

} // !irccd

#endif // _IRCCD_SERVER_MANAGER_H_
