/*
 * ServerState.h -- current server state
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

#ifndef _SERVER_STATE_H_
#define _SERVER_STATE_H_

/**
 * @file ServerState.h
 * @brief Server state
 */

#include <memory>
#include <string>

namespace irccd {

class Server;

/**
 * @class ServerState
 * @brief The server state
 *
 * The server is implemented using a state machine. Each state
 * can switch to any other depending on the context.
 */
class ServerState {
public:
	/**
	 * Default destructor.
	 */
	virtual ~ServerState() = default;

	/**
	 * Execute the current state command.
	 *
	 * @param server the owner
	 */
	virtual void exec(Server &server) = 0;

	/**
	 * Get the current state identity.
	 *
	 * @return the state name
	 */
	virtual std::string which() const = 0;
};

} // !irccd

#endif // !_SERVER_STATE_H_
