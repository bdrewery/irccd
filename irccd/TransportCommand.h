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

#include <functional>
#include <memory>
#include <string>

namespace irccd {

class Irccd;
class TransportClientAbstract;

class TransportCommand {
protected:
	std::string m_ident;
	std::shared_ptr<TransportClientAbstract> m_client;
	std::function<void (TransportCommand &, Irccd &)> m_command;

public:
	void cnotice(const std::string &server, const std::string &channel, const std::string &message);
	void connect(/* TODO */);
	void disconnect(const std::string &name);
	void invite(const std::string &server, const std::string &target, const std::string &channel);
	void join(const std::string &server, const std::string &channel, const std::string &password);
	void kick(const std::string &server, const std::string &target, const std::string &channel, const std::string &reason);
	void load(const std::string &path, bool isrelative);
	void me(const std::string &server, const std::string &channel, const std::string &message);
	void mode(const std::string &server, const std::string &channel, const std::string &mode);
	void nick(const std::string &server, const std::string &nickname);
	void notice(const std::string &server, const std::string &target, const std::string &message);
	void part(const std::string &server, const std::string &channel, const std::string &reason);
	void reconnect(const std::string &server);
	void reload(const std::string &plugin);
	void topic(const std::string &server, const std::string &channel, const std::string &topic);
	void unload(const std::string &plugin);
	void umode(const std::string &server, const std::string &mode);

	inline TransportCommand(std::string ident,
				std::shared_ptr<TransportClientAbstract> client,
				std::function<void (TransportCommand &, Irccd &)> command)
		: m_ident(std::move(ident))
		, m_client(std::move(client))
		, m_command(std::move(command))
	{
	}

	inline void exec(Irccd &irccd)
	{
		m_command(*this, irccd);
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
