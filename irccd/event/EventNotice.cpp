/*
 * EventNotice.cpp -- on private notices
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

#include <irccd/Plugin.h>

#include "EventNotice.h"

namespace irccd {

EventNotice::EventNotice(const std::shared_ptr<Server> &server,
			 const std::string &who,
			 const std::string &target,
			 const std::string &notice)
	: m_server(server)
	, m_who(who)
	, m_target(target)
	, m_notice(notice)
{
}

void EventNotice::call(Plugin &p)
{
	p.onNotice(m_server, m_who, m_target, m_notice);
}

const char *EventNotice::name() const
{
	return "onNotice";
}

} // !irccd
