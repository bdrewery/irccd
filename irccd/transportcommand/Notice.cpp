/*
 * Notice.cpp -- notice transport command
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

#include "Notice.h"

namespace irccd {

namespace transport {

Notice::Notice(std::shared_ptr<TransportClientAbstract> client, std::string server, std::string target, std::string message)
	: TransportCommand(std::move(client))
	, m_server(std::move(server))
	, m_target(std::move(target))
	, m_message(std::move(message))
{
}

void Notice::exec(Irccd &)
{
}

std::string Notice::ident() const
{
	return "notice:" + m_server + ":" + m_target + ":" + m_message;
}

} // !transport

} // !irccd
