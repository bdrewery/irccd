/*
 * ServerDisconnected.cpp -- server disconnected but not dead
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
	auto &info = server->getInfo();
	auto &reco = server->getRecoInfo();
	auto running = Irccd::getInstance().isRunning();

	Logger::log("server %s: disconnected", info.name.c_str());

	if (running && reco.enabled) {
		if (reco.maxretries <= 0 ||
		    (reco.maxretries >= 1 && ++reco.noretried <= reco.maxretries)) {
			Logger::log("server %s: retrying in %d seconds", info.name.c_str(), reco.timeout);
			System::sleep(reco.timeout);

			return ServerState::Ptr(new ServerConnecting);
		}

		Logger::log("server %s: giving up", info.name.c_str());
	}

	return ServerState::Ptr(new ServerDead);
}

std::string ServerDisconnected::which() const
{
	return "Disconnected";
}

} // !irccd
