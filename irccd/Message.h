/*
 * Message.h -- message handler for clients
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

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <sstream>
#include <string>

/**
 * @class Message
 * @brief Message handler for clients
 *
 * This class handles incoming data from clients and tells when a data
 * is finished.
 */
class Message {
private:
	std::ostringstream m_data;

public:
	/**
	 * Default constructor.
	 */
	Message() = default;

	/**
	 * Copy constructor.
	 *
	 * @param m the old message
	 */
	Message(const Message &m);

	/**
	 * Tell if the client message has finished. A client message
	 * ends with '\n'.
	 *
	 * @param msg the received data
	 * @param command the final command (if finished)
	 * @return true if finished
	 */
	bool isFinished(const std::string &msg, std::string &command);

	/**
	 * Copy assignment.
	 *
	 * @param m the message
	 * @return the message
	 */
	Message &operator=(const Message &m);
};

#endif // !_MESSAGE_H_
