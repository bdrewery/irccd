/*
 * ServerService.h -- processes servers automatically
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

#ifndef _IRCCD_SERVER_SERVICE_H_
#define _IRCCD_SERVER_SERVICE_H_

#include <cassert>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include "Server.h"
#include "Service.h"

namespace irccd {

class ServerEvent;

class ServerService : public Service {
private:
	std::function<void (std::unique_ptr<ServerEvent>)> m_onEvent;
	std::unordered_map<std::string, std::shared_ptr<Server>> m_servers;

protected:
	void run() override;

private:
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
	ServerService();

	/**
	 * Set the event handler.
	 *
	 * @param func the function
	 * @pre the thread must not be started
	 */
	inline void setOnEvent(std::function<void (std::unique_ptr<ServerEvent>)> func) noexcept
	{
		assert(!isRunning());

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

		shared_ptr<Server> server = make_shared<Server>(forward<Args>(args)...);

		server->setOnChannelNotice(bind(&ServerService::onChannelNotice, this, server, _1, _2, _3));
		server->setOnConnect(bind(&ServerService::onConnect, this, server));
		server->setOnInvite(bind(&ServerService::onInvite, this, server, _1, _2, _3));
		server->setOnJoin(bind(&ServerService::onJoin, this, server, _1, _2));
		server->setOnKick(bind(&ServerService::onKick, this, server, _1, _2, _3, _4));
		server->setOnMessage(bind(&ServerService::onMessage, this, server, _1, _2, _3));
		server->setOnMe(bind(&ServerService::onMe, this, server, _1, _2, _3));
		server->setOnMode(bind(&ServerService::onMode, this, server, _1, _2, _3, _4));
		server->setOnNick(bind(&ServerService::onNick, this, server, _1, _2));
		server->setOnNotice(bind(&ServerService::onNotice, this, server, _1, _2));
		server->setOnPart(bind(&ServerService::onPart, this, server, _1, _2, _3));
		server->setOnQuery(bind(&ServerService::onQuery, this, server, _1, _2));
		server->setOnTopic(bind(&ServerService::onTopic, this, server, _1, _2, _3));
		server->setOnUserMode(bind(&ServerService::onUserMode, this, server, _1, _2));

		{
			lock_guard<mutex> lock(m_mutex);

			m_servers.emplace(server->info().name, std::move(server));
		}

		// This function can be called even if the thread is not currently started
		if (isRunning()) {
			reload();
		}
	}

	/**
	 * Get a server by name.
	 *
	 * @param name the server name
	 * @return the server
	 * @throw std::out_of_range if the server is not found
	 * @note Thread-safe
	 */
	std::shared_ptr<Server> find(const std::string &name) const;
};

} // !irccd

#endif // _IRCCD_SERVER_SERVICE_H_
