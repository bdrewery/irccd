/*
 * SocketClient.h -- client socket stream
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

#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_

#include <sys/socket.h>
#include <sys/types.h>

#include <sstream>
#include <string>

#include "Socket.h"

namespace irccd {

class SocketClient : public Socket {
private:
	std::ostringstream m_buffer;

public:
	/**
	 * Add message data to the client.
	 *
	 * @param data the buffer terminated by '\0'
	 */
	void addData(const char *data);

	/**
	 * Tells if the command is finished received entirely, usually
	 * when it ends with '\n'.
	 *
	 * @return true on success
	 */
	bool isFinished(void) const;

	/**
	 * Get the final command without the ending '\n'. This
	 * will also empty the command buffer.
	 *
	 * @return the command string
	 */
	std::string getCommand(void);
};

} // !irccd

#endif // !_SOCKET_CLIENT_H_
