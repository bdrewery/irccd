/*
 * Connecting.cpp -- server is connecting
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

#if !defined(_WIN32)
#  include <sys/types.h>
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif

#include <Logger.h>
#include <irccd/Server.h>
#include <irccd/server/Connecting.h>
#include <irccd/server/Running.h>

namespace irccd {

namespace state {

Connecting::Connecting()
{
	Logger::debug("server: switching to state \"Connecting\"");
}

void Connecting::exec(Server &server)
{
	/*
	 * This is needed if irccd is started before DHCP or if
	 * DNS cache is outdated.
	 *
	 * For more information see bug #190
	 */
#if !defined(_WIN32)
	(void)res_init();
#endif

	server.session() = IrcSession();
	server.session().connect(server.shared_from_this());
	server.next<Running>();
}

std::string Connecting::which() const
{
	return "Connecting";
}

} // !state

} // !irccd
