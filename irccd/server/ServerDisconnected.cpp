/*
 * ServerDisconnected.cpp -- server disconnected but not dead
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

#include "Irccd.h"
#include "Logger.h"
#include "Server.h"
#include "ServerDead.h"
#include "ServerConnecting.h"
#include "ServerDisconnected.h"
#include "System.h"

namespace irccd {

ServerDisconnected::ServerDisconnected()
{
	Logger::debug("server: switching to state \"Disconnected\"");
}

ServerState::Ptr ServerDisconnected::exec(Server::Ptr server)
{
	auto &info(server->getInfo());
	auto &reco(server->getRecoInfo());
	int tosleep(reco.timeout);
	bool done(false);
	bool printed(false);

	/*
	 * First, check that we requested to quit or stopping the server.
	 */
	if (!Irccd::getInstance().isRunning() || reco.stopping)
		return ServerState::Ptr(new ServerDead);

	Logger::log("server %s: disconnected", info.name.c_str());

	/*
	 * Do a fake sleep loop. We wait until the timeout goes through 0, we
	 * only sleep 1 second so that we can check that irccd may has been
	 * request to quit or to stop the server. That allows the usage of
	 * irccdctl even if a timeout is of 2 minutes.
	 */
	while (!done && reco.enabled) {
		done = reco.restarting || reco.stopping;

		if (!Irccd::getInstance().isRunning() || tosleep <= 0)
			done = true;
		if (reco.maxretries >= 1 && reco.noretried >= reco.maxretries)
			done = true;

		if (done)
			continue;

		if (!printed) {
			Logger::log("server %s: retrying in %d seconds", info.name.c_str(), reco.timeout);
			printed = true;
		}

		System::sleep(1);
		tosleep = (tosleep - 1 <= 0) ? 0 : tosleep - 1;
	}

	/*
	 * If maxretries is set to 0, we retry for ever.
	 */
	if (reco.restarting || reco.maxretries == 0 ||
	    (reco.enabled && ++reco.noretried <= reco.maxretries))
		return ServerState::Ptr(new ServerConnecting);

	if (reco.enabled)
		Logger::log("server %s: giving up", info.name.c_str());

	return ServerState::Ptr(new ServerDead);
}

std::string ServerDisconnected::which() const
{
	return "Disconnected";
}

} // !irccd
