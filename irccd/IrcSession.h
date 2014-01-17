/*
 * IrcSession.h -- libircclient wrapper
 *
 * Copyright (c) 2013 David Demelier <markand@malikania.fr>
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

#ifndef _IRC_SESSION_H_
#define _IRC_SESSION_H_

#include <memory>

#include <libircclient.h>
#include <libirc_rfcnumeric.h>

namespace irccd {

class Server;

/**
 * @class IrcDeleter
 * @brief Delete the irc_session_t
 */
class IrcDeleter {
public:
	void operator()(irc_session_t *s);
};

/**
 * @class IrcSession
 * @brief Wrapper for irc_session_t
 */
class IrcSession {
private:
	using Ptr	= std::unique_ptr<irc_session_t, IrcDeleter>;

	Ptr m_handle;

public:
	/**
	 * Default constructor.
	 */
	IrcSession();

	/**
	 * Move constructor.
	 *
	 * @param other the other IrcSession
	 */
	IrcSession(IrcSession &&other);

	/**
	 * Move assignment operator.
	 *
	 * @param other the other IrcSession
	 * @return self
	 */
	IrcSession &operator=(IrcSession &&other);

	/**
	 * Cast to irc_session_t for raw commands.
	 *
	 * @return the irc_session_t
	 */
	operator irc_session_t *();
};

} // !irccd

#endif // !_IRC_SESSION_H_
