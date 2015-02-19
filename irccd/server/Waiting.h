/*
 * Waiting.h -- wait before trying to reconnect
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

#ifndef _WAITING_H_
#define _WAITING_H_

/**
 * @file Waiting.h
 * @brief Wait before reconnecting
 */

#include "ServerState.h"

namespace irccd {

namespace state {

/**
 * @class Waiting
 * @brief Wait before reconnecting
 *
 * Wait a specific amount of time and switch back to connecting state.
 */
class Waiting final : public ServerState {
private:
	bool m_printed = false;

public:
	/**
	 * Default constructor.
	 */
	Waiting();

	void exec(Server &server) override;

	std::string which() const override;
};

} // !state

} // !irccd

#endif // !_WAITING_H_
