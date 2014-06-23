/*
 * EventInvite.h -- on channel invites
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

#ifndef _EVENT_INVITE_H_
#define _EVENT_INVITE_H_

/**
 * @file EventInvite.h
 * @brief On invite
 */

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

/**
 * @class EventInvite
 * @brief On invite
 */
class EventInvite final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_who;

public:
	/**
	 * Event constructor.
	 *
	 * @param server the server
	 * @param channel the channel
	 * @param who the user who invited you
	 */
	EventInvite(const std::shared_ptr<Server> &server,
		    const std::string &channel,
		    const std::string &who);

	/**
	 * @copydoc Event::call
	 */
	void call(Plugin &p) override;

	/**
	 * @copydoc Event::name
	 */
	const char *name() const override;
};

} // !irccd

#endif // !_EVENT_INVITE_H_
