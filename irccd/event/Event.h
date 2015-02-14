/*
 * Event.h -- base event class for plugins
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#ifndef _EVENT_H_
#define _EVENT_H_

/**
 * @file Event.h
 * @brief Base event class
 */

#include <utility>

#include <irccd/IO.h>

namespace irccd {

class Plugin;
class Server;

enum class MessageType {
	Command,
	Message
};

using MessagePack = std::pair<std::string, MessageType>;

/**
 * @class Event
 * @brief Base event class for plugins
 */
class Event : public IO {
protected:
	MessagePack parseMessage(std::string message, Server &server, Plugin &plugin) const;

public:
	/**
	 * Construct an event.
	 *
	 * @param serverName the server name
	 * @param targetName the target name
	 */
	Event(const std::string &serverName = "", const std::string &targetName = "");

	/**
	 * Try to encode the plugin to UTF-8 from the server encoding.
	 *
	 * @param input the input
	 * @return the converted string or input on failures
	 */
	std::string tryEncode(const std::string &input);

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

#endif // !_EVENT_H_
