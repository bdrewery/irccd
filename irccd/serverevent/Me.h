/*
 * Me.h -- on CTCP Action
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

#ifndef _IRCCD_ME_H_
#define _IRCCD_ME_H_

/**
 * @file event/Me.h
 * @brief On me
 */

#include <memory>

#include <ServerEvent.h>

namespace irccd {

class Server;

namespace event {

/**
 * @class Me
 * @brief On me
 */
class Me final : public ServerEvent {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_origin;
	std::string		m_target;
	std::string		m_message;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param origin the user
	 * @param target the target (maybe you or a channel)
	 * @param message the message
	 */
	Me(std::shared_ptr<Server> server, std::string origin, std::string target, std::string message);

	/**
	 * @copydoc Event::call
	 */
	void call(Plugin &p) const override;

	/**
	 * @copydoc Event::name
	 */
	std::string name(Plugin &p) const override;

	/**
	 * @copydoc ServerEvent::toJson
	 */
	std::string toJson() const override;

	/**
	 * @copydoc ServerEvent::ident
	 */
	std::string ident() const override;
};

} // !event

} // !irccd

#endif // !_IRCCD_ME_H_
