/*
 * UserMode.h -- on user mode change
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

#ifndef _USER_MODE_H_
#define _USER_MODE_H_

/**
 * @file event/UserMode.h
 * @brief On userm ode change
 */

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

namespace event {

/**
 * @class UserMode
 * @brief On user mode change
 */
class UserMode final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_nickname;
	std::string		m_mode;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param nickname the one who changed your mode
	 * @param mode the mode
	 */
	UserMode(std::shared_ptr<Server> server, std::string nickname, std::string mode);

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

#endif // !_USER_MODE_H_
