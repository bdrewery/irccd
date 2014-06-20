/*
 * ServerUninitialized.h -- server created but not started
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

#ifndef _SERVER_UNINITIALIZED_H_
#define _SERVER_UNINITIALIZED_H_

/**
 * @file ServerUninitialized.h
 * @brief Server is currently not initialized
 */

#include "ServerState.h"

namespace irccd {

/**
 * @class ServerUninitialized
 * @brief Default server state
 *
 * This state is used when the server is allocated. It simply
 * switch to the connecting state after being added to the
 * server registry.
 */
class ServerUninitialized : public ServerState {
public:
	ServerUninitialized();

	/**
	 * @copydoc ServerState::exec
	 */
	virtual Ptr exec(Server::Ptr server);

	/**
	 * @copydoc ServerState::which
	 */
	virtual std::string which() const;
};

} // !irccd

#endif // !_SERVER_UNINITIALIZED_H_
