/*
 * ChannelNotice.h -- channel notice
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

#ifndef _CHANNEL_NOTICE_H_
#define _CHANNEL_NOTICE_H_

/**
 * @file command/ChannelNotice.h
 * @brief Channel notice
 */

#include <memory>

#include "Command.h"

namespace irccd {

class Server;

namespace command {

/**
 * @class ChannelNotice
 * @brief Channel notice
 */
class ChannelNotice final : public Command {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_notice;

public:
	/**
	 * Channel notice constructor.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param notice the notice
	 */
	ChannelNotice(std::shared_ptr<Server> server, std::string channel, std::string notice);

	/**
	 * @copydoc Command::call
	 */
	bool call() override;
};

} // !command

} // !irccd

#endif // !_CHANNEL_NOTICE_H_
