/*
 * Running.h -- server running in a forever loop
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

#ifndef _RUNNING_H_
#define _RUNNING_H_

/**
 * @file Running.h
 * @brief Server is running
 */

#include <thread>
#include <memory>
#include <mutex>

#include "ServerState.h"

namespace irccd {

namespace state {

/**
 * @class Running
 * @brief Forever loop
 *
 * This class creates a thread and runs a forever loop until
 * the server disconnect.
 */
class Running final : public ServerState {
public:
	/**
	 * Defautl constructor.
	 */
	Running();

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

#endif // !_RUNNING_H_
