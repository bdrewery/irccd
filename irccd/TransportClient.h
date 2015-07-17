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
#include <string>

#include <Json.h>
#include <Signals.h>

#include "Server.h"

namespace irccd {

/**
 * @class TransportClient
 * @brief Client connected to irccd
 */
class TransportClientAbstract {
public:
	/**
	 * Signal: onChannelNotice
	 * --------------------------------------------------------
	 *
	 * Send a channel notice to the specified channel.
	 *
	 * Arguments:
	 * - the server name
	 * - the channel
	 * - the notice message
	 */
	Signal<std::string, std::string, std::string> onChannelNotice;

	/**
	 * Signal: onConnect
	 * ------------------------------------------------
	 *
	 * Request to connect to a server.
	 *
	 * Arguments:
	 * - the server information
	 * - the server identity
	 * - the server settings
	 */
	Signal<ServerInfo, ServerIdentity, ServerSettings> onConnect;

	/**
	 * TODO
	 */
	Signal<std::string> onDisconnect;

	/**
	 * Signal: onInvite
	 * ------------------------------------------------
	 *
	 * Invite someone to a channel.
	 *
	 * Arguments:
	 * - the server name
	 * - the target name
	 * - the channel
	 */
	Signal<std::string, std::string, std::string> onInvite;

	/**
	 * Signal: onJoin
	 * ------------------------------------------------
	 *
	 * Join a channel.
	 *
	 * Arguments:
	 * - the server name
	 * - the channel
	 * - the password (optional)
	 */
	Signal<std::string, std::string, std::string> onJoin;

	/**
	 * Signal: onKick
	 * ------------------------------------------------
	 *
	 * Kick someone from a channel.
	 *
	 * Arguments:
	 * - the server name
	 * - the target name
	 * - the channel
	 * - the reason (optional)
	 */
	Signal<std::string, std::string, std::string, std::string> onKick;

	/**
	 * Signal: onLoad
	 * ------------------------------------------------
	 *
	 * Request to load a plugin. Always relative.
	 *
	 * Arguments:
	 * - the plugin name
	 */
	Signal<std::string> onLoad;

	/**
	 * Signal: onMe
	 * ------------------------------------------------
	 *
	 * Send a CTCP Action.
	 *
	 * Arguments:
	 * - the server name
	 * - the target name
	 * - the message
	 */
	Signal<std::string, std::string, std::string> onMe;

	/**
	 * Signal: onMessage
	 * ------------------------------------------------
	 *
	 * Send a standard message.
	 *
	 * Arguments:
	 * - the server name
	 * - the target name
	 * - the message
	 */
	Signal<std::string, std::string, std::string> onMessage;

	/**
	 * Signal: onMode
	 * ------------------------------------------------
	 *
	 * Change the channel mode.
	 *
	 * Arguments:
	 * - the server name
	 * - the channel name
	 * - the mode argument
	 */
	Signal<std::string, std::string, std::string> onMode;

	/**
	 * Signal: onNick
	 * ------------------------------------------------
	 *
	 * Change the nickname.
	 *
	 * Arguments:
	 * - the server name
	 * - the new nickname
	 */
	Signal<std::string, std::string> onNick;

	/**
	 * Signal: onNotice
	 * ------------------------------------------------
	 *
	 * Send a notice.
	 *
	 * Arguments:
	 * - the server name
	 * - the target name
	 * - the message
	 */
	Signal<std::string, std::string, std::string> onNotice;

	/**
	 * Signal: onPart
	 * ------------------------------------------------
	 *
	 * Leave a channel.
	 *
	 * Arguments:
	 * - the server name
	 * - the channel name
	 * - the reason (optional)
	 */
	Signal<std::string, std::string, std::string> onPart;

	/**
	 * Signal: onReconnect
	 * ------------------------------------------------
	 *
	 * Reconnect one or all servers.
	 *
	 * Arguments:
	 * - the server name (optional)
	 */
	Signal<std::string> onReconnect;

