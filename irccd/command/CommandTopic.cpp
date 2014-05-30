/*
 * CommandTopic.cpp -- change the topic
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

#include "CommandTopic.h"
#include "Server.h"

namespace irccd {

CommandTopic::CommandTopic(const std::shared_ptr<Server> &server,
			   const std::string &channel,
			   const std::string &topic)
	: Command(server->info().name, channel)
	, m_server(server)
	, m_channel(channel)
	, m_topic(topic)
{
}

bool CommandTopic::call()
{
	return m_server->session().topic(m_channel, tryEncode(m_topic));
}

} // !irccd
