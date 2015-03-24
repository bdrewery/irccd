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

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "Server.h"

namespace irccd {

class ServerManager {
private:
	std::unordered_map<std::string, std::shared_ptr<Server>> m_servers;
	std::atomic<bool> m_running{true};
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
	ServerManager();

	~ServerManager();

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

#if 0

/**
 * @file ServerManager.h
 * @brief Manage all servers
 */

#include <thread>

#include <Singleton.h>

#include "Server.h"

namespace irccd {

/**
 * @class ServerManager
 * @brief Manages the connected servers
 */
class ServerManager final : public Singleton<ServerManager> {
private:
	SINGLETON(ServerManager);

	using Thread	= std::thread;
	using Mutex	= std::recursive_mutex;
	using Map	= std::unordered_map<std::string, std::shared_ptr<Server>>;
	using Lock	= std::lock_guard<Mutex>;

	mutable Mutex	m_lock;
	Map		m_servers;
	Thread		m_thread;

	void cleaner();

	ServerManager();

public:
	/**
	 * Default destructor.
	 */
	~ServerManager() override;

	/**
	 * Add a new server to the registry. It also start the server
	 * immediately.
	 *
	 * @param server the server to add
	 */
	void add(std::shared_ptr<Server> &&server);

	/**
	 * Remove the server from the registry.
	 *
	 * @param server
	 */
	void remove(const std::shared_ptr<Server> &server);

	/**
	 * Get an existing server.
	 *
	 * @param name the server name
	 * @return the server
	 * @throw std::out_of_range if not found
	 */
	std::shared_ptr<Server> get(const std::string &name) const;

	/**
	 * Check if a server exists
	 *
	 * @param name the server name
	 * @return true if the server by this name is loaded
	 */
	bool has(const std::string &name) const;

	/**
	 * Call a function for all servers.
	 *
	 * @param func the function
	 */
	template <typename Func>
	void forAll(Func func)
	{
		Lock lk(m_lock);

		for (const auto &s : m_servers)
			func(s.second);
	}
};

} // !irccd

#endif

#endif // _IRCCD_SERVER_MANAGER_H_
