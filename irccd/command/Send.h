/*
 * Send.h -- send a raw message
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

#ifndef _IRCCD_COMMAND_SEND_H_
#define _IRCCD_COMMAND_SEND_H_

/**
 * @file command/Send.h
 * @brief Send a raw message
 */

#include "ServerCommand.h"

namespace irccd {

namespace command {

/**
 * @class Send
 * @brief Send a raw message
 */
class Send final : public ServerCommand {
private:
	Server &m_server;
	std::string m_message;

public:
	/**
	 * Send constructor.
	 *
	 * @param server the server
	 * @param message the message
	 */
	Send(Server &server, std::string message);

	/**
	 * @copydoc Command::call
	 */
	bool call() override;
};

} // !command

} // !irccd

#endif // !_IRCCD_COMMAND_SEND_H_
