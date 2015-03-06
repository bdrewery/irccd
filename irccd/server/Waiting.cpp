/*
 * Waiting.cpp -- wait before trying to reconnect
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

#include <chrono>

#include <common/Logger.h>

#include <irccd/Irccd.h>
#include <irccd/Server.h>

#include "Connecting.h"
#include "Waiting.h"

using namespace std::literals::chrono_literals;

namespace irccd {

namespace state {

Waiting::Waiting()
{
	Logger::debug("server: switching to state \"Waiting\"");
}

void Waiting::exec(Server &server)
{
	auto &reco = server.reco();
	auto duration = reco.timeout * 1000;

	Logger::log("server %s: disconnected", server.info().name.c_str());

	/*
	 * Loop for the time remaining but do not block for the total timeout
	 * delay in case user wants to close irccd.
	 */
	do {
		/*
		 * While we are waiting, it is possible that user wants to abort
		 * the reconnection.
		 */
		if (!Irccd::instance().isRunning() || reco.stopping) {
			server.next(nullptr);
			return;
		} else {
			if (!m_printed) {
				Logger::log("server %s: retrying in %u seconds", server.info().name.c_str(), server.reco().timeout);
				m_printed = true;
			}

			std::this_thread::sleep_for(50ms);
			duration -= 50;
		}
	} while (duration > 0);

	// Increment total number of tries
	reco.noretried += 1;
	server.next<Connecting>();
}

std::string Waiting::which() const
{
	return "Waiting";
}

} // !state

} // !irccd
