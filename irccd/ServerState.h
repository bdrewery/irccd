/*
 * ServerState.h -- server current state
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

#ifndef _IRCCD_SERVER_STATE_H_
#define _IRCCD_SERVER_STATE_H_

#include <ElapsedTimer.h>
#include <IrccdConfig.h>

#if defined(IRCCD_SYSTEM_WINDOWS)
#  include <Winsock2.h>
#else
#  include <sys/types.h>
#  include <sys/select.h>
#endif

namespace irccd {

class Server;

/**
 * @class ServerState
 * @brief State machine for servers
 *
 * The servers use a state machine pattern for executing the socket selection
 * differently.
 *
 * The following states are used:
 *
 *         |<---------------------|
 *         |                      |
 *         v                      |
 * +---------------+      +---------------+      +---------------+
 * | Connecting    |----->| Disconnected  |----->| Dead          |
 * +---------------+      +---------------+      +---------------+
 *         |                      ^
 *         |                      |
 *         v                      |
 * +---------------+              |
 * | Connected     |------------->|
 * +---------------+
 *
 * The states are very basic and small so we implement them in a functional
 * basis using a type and a switch statement.
 *
 * The Connecting state
 * --------------------
 *
 * The server is not connected to the IRC server, it just try to resolve
 * the hostname and connect, it does not mean that the connection is
 * established.
 *
 * The Connected state
 * -------------------
 *
 * The server connection is complete and can now send and receive data.
 *
 * The Disconnected state
 * ----------------------
 *
 * The server has been disconnected by a network failure or a server shutdown.
 * This state will track the elapsed time until the user specified time has
 * been elapsed to try a reconnection.
 *
 * If reconnection is completely disabled, this state switch immediately to
 * Dead. Otherwise, it will switch to connecting again.
 *
 * Also, if the number of reconnection has failed too, this state switch to
 * Dead again.
 *
 * The Dead state
 * --------------
 *
 * The server is completely inactive and removed from the ServerManager, it is
 * not destroyed as it can be used somewhere else but any of its function
 * will be ineffective.
 */
class ServerState {
public:
	enum Type {
		Undefined,
		Connecting,
		Connected,
		Disconnected,
		Dead
	};

private:
	Type m_type;

	/* For ServerState::Connecting */
	bool m_started{false};
	ElapsedTimer m_timer;

	/* Private helpers */
	bool connect(Server &server);

	/* Different preparation */
	void prepareConnected(Server &, fd_set &setinput, fd_set &setoutput, int &maxfd);
	void prepareConnecting(Server &, fd_set &setinput, fd_set &setoutput, int &maxfd);
	void prepareDead(Server &, fd_set &setinput, fd_set &setoutput, int &maxfd);
	void prepareDisconnected(Server &, fd_set &setinput, fd_set &setoutput, int &maxfd);

public:
	ServerState(Type type);

	void prepare(Server &server, fd_set &setinput, fd_set &setoutput, int &maxfd);

	inline Type type() const noexcept
	{
		return m_type;
	}
};

} // !irccd

#endif // !_IRCCD_SERVER_STATE_H_