	/**
	 * Signal: onReload
	 * ------------------------------------------------
	 *
	 * Reload a plugin.
	 *
	 * Arguments:
	 * - the plugin name
	 */
	Signal<std::string> onReload;

	/**
	 * Signal: onTopic
	 * ------------------------------------------------
	 *
	 * Change a channel topic.
	 *
	 * Arguments:
	 * - the server name
	 * - the channel name
	 * - the new topic (optional)
	 */
	Signal<std::string, std::string, std::string> onTopic;

	/**
	 * Signal: onUnload
	 * ------------------------------------------------
	 *
	 * Unload a plugin.
	 *
	 * Arguments:
	 * - the plugin name
	 */
	Signal<std::string> onUnload;

	/**
	 * Signal: onUserMode
	 * ------------------------------------------------
	 *
	 * Change a user mode.
	 *
	 * Arguments:
	 * - the server name
	 * - the new mode
	 */
	Signal<std::string, std::string> onUserMode;

	/**
	 * Signal: onDie
	 * ------------------------------------------------
	 *
	 * The client has disconnected.
	 */
	Signal<> onDie;

protected:
	std::string m_input;
	std::string m_output;

	/* JSON helpers */
	JsonValue value(const JsonObject &, const std::string &name) const;
	JsonValue valueOr(const JsonObject &, const std::string &name, const JsonValue &def) const;

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
	virtual void receive() = 0;
	virtual void send() = 0;

public:
	/**
	 * Virtual destructor defaulted.
	 */
	virtual ~TransportClientAbstract() = default;

	/**
	 * Send or receive data, called after a select.
	 *
	 * @param setinput the input fd_set
	 * @param setoutput the output fd_set
	 */
	void sync(fd_set &setinput, fd_set &setoutput);

	/**
	 * Send an error message to the client.
	 *
	 * @param message the error message
	 */
	void error(std::string message);

	/**
	 * Send some data, it will be pushed to the outgoing buffer.
	 *
	 * This function appends "\r\n\r\n" after the message so you don't have
	 * to do it manually.
	 *
	 * @param message the message
	 */
	void send(std::string message);

	/**
	 * Tell if the client has data pending for output.
	 *
	 * @return true if has pending data to write
	 */
	inline bool hasOutput() const noexcept
	{
		return !m_output.empty();
	}

	/**
	 * Get the underlying socket.
	 */
	virtual SocketAbstract &socket() noexcept = 0;
};

/**
 * @class TransportClient
 * @brief Template class for Tcp and Ssl sockets
 */
template <typename Address>
class TransportClient : public TransportClientAbstract {
private:
	SocketTcp<Address> m_socket;

protected:
	void send() override;
	void receive() override;

public:
	/**
	 * Create a client.
	 *
	 * @param sock the socket
	 */
	inline TransportClient(SocketTcp<Address> socket)
		: m_socket{std::move(socket)}
	{
	}

	SocketAbstract &socket() noexcept override
	{
		return m_socket;
	}
};

template <typename Address>
void TransportClient<Address>::receive()
{
	try {
		auto message = m_socket.recv(512);

		if (message.empty()) {
			throw std::runtime_error("client disconnected");
		}

		m_input += message;
	} catch (const std::exception &ex) {
		onDie();
	}

	std::string::size_type pos;
	while ((pos = m_input.find("\r\n\r\n")) != std::string::npos) {
		/*
		 * Make a copy and erase it in case that onComplete function
		 * throws.
		 */
		auto message = m_input.substr(0, pos);

		m_input.erase(m_input.begin(), m_input.begin() + pos + 4);

		try {
			parse(message);
		} catch (const std::exception &ex) {
			// TODO: report error to client
			Logger::warning() << "transport: " << ex.what() << std::endl;
		}
	}
}

template <typename Address>
void TransportClient<Address>::send()
{
	m_output.erase(0, m_socket.send(m_output));
}

} // !irccd

#endif // !_IRCCD_TRANSPORT_CLIENT_H_
