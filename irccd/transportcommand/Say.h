/*
 * Say.h -- say transport command
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

#ifndef _IRCCD_TRANSPORT_COMMAND_SAY_H_
#define _IRCCD_TRANSPORT_COMMAND_SAY_H_

#include "TransportCommand.h"

namespace irccd {

namespace transport {

class Say : public TransportCommand {
private:
	std::string m_server;
	std::string m_target;
	std::string m_message;

public:
	Say(std::shared_ptr<TransportClientAbstract> client, std::string server, std::string target, std::string message);

	void exec(Irccd &) override;

	std::string ident() const override;
};

} // !transport

} // !irccd

#endif // !_IRCCD_TRANSPORT_COMMAND_SAY_H_
