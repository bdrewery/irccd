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
#include <SocketUdp.h>

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
	using CommandHandler = void (TransportService::*)(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);
	using CommandMap = std::unordered_map<std::string, CommandHandler>;

private:
	CommandMap m_commandMap;
	std::function<void (TransportCommand)> m_onEvent;
	std::map<Socket, std::unique_ptr<TransportAbstract>> m_transports;
	std::map<Socket, std::shared_ptr<TransportClientAbstract>> m_clients;

	// TODO: rename all commands to handle<Name>().

	/*
	 * Send a channel notice
	 * --------------------------------------------------------
	 *
	 * Send a channel notice to the specified channel.
	 *
	 * {
	 *   "command": "cnotice",
	 *   "server: "the server name",
	 *   "channel": "name",
	 *   "message": "the message"
	 * }
	 */
	void handleChannelNotice(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Connect to a server
	 * --------------------------------------------------------
	 *
	 * Connect to a server.
	 *
	 * There are no default argument, everything must be set.
	 *
	 * {
	 *   "command": "connect",
	 *   "name": "server ident",
	 *   "host": "server host",
	 *   "port": 6667,
	 *   "ssl": true,
	 *   "ssl-verify": true
	 * }
	 *
	 * Responses:
	 *   - Error if a server with that name already exists
	 */
	void handleConnect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Disconnect a server
	 * --------------------------------------------------------
	 *
	 * Disconnect from a server.
	 *
	 * {
	 *   "command": "disconnect",
	 *   "server: "the server name"
	 * }
	 *
	 * Responses:
	 *   - Error if the server does not exist
	 */
	void handleDisconnect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Invite someone
	 * --------------------------------------------------------
	 *
	 * Invite someone to the specified channel.
	 *
	 * {
	 *   "command": "invite",
	 *   "server: "the server name",
	 *   "target": "the nickname",
	 *   "channel": "the channel"
	 * }
	 */
	void handleInvite(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Join a channel
	 * --------------------------------------------------------
	 *
	 * Join a channel, you may add an optional password.
	 *
	 * {
	 *   "command": "join",
	 *   "server: "the server name",
	 *   "channel": "channel name",
	 *   "password": "the password"		(Optional)
	 * }
	 */
	void handleJoin(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Kick someone from a channel
	 * --------------------------------------------------------
	 *
	 * Kick a target from a channel.
	 *
	 * {
	 *   "command": "kick",
	 *   "server: "the server name",
	 *   "target": "the nickname",
	 *   "channel": "the channel",
	 *   "reason": "the optional reason"	(Optional)
	 * }
	 */
	void handleKick(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Load a plugin.
	 * --------------------------------------------------------
	 *
	 * Load a plugin not already loaded by Irccd.
	 *
	 * 1. By relative path (searched through all Irccd plugin directories)
	 *
	 * {
	 *   "command": "load",
	 *   "name": "plugin"
	 * }
	 *
	 * 2. By path, if not absolute, it is relative to the irccd current working directory.
	 *
	 * {
	 *   "command": "load",
	 *   "path": "/opt/irccd/plugins/crazygame.js"
	 * }
	 *
	 * Responses:
	 *   - Error if the plugin failed to load or was not found
	 */
	void handleLoad(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Send a CTCP Action
	 * --------------------------------------------------------
	 *
	 * Send a CTCP action knows as /me.
	 *
	 * {
	 *   "command": "me",
	 *   "server: "the server name",
	 *   "channel": "the channel",
	 *   "message": "the message"		(Optional)
	 * }
	 */
	void handleMe(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Say something to a target
	 * --------------------------------------------------------
	 *
	 * Send a message to a target which can be a nickname or a channel.
	 *
	 * {
	 *   "command": "say",
	 *   "server: "the server name",
	 *   "target": "channel or nickname",
	 *   "message": "The message"
	 * }
	 */
	void handleMessage(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Change the channel mode
	 * --------------------------------------------------------
	 *
	 * Change the channel mode.
	 *
	 * {
	 *   "command": "mode",
	 *   "server: "the server name",
	 *   "channel": "channel",
	 *   "mode": "mode and its arguments"
	 * }
	 */
	void handleMode(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Change the bot nickname
	 * --------------------------------------------------------
	 *
	 * Change the daemon nickname.
	 *
	 * {
	 *   "command": "nick",
	 *   "server: "the server name",
	 *   "nickname": "the new nickname"
	 * }
	 */
	void handleNick(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Send a notice
	 * --------------------------------------------------------
	 *
	 * Send a notice to a target.
	 *
	 * {
	 *   "command": "notice",
	 *   "server: "the server name",
	 *   "target": "the nickname",
	 *   "message": "the message"
	 * }
	 */
	void handleNotice(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Part from a channel
	 * --------------------------------------------------------
	 *
	 * Leaves a channel. May add an optional reason but it does not work
	 * for every servers.
	 *
	 * {
	 *   "command": "part",
	 *   "server: "the server name",
	 *   "channel": "the channel name",
	 *   "reason": "the reason"		(Optional)
	 * }
	 */
	void handlePart(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Force reconnection of a server
	 * --------------------------------------------------------
	 *
	 * May be used to force the reconnexion of a server if Irccd didn't catch
	 * the disconnection.
	 *
	 * If no server has been specified, all servers are marked for reconnection.
	 *
	 * {
	 *   "command": "reconnect",
	 *   "server": "the server name",	(Optional)
	 * }
	 *
	 * Responses:
	 *   - Error if the server does not exist
	 */
	void handleReconnect(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Reload a plugin
	 * --------------------------------------------------------
	 *
	 * Reload the plugin by name, invokes the onReload function, does not unload and
	 * load the plugin.
	 *
	 * {
	 *   "command": "reload",
	 *   "plugin": "crazygame"
	 * }
	 *
	 * Responses:
	 *   - Error if the plugin does not exists
	 */
	void handleReload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Change a channel topic
	 * --------------------------------------------------------
	 *
	 * Change the topic on the specified channel.
	 *
	 * {
	 *   "command": "topic",
	 *   "server: "the server name",
	 *   "channel": "the channel name",
	 *   "topic": "the new topic"
	 * }
	 */
	void handleTopic(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Set the irccd user mode
	 * --------------------------------------------------------
	 *
	 * Change your irccd user mode for the specified server.
	 *
	 * {
	 *   "command": "umode",
	 *   "server: "the server name",
	 *   "mode": "the mode"
	 * }
	 */
	void handleUserMode(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/*
	 * Unload a plugin
	 * --------------------------------------------------------
	 *
	 * Unload a plugin by its name. Also invokes the onUnload function
	 * before removing it.
	 *
	 * {
	 *   "command": "unload",
	 *   "plugin": "crazygame"
	 * }
	 *
	 * Responses:
	 *   - Error if the plugin does not exists
	 */
	void handleUnload(const std::shared_ptr<TransportClientAbstract> &, const JsonObject &);

	/* transport slots */
	void onMessage(const std::shared_ptr<TransportClientAbstract> &, const std::string &);
	void onWrite();
	void onDie(const std::shared_ptr<TransportClientAbstract> &);

	/* private Json helpers */
	JsonValue want(const JsonObject &, const std::string &name) const;
	JsonValue optional(const JsonObject &, const std::string &name, const JsonValue &def) const;

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
	 * Set the event handler.
	 *
	 * @pre isRunning() must return false
	 * @param func the handler function
	 */
	inline void setOnEvent(std::function<void (TransportCommand)> func) noexcept
	{
		assert(!isRunning());

		m_onEvent = std::move(func);
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
