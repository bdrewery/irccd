/*
 * Command.h -- base class for server commands
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

#ifndef _IRCCD_COMMAND_H_
#define _IRCCD_COMMAND_H_

/**
 * @file command/Command.h
 * @brief Base class for server commands
 */

#include <irccd/IO.h>

namespace irccd {

/**
 * @class Command
 * @brief Base class for server command
 */
class Command : public IO {
public:
	/**
	 * Command constructor.
	 *
	 * @param serverName the server name
	 * @param targetName the channel name
	 */
	Command(const std::string &serverName = "", const std::string &targetName = "");

	/**
	 * Try to encode to the server encoding or return the same input.
	 *
	 * @param input the input
	 * @return the converted string or input
	 */
	std::string tryEncode(const std::string &input);

	/**
	 * Call the server command.
	 *
	 * @return true if was sent correctly
	 */
	virtual bool call() = 0;
};

} // !irccd

#endif // !_IRCCD_COMMAND_H_
