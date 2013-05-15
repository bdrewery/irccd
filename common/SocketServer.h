/*
 * SocketServer.h -- base class for server sockets
 *
 * Copyright (c) 2011, 2012, 2013 David Demelier <markand@malikania.fr>
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

#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#include "Socket.h"
#include "SocketClient.h"

namespace irccd {

class SocketServer : public Socket {
public:
	/**
	 * Bind the socket server.
	 *
	 * @return true on success
	 */
	virtual bool bind(void) = 0;	

	/**
	 * Listen to specific number of pending connections.
	 *
	 * @param max the max number
	 * @return true on success
	 */
	bool listen(int max);

	/**
	 * Accept a client
	 */
	virtual SocketClient * accept(void) = 0;
};

} // !irccd

#endif // !_SOCKET_SERVER_H_
