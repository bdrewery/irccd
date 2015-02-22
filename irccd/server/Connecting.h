/*
 * Connecting.h -- server is connecting
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

#ifndef _IRCCD_CONNECTING_H_
#define _IRCCD_CONNECTING_H_

/**
 * @file Connecting.h
 * @brief Server is connecting
 */

#include "ServerState.h"

namespace irccd {

namespace state {

/**
 * @class Connecting
 * @brief The connecting state
 *
 * This class is used to connect to the IRC server. It just try
 * to resolve the domain and such.
 */
class Connecting final : public ServerState {
public:
	/**
	 * Default constructor.
	 */
	Connecting();

	/**
	 * @copydoc ServerState::exec
	 */
	void exec(Server &server) override;

	/**
	 * @copydoc ServerState::which
	 */
	std::string which() const override;
};

} // !state

} // !irccd

#endif // !_IRCCD_CONNECTING_H_
