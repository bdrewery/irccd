/*
 * ServerCommand.h -- base class for server commands
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

#ifndef _IRCCD_SERVER_COMMAND_H_
#define _IRCCD_SERVER_COMMAND_H_

/**
 * @file ServerCommand.h
 * @brief Base class for server commands
 */

#include <string>

namespace irccd {

class Server;

/**
 * @class ServerCommand
 * @brief Base class for server command
 *
 * This class is used to call the appropriate libircclient function and check
 * if it was successfully enqueued.
 *
 * The libircclient library uses a non-blocking model with fixed size buffers,
 * so we enqueue user commands such as message, query and such into the queue
 * and flush them when possible.
 */
class ServerCommand {
private:
	std::function<void (ServerCommand &)> m_command;

public:
	/**
	 * Construct a server command.
	 *
	 * @pre command must be valid
	 */
	ServerCommand(std::function<bool (ServerCommand &)> command);

	/**
	 * Call the server command.
	 *
	 * @return true if was sent correctly
	 */
	bool call();
};

} // !irccd

#endif // !_IRCCD_SERVER_COMMAND_H_
