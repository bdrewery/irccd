/*
 * Kick.h -- on channel kick
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

#ifndef _KICK_H_
#define _KICK_H_

/**
 * @file event/Kick.h
 * @brief On kick
 */

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

namespace event {

/**
 * @class Kick
 * @brief On kick
 */
class Kick final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_who;
	std::string		m_kicked;
	std::string		m_reason;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param channel from which channel
	 * @param who the user who kicked
	 * @param kicked the kicked person
	 * @param reason the reason
	 */
	Kick(std::shared_ptr<Server> server, std::string channel, std::string who, std::string kicked, std::string reason);

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

#endif // !_KICK_H_
