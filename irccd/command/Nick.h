/*
 * Nick.h -- change your nickname
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

#ifndef _NICK_H_
#define _NICK_H_

/**
 * @file command/Nick.h
 * @brief Change the nickname
 */

#include <memory>

#include "Command.h"

namespace irccd {

class Server;

namespace command {

/**
 * @class Nick
 * @brief Change the nickname
 */
class Nick final : public Command {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_nick;

public:
	/**
	 * Nickname change command.
	 *
	 * @param server the server
	 * @param nick the new nickname
	 */
	Nick(std::shared_ptr<Server> server, std::string nick);

	/**
	 * @copydoc Command::call
	 */
	bool call() override;
};

} // !command

} // !irccd

#endif // !_NICK_H_
