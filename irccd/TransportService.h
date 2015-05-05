/*
 * TransportService.h -- maintain transport I/O
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

#ifndef _IRCCD_TRANSPORT_SERVICE_H_
#define _IRCCD_TRANSPORT_SERVICE_H_

/**
 * @class TransportService
 * @brief Manage all transports and the clients
 */

#include <atomic>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <unordered_map>

#include <Logger.h>

#include "Service.h"
#include "Transport.h"
#include "TransportClient.h"

class JsonObject;
class JsonValue;

namespace irccd {

class TransportCommand;

/**
 * @class TransportService
 * @brief Manage transports and clients
 *
 * This class contains a transport for each one defined in the user
 * configuration, a thread waits for clients and receive their
 * messages for further usage.
 *
 * This class has also a socket for very basic IPC between Irccd and this
 * manager. This allows large timeout but quick reload of the listener
 * set in case of changes.
 */
class TransportService : public Service {
public:
	Signal<TransportCommand> onCommand;

private:
	std::map<Socket, std::unique_ptr<TransportAbstract>> m_transports;
	std::map<Socket, std::shared_ptr<TransportClientAbstract>> m_clients;

	/* ------------------------------------------------
	 * Slots (convert TransportClient's signals to TransportCommand)
	 * ------------------------------------------------ */

	void handleChannelNotice(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleConnect(std::shared_ptr<TransportClientAbstract>, ServerInfo, ServerIdentity, ServerSettings) const;
	void handleDisconnect(std::shared_ptr<TransportClientAbstract>, std::string) const;
	void handleInvite(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleJoin(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleKick(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string, std::string) const;
	void handleLoad(std::shared_ptr<TransportClientAbstract>, std::string) const;
	void handleMe(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleMessage(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleMode(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleNick(std::shared_ptr<TransportClientAbstract>, std::string, std::string) const;
	void handleNotice(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handlePart(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleReconnect(std::shared_ptr<TransportClientAbstract>, std::string) const;
	void handleReload(std::shared_ptr<TransportClientAbstract>, std::string) const;
	void handleTopic(std::shared_ptr<TransportClientAbstract>, std::string, std::string, std::string) const;
	void handleUnload(std::shared_ptr<TransportClientAbstract>, std::string) const;
	void handleUserMode(std::shared_ptr<TransportClientAbstract>, std::string, std::string) const;
	void handleOnWrite();
	void handleOnDie(const std::shared_ptr<TransportClientAbstract> &);

	/* private service helpers */
	void accept(const Socket &s);
	void process(const Socket &s, int direction);
	bool isTransport(const Socket &s) const noexcept;

protected:
	void run() override;

public:
	/**
	 * Create the transport manager, this create the Udp IPC socket.
	 *
	 * @throw SocketError on errors
	 */
	TransportService();

	/**
	 * Create a new transport in-place.
	 *
	 * @pre isRunning() must return false
	 * @param args the arguments to the transport constructor
	 * @throw std::exception on failures
	 */
	template <typename T, typename... Args>
	void add(Args&&... args)
	{
		assert(!isRunning());

		std::unique_ptr<TransportAbstract> ptr = std::make_unique<T>(std::forward<Args>(args)...);

		ptr->bind();
		Logger::info() << "transport: listening on " << ptr->info() << std::endl;

		m_transports.emplace(ptr->socket(), std::move(ptr));
	}

	/**
	 * Stop the thread and clean everything.
	 *
	 * @pre isRunning() must return true
	 * @note Thread-safe
	 */
	void stop();

	/**
	 * Send a message to all connected clients. Do not append \r\n\r\n,
	 * the function does it automatically.
	 *
	 * @note Thread-safe
	 * @pre isRunning() must return true
	 * @param msg the message (without \r\n\r\n)
	 */
	void broadcast(const std::string &msg);
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_SERVICE_H_
