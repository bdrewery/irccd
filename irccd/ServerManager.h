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

#endif // _IRCCD_SERVER_MANAGER_H_
