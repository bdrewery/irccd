/*
 * Whois.h -- on whois information
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

#ifndef _IRCCD_WHOIS_H_
#define _IRCCD_WHOIS_H_

/**
 * @file event/Whois.h
 * @brief On whois
 */

#include <memory>

#include "Event.h"

namespace irccd {

namespace event {

/**
 * @class Whois
 * @brief On whois
 */
class Whois final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	IrcWhois		m_info;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param info the whois information
	 */
	Whois(std::shared_ptr<Server> server, IrcWhois info);

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

#endif // !_IRCCD_WHOIS_H_
