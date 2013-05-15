/*
 * SocketInet.h -- internet domain socket
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

#ifndef _SOCKET_INET_H_
#define _SOCKET_INET_H_

#include <cstdint>
#include <string>

#include "SocketServer.h"

namespace irccd {

class SocketServerInet : public SocketServer {
private:
	std::string m_host;
	unsigned short m_port;
	int m_inet;

	bool bind6(void);
	bool bind4(void);

	bool connect6(void);
	bool connect4(void);

public:
	SocketServerInet(void);
	~SocketServerInet(void);

	/**
	 * Constructor with some parameters.
	 *
	 * @param host the hostname or "*" for any
	 * @param port the port
	 * @param family OR'ed family allowed
	 * @see InetFamily
	 */
	SocketServerInet(const std::string &host, uint16_t port, int inet = Inet6 | Inet4);

	virtual bool bind(void);

	virtual SocketClient * accept(void);
};

} // !irccd

#endif // !_SOCKET_INET_H_
