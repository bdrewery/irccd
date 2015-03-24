/*
 * Command.h -- on channel command
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

#ifndef _COMMAND_H_
#define _COMMAND_H_

/**
 * @file event/Command.h
 * @brief On message
 */

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

namespace event {

/**
 * @class Command
 * @brief On command
 *
 * This event is generated from a message but when it starts with the command
 * character (e.g "!history help")
 */
class Command final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_nickname;
	std::string		m_message;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param nickname the user who sent the message
	 * @param message the message
	 */
	Command(std::shared_ptr<Server> server, std::string channel, std::string nickname, std::string message);

	/**
	 * @copydoc Event::call
	 */
	void call(Plugin &p) override;

	/**
	 * @copydoc Event::name
	 */
	const char *name(Plugin &p) const override;
};

} // !event

} // !irccd

#endif // !_MESSAGE_H_


