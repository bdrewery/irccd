/*
 * EventMessage.h -- on channel messages (and commands)
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

#ifndef _EVENT_MESSAGE_H_
#define _EVENT_MESSAGE_H_

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

class EventMessage final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_nickname;
	std::string		m_message;

public:
	EventMessage(const std::shared_ptr<Server> &server,
		     const std::string &channel,
		     const std::string &nickname,
		     const std::string &message);

	void call(Plugin &p) override;
};

} // !irccd

#endif // !_EVENT_MESSAGE_H_
