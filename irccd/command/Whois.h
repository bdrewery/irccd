/*
 * Whois.h -- get whois information
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

#ifndef _WHOIS_H_
#define _WHOIS_H_

/**
 * @file command/Whois.h
 * @brief Get whois
 */

#include <memory>

#include "Command.h"

namespace irccd {

class Server;

namespace command {

/**
 * @class Whois
 * @brief Get whois
 */
class Whois final : public Command {
private:
	std::shared_ptr<Server>	m_server;
	std::string		m_target;

public:
	/**
	 * Whois command constructor.
	 *
	 * @param server the server
	 * @param target the target
	 */
	Whois(std::shared_ptr<Server> server, std::string target);

	/**
	 * @copydoc Command::call
	 */
	bool call() override;
};

} // !command

} // !irccd

#endif // !_WHOIS_H_
