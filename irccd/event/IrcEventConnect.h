/*
 * IrcEventConnect.h -- on successful connection
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#ifndef _IRC_EVENT_CONNECT_H_
#define _IRC_EVENT_CONNECT_H_

#include "IrcEvent.h"
#include "Server.h"

namespace irccd {

class IrcEventConnect : public IrcEvent {
private:
	Server::Ptr m_server;

public:
	IrcEventConnect(Server::Ptr server);

	/**
	 * @copydoc IrcEvent::action
	 */
	virtual void action(lua_State *L) const;
};

};

#endif // !_IRC_EVENT_CONNECT_H_
