/*
 * Names.h -- on channel name listing
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

#ifndef _NAMES_H_
#define _NAMES_H_

/**
 * @file event/Names.h
 * @brief On name listing
 */

#include <memory>
#include <vector>

#include "Event.h"

namespace irccd {

class Server;

namespace event {

/**
 * @class Names
 * @brief On name listing
 */
class Names final : public Event {
private:
	std::shared_ptr<Server>		m_server;
	std::string			m_channel;
	std::vector<std::string>	m_names;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param names the list
	 */
	Names(std::shared_ptr<Server> server, std::string channel, std::vector<std::string> names);

	/**
	 * @copydoc Event::call
	 */
	void call(Plugin &p) override;

	/**
	 * @copydoc Event::name
	 */
	const char *name() const override;
};

} // !event

} // !irccd

#endif // !_NAMES_H_
