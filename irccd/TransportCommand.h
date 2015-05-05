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

/**
 * @file TransportCommand.h
 * @brief Command from transport clients
 */

#include <functional>
#include <memory>
#include <string>

namespace irccd {

class Irccd;
class TransportClientAbstract;

/**
 * @class TransportCommand
 * @brief Regroup all commands
 *
 * This class regroup all commands that are understood under the irccd transport
 * protocol.
 *
 * It contains a std::function to avoid creating lots of classes since it is
 * used to execute only one action.
 */
class TransportCommand {
protected:
	std::string m_ident;
	std::shared_ptr<TransportClientAbstract> m_client;
	std::function<void (Irccd &)> m_command;

public:
	/**
	 * Construct a command with the appropriate function to call.
	 *
	 * @param client the client
	 * @param ident the transport command ident
	 * @param command the command to call
	 */
	inline TransportCommand(std::shared_ptr<TransportClientAbstract> client, std::string ident, std::function<void (Irccd &)> command)
		: m_ident(std::move(ident))
		, m_client(std::move(client))
		, m_command(std::move(command))
	{
	}

	/**
	 * Execute the command.
	 *
	 * @param irccd the irccd instance
	 */
	inline void exec(Irccd &irccd)
	{
		m_command(irccd);
	}

	/**
	 * Provide a ident string for unit tests.
	 *
	 * Command should just concat their name plus all fields
	 * separated by ':'.
	 *
	 * @return the ident
	 */
	inline const std::string &ident() const noexcept
	{
		return m_ident;
	}
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_COMMAND_H_
