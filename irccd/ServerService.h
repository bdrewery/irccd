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

#include <Signals.h>

#include "Server.h"
#include "Service.h"

namespace irccd {

class ServerEvent;

class ServerService : public Service {
public:
	/*
	 * Signal: event has been generated
	 * ------------------------------------------------
	 *
	 * This signal is emitted when an event has been received.
	 *
	 * <strong>Do not add a function while the thread is running</strong>
	 *
	 * Arguments:
	 * - the generic event
	 */
	Signal<ServerEvent> onEvent;

private:
	std::unordered_map<std::string, std::shared_ptr<Server>> m_servers;

protected:
	void run() override;

private:
	/* ------------------------------------------------
	 * Slots (convert Server's signals to ServerEvent)
	 * ------------------------------------------------ */

	void handleOnChannelNotice(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnConnect(std::shared_ptr<Server>);
	void handleOnInvite(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnJoin(std::shared_ptr<Server>, std::string, std::string);
	void handleOnKick(std::shared_ptr<Server>, std::string, std::string, std::string, std::string);
	void handleOnMessage(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnMe(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnMode(std::shared_ptr<Server>, std::string, std::string, std::string, std::string);
	void handleOnNick(std::shared_ptr<Server>, std::string, std::string);
	void handleOnNotice(std::shared_ptr<Server>, std::string, std::string);
	void handleOnPart(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnQuery(std::shared_ptr<Server>, std::string, std::string);
	void handleOnTopic(std::shared_ptr<Server>, std::string, std::string, std::string);
	void handleOnUserMode(std::shared_ptr<Server>, std::string, std::string);

public:
	/**
	 * Default constructor, does nothing.
	 */
	ServerService();

	/**
	 * Add a server. Pass exactly the same arguments as you would pass
	 * to the Server constructor.
	 *
	 * @pre the server must not exists
	 * @param args the arguments to pass to Server constructor
	 * @throw std::exception on failures
	 */
	template <typename... Args>
	void add(Args&&... args)
	{
		using namespace std;
		using namespace std::placeholders;

		shared_ptr<Server> server = make_shared<Server>(forward<Args>(args)...);

		assert(!has(server->info().name));

		server->onChannelNotice.connect(bind(&ServerService::handleOnChannelNotice, this, server, _1, _2, _3));
		server->onConnect.connect(bind(&ServerService::handleOnConnect, this, server));
		server->onInvite.connect(bind(&ServerService::handleOnInvite, this, server, _1, _2, _3));
		server->onJoin.connect(bind(&ServerService::handleOnJoin, this, server, _1, _2));
		server->onKick.connect(bind(&ServerService::handleOnKick, this, server, _1, _2, _3, _4));
		server->onMessage.connect(bind(&ServerService::handleOnMessage, this, server, _1, _2, _3));
		server->onMe.connect(bind(&ServerService::handleOnMe, this, server, _1, _2, _3));
		server->onMode.connect(bind(&ServerService::handleOnMode, this, server, _1, _2, _3, _4));
		server->onNick.connect(bind(&ServerService::handleOnNick, this, server, _1, _2));
		server->onNotice.connect(bind(&ServerService::handleOnNotice, this, server, _1, _2));
		server->onPart.connect(bind(&ServerService::handleOnPart, this, server, _1, _2, _3));
		server->onQuery.connect(bind(&ServerService::handleOnQuery, this, server, _1, _2));
		server->onTopic.connect(bind(&ServerService::handleOnTopic, this, server, _1, _2, _3));
		server->onUserMode.connect(bind(&ServerService::handleOnUserMode, this, server, _1, _2));

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

	/**
	 * Check if a server is already enabled.
	 *
	 * @param name the server name
	 * @return true if the server exists
	 * @note Thread-safe
	 */
	inline bool has(const std::string &name) const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		return m_servers.count(name) > 0;
	}
};

} // !irccd

#endif // _IRCCD_SERVER_SERVICE_H_
