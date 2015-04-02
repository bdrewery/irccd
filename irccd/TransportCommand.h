/*
 * TransportCommand.h -- transport command queue'ed to the main loop
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

#ifndef _IRCCD_TRANSPORT_COMMAND_H_
#define _IRCCD_TRANSPORT_COMMAND_H_

#include <memory>
#include <string>

namespace irccd {

class Irccd;
class TransportClientAbstract;

class TransportCommand {
protected:
	std::shared_ptr<TransportClientAbstract> m_client;

public:
	inline TransportCommand(std::shared_ptr<TransportClientAbstract> client)
		: m_client(client)
	{
	}

	virtual ~TransportCommand() = default;

	virtual void exec(Irccd &) = 0;

	/**
	 * Provide a ident string for unit tests.
	 *
	 * Derived classes should just concat their name plus all fields
	 * separated by ':'.
	 *
	 * @return the ident
	 */
	virtual std::string ident() const = 0;
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_COMMAND_H_
