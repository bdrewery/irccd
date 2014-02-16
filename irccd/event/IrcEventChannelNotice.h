/*
 * IrcEventChannelNotice.h -- on channel notice event
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

#ifndef _IRC_EVENT_CHANNEL_NOTICE_H_
#define _IRC_EVENT_CHANNEL_NOTICE_H_

#include <string>

#include "IrcEvent.h"
#include "Server.h"

namespace irccd {

class IrcEventChannelNotice : public IrcEvent {
private:
	Server::Ptr	m_server;
	std::string	m_channel;
	std::string	m_who;
	std::string	m_notice;

public:
	/**
	 * @param server the server
	 * @param channel the channel
	 * @param who the originator
	 * @param notice the notice message
	 */
	IrcEventChannelNotice(Server::Ptr server,
			      const std::string &channel,
			      const std::string &who,
			      const std::string &notice);

	/**
	 * @copydoc IrcEvent::action
	 */
	virtual void action(lua_State *L) const;
};

} // !irccd

#endif // !_IRC_EVENT_CHANNEL_NOTICE_H_
