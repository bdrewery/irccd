/*
 * ServerEvent.h -- base event class for server events
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

#ifndef _IRCCD_SERVER_EVENT_H_
#define _IRCCD_SERVER_EVENT_H_

/**
 * @file ServerEvent.h
 * @brief Base event class
 */

#include <utility>

namespace irccd {

class Plugin;
class Server;

/**
 * @enum MessageType
 * @brief Standard message or plugin command
 */
enum class MessageType {
	Command,			//!< Message is a command
	Message				//!< Message is a standard IRC message
};

/**
 * Pair of message and its category
 */
using MessagePack = std::pair<std::string, MessageType>;

/**
 * @class Event
 * @brief Base event class for plugins
 */
class ServerEvent {
protected:
	/**
	 * Parse IRC message depending on the command char and the plugin name.
	 *
	 * @param message the message content
	 * @param server which server
	 * @param plugin which plugin
	 * @return the parsed message as a command or a standard message
	 */
	MessagePack parseMessage(std::string message, Server &server, Plugin &plugin) const;

public:
	/**
	 * Construct an event.
	 *
	 * @param serverName the server name
	 * @param targetName the target name
	 */
	ServerEvent(const std::string &serverName = "", const std::string &targetName = "");

	/**
	 * Execute the plugin command.
	 *
	 * @param p the current plugin
	 */
	virtual void call(Plugin &p) = 0;

	/**
	 * Get the event name such as onMessage, onCommand.
	 *
	 * The plugin is passed as parameter in case of the event may differ
	 * from a plugin to other.
	 *
	 * Example:
	 *	A channel message "!history help" will trigger history's
	 *	onCcommand while it will call onMessage for other plugins.
	 *
	 * @param p the current plugin
	 * @return the event name
	 */
	virtual const char *name(Plugin &p) const = 0;
};

} // !irccd

#endif // !_IRCCD_SERVER_EVENT_H_
