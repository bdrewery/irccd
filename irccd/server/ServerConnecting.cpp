/*
 * ServerConnecting.cpp -- server is connecting
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

#if !defined(_WIN32)
#  include <sys/types.h>
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif

#include "Logger.h"
#include "Server.h"
#include "ServerConnecting.h"
#include "ServerRunning.h"

namespace irccd {

ServerConnecting::ServerConnecting()
{
	Logger::debug("server: switching to state \"Connecting\"");
}

ServerState::Ptr ServerConnecting::exec(Server::Ptr server)
{
	auto &session = server->getSession();
	session = IrcSession();

	/*
	 * This is needed if irccd is started before DHCP or if
	 * DNS cache is outdated.
	 *
	 * For more information see #190
	 */
#if !defined(_WIN32)
	(void)res_init();
#endif

	unsigned int major, minor;

	irc_set_ctx(session, new Server::Ptr(server));
	irc_get_version(&major, &minor);

	auto &info = server->getInfo();
	auto &identity = server->getIdentity();

	/*
	 * After some discuss with George, SSL has been fixed in newer version
	 * of libircclient. > 1.6 is needed for SSL.
	 */
	if (major >= 1 && minor > 6) {
		// SSL needs to add # front of host
		if (info.ssl)
			info.host.insert(0, 1, '#');

		if (!info.sslVerify)
			irc_option_set(session, LIBIRC_OPTION_SSL_NO_VERIFY);
	} else {
		if (info.ssl)
			Logger::log("server %s: SSL is only supported with libircclient > 1.6",
			    info.name.c_str());
	}

	const char *password = nullptr;

	if (info.password.length() > 0)
		password = info.password.c_str();

	auto res = irc_connect(
	    session,
	    info.host.c_str(),
	    info.port,
	    password,
	    identity.nickname.c_str(),
	    identity.username.c_str(),
	    identity.realname.c_str());

	if (res == 0)
		server->getRecoInfo().noretried = 0;

	return ServerState::Ptr(new ServerRunning);
}

std::string ServerConnecting::which() const
{
	return "Connecting";
}

} // !irccd