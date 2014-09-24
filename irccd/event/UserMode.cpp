/*
 * UserMode.cpp -- on user mode change
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

#include "UserMode.h"

namespace irccd {

namespace event {

UserMode::UserMode(std::shared_ptr<Server> server, std::string nickname, std::string mode)
	: m_server(std::move(server))
	, m_nickname(std::move(nickname))
	, m_mode(std::move(mode))
{
}

void UserMode::call(Plugin &p)
{
	p.onUserMode(m_server, m_nickname, m_mode);
}

const char *UserMode::name() const
{
	return "onUserMode";
}

} // !event

} // !irccd
