/*
 * ServerConnecting.h -- server is connecting
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

#ifndef _SERVER_CONNECTING_H_
#define _SERVER_CONNECTING_H_

#include "ServerState.h"

namespace irccd {

/**
 * @class ServerConnecting
 * @brief The connecting state
 *
 * This class is used to connect to the IRC server. It just try
 * to resolve the domain and such.
 */
class ServerConnecting : public ServerState {
public:
	ServerConnecting();

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

#endif // !_SERVER_CONNECTING_H_
