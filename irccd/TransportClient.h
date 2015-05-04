/*
 * TransportClient.h -- client connected to irccd
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

#ifndef _IRCCD_TRANSPORT_CLIENT_H_
#define _IRCCD_TRANSPORT_CLIENT_H_

/**
 * @file TransportClient.h
 * @brief Client connected to irccd
 */

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <Json.h>
#include <Signals.h>

#include "SocketTcp.h"

namespace irccd {

/**
 * @class TransportClient
 * @brief Client connected to irccd
 */
class TransportClientAbstract {
public:
	/*
	 * Signal: onChannelNotice
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
	Signal<std::string, std::string, std::string> onChannelNotice;
	Signal<> onConnect;
	Signal<std::string> onDisconnect;
	Signal<std::string, std::string, std::string> onInvite;
	Signal<std::string, std::string, std::string> onJoin;
	Signal<std::string, std::string, std::string, std::string> onKick;
	Signal<std::string> onLoad;
	Signal<std::string, std::string, std::string> onMe;
	Signal<std::string, std::string, std::string> onMessage;
	Signal<std::string, std::string, std::string> onMode;
	Signal<std::string, std::string> onNick;
	Signal<std::string, std::string, std::string> onNotice;
	Signal<std::string, std::string, std::string> onPart;
	Signal<std::string> onReconnect;
	Signal<std::string> onReload;
	Signal<std::string> onTopic;
	Signal<std::string> onUnload;
	Signal<std::string, std::string> onUserMode;
	Signal<> onDie;
	Signal<> onWrite;

private:
	std::string m_input;
	std::string m_output;
	mutable std::mutex m_mutex;

	/* JSON helpers */
	JsonValue want(const JsonObject &, const std::string &name) const;
	JsonValue optional(const JsonObject &, const std::string &name, const JsonValue &def) const;

	/* Parse JSON commands */
	void parseChannelNotice(const JsonObject &) const;
	void parseConnect(const JsonObject &) const;
	void parseDisconnect(const JsonObject &) const;
	void parseInvite(const JsonObject &) const;
	void parseJoin(const JsonObject &) const;
	void parseKick(const JsonObject &) const;
	void parseLoad(const JsonObject &) const;
	void parseMe(const JsonObject &) const;
	void parseMessage(const JsonObject &) const;
	void parseMode(const JsonObject &) const;
	void parseNick(const JsonObject &) const;
	void parseNotice(const JsonObject &) const;
	void parsePart(const JsonObject &) const;
	void parseReconnect(const JsonObject &) const;
	void parseReload(const JsonObject &) const;
	void parseTopic(const JsonObject &) const;
	void parseUnload(const JsonObject &) const;
	void parseUserMode(const JsonObject &) const;
	void parse(const std::string &) const;

	/* Do I/O */
	void receive();
	void send();

public:
	/**
	 * Virtual destructor defaulted.
	 */
	virtual ~TransportClientAbstract() = default;

	/**
	 * Flush pending data to send and try to receive if possible.
	 *
	 * @note This function is called from the TransportManager thread and should not be used somewhere else
	 */
	void process(int direction);

	/**
	 * Send an error message to the client.
	 *
	 * @param message the error message
	 * @param notify set to true to notify the TransportService
	 */
	void error(std::string message, bool notify = true);

	/**
	 * Send some data, it will be pushed to the outgoing buffer.
	 *
	 * This function appends "\r\n\r\n" after the message so you don't have
	 * to do it manually.
	 *
	 * @note Thread-safe
	 * @param message the message
	 * @param notify set to true to notify the TransportService
	 */
	void send(std::string message, bool notify = true);

	/**
	 * Tell if the client has data pending for output.
	 *
	 * @note Thread-safe
	 * @return true if has pending data to write
	 */
	bool hasOutput() const noexcept;

	/**
	 * Get the underlying socket as abstract.
	 *
	 * @return the abstract socket
	 */
	virtual SocketAbstractTcp &socket() noexcept = 0;
};

/**
 * @class TransportClient
 * @brief Template class for Tcp and Ssl sockets
 */
template <typename Sock>
class TransportClient : public TransportClientAbstract {
private:
	Sock m_socket;

public:
	/**
	 * Create a client.
	 *
	 * @param sock the socket
	 */
	inline TransportClient(Sock socket)
		: m_socket(std::move(socket))
	{
	}

	/**
	 * @copydoc TransportClientAbstract::socket
	 */
	SocketAbstractTcp &socket() noexcept override
	{
		return m_socket;
	}
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_CLIENT_H_
