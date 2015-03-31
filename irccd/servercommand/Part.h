/*
 * Part.h -- leave a channel
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

#ifndef _IRCCD_COMMAND_PART_H_
#define _IRCCD_COMMAND_PART_H_

/**
 * @file command/Part.h
 * @brief Leave a channel
 */

#include "ServerCommand.h"

namespace irccd {

namespace command {

/**
 * @class Part
 * @brief Leave a channel
 */
class Part final : public ServerCommand {
private:
	Server &m_server;
	std::string m_channel;
	std::string m_reason;

public:
	/**
	 * Leave a channel.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param reason the reason
	 */
	Part(Server &server, std::string channel, std::string reason);

	/**
	 * @copydoc Command::call
	 */
	bool call() override;
};

} // !command

} // !irccd

#endif // !_IRCCD_COMMAND_PART_H_
