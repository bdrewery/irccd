/*
 * EventTopic.h -- on topic changes
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

#ifndef _EVENT_TOPIC_H_
#define _EVENT_TOPIC_H_

#include <memory>

#include "Event.h"

namespace irccd {

class Server;

class EventTopic final : public Event {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_channel;
	std::string		m_who;
	std::string		m_topic;

public:
	EventTopic(const std::shared_ptr<Server> &server,
		   const std::string &channel,
		   const std::string &who,
		   const std::string &topic);

	void call(Plugin &p) override;
	const char *name() const override;
};

} // !irccd

#endif // !_EVENT_TOPIC_H_
