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

#include "Connecting.h"
#include "Disconnected.h"
#include "Server.h"

namespace irccd {

namespace state {

Connecting::Connecting()
{
	Logger::debug("server: switching to state \"Connecting\"");
}

Connecting::~Connecting()
{
}

bool Connecting::connect(Server &server)
{
	const ServerInfo &info = server.info();
	const Identity &identity = server.identity();
	const char *password = info.password.empty() ? nullptr : info.password.c_str();
	std::string host = info.host;
	int code;

	// libircclient requires # for SSL connection
	if (info.ssl) {
		host.insert(0, 1, '#');
	}

	if (info.ipv6) {
		code = irc_connect6(server.session(), host.c_str(), info.port, password,
				    identity.nickname().c_str(),
				    identity.username().c_str(),
				    identity.realname().c_str());
	} else {
		code = irc_connect(server.session(), host.c_str(), info.port, password,
				   identity.nickname().c_str(),
				   identity.username().c_str(),
				   identity.realname().c_str());
	}

	return code == 0;
}

void Connecting::prepare(Server &server, fd_set &setinput, fd_set &setoutput, int &maxfd)
{
	/*
	 * The connect function will either fail if the hostname wasn't resolved
	 * or if any of the internal functions fail.
	 *
	 * It returns success if the connection was successful but it does not
	 * mean that connection is established.
	 *
	 * Because this function will be called repeatidly from the
	 * ServerManager, if the connection was started and we're still not
	 * connected in the specified timeout time, we mark the server
	 * as disconnected.
	 *
	 * Otherwise, the libircclient event_connect will change the state.
	 */
	const ServerInfo &info = server.info();

	if (m_started) {
		const ServerSettings &settings = server.settings();

		if (m_timer.elapsed() > static_cast<unsigned>(settings.recotimeout * 1000)) {
			Logger::warn("server %s: timeout while connecting", info.name.c_str());
			server.next<state::Disconnected>();
		} else if (!irc_is_connected(server.session())) {
			Logger::warn("server %s: error while connecting: %s",
				     info.name.c_str(), irc_strerror(irc_errno(server.session())));

			if (settings.recotimeout > 0) {
				Logger::warn("server %s: retrying in %d seconds", info.name.c_str(), settings.recotimeout);
			}

			server.next<state::Disconnected>();
		} else {
			irc_add_select_descriptors(server.session(), &setinput, &setoutput, &maxfd);
		}
	} else {
		/*
		 * This is needed if irccd is started before DHCP or if
		 * DNS cache is outdated.
		 *
		 * For more information see bug #190.
		 */
#if !defined(_WIN32)
		(void)res_init();
#endif
		Logger::log("server %s: trying to connect to %s, port %d",
			    info.name.c_str(), info.host.c_str(), info.port);

		if (!connect(server)) {
			Logger::warn("server %s: disconnected while connecting: %s",
				     info.name.c_str(), irc_strerror(irc_errno(server.session())));
			server.next<state::Disconnected>();
		} else {
			m_started = true;
		}
	}
}

int Connecting::state() const noexcept
{
	return ServerState::Connecting;
}

} // !state

} // !irccd
