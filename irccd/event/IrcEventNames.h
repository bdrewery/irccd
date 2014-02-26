/*
 * IrcEventNames.h -- on channel names listing
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

#ifndef _IRC_EVENT_NAMES_H_
#define _IRC_EVENT_NAMES_H_

#include <string>
#include <vector>

#include "IrcEvent.h"
#include "Server.h"

namespace irccd {

class IrcEventNames : public IrcEvent {
public:
	using List	= std::vector<std::string>;

private:
	Server::Ptr	m_server;
	List		m_list;
	std::string	m_channel;

public:
	IrcEventNames(Server::Ptr server,
		      const List &list,
		      const std::string &channel);

	/**
	 * @copydoc IrcEvent::action
	 */
	virtual void action(lua_State *L) const;
};

} // !irccd

#endif // !_IRC_EVENT_NAMES_H_
