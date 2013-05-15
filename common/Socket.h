/*
 * Socket.h -- base abstract socket class
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

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <string>
#include <exception>

namespace irccd {

/**
 * Socket class, works only in TCP mode.
 */
class Socket {
public:
	enum InetFamily {
		Inet4	= (1 << 0),
		Inet6	= (1 << 1)
	};

	class Exception : public std::exception {
	private:
		std::string m_error;
		bool m_isDisconnected;

	public:
		Exception(const std::string &message, bool isDisconneted = false)
		{
			m_error = message;
			m_isDisconnected = isDisconneted;
		}

		~Exception(void)
		{
		}

		bool disconnected(void) const {
			return m_isDisconnected;
		}

		virtual const char * what(void) const throw() {
			return m_error.c_str();
		}
	};

protected:
	int m_sock;
	int m_family;
	std::string m_error;

public:

	Socket(void);
	virtual ~Socket(void);

	int getSock(void) const;

	void setSock(int sock);

	void setFamily(int family);

	const std::string & getErrorMessage(void) const ;

	/**
	 * Create the socket
	 *
	 * @return true on success
	 */
	bool create(void);

	/**
	 * Send a message to the socket.
	 *
	 * @param data the data pointer
	 * @param dataLength the data length
	 * @return the number of bytes sent
	 * @throw Exception on a problem
	 */
	unsigned send(const void *data, unsigned dataLength);

	/**
	 * Receive a message.
	 *
	 * @param data where to store the data
	 * @param dataLength buffer size
	 * @return the number of byte received
	 * @throw Exception if a problem appears
	 */
	unsigned receive(void *data, unsigned dataLength);

	/**
	 * Set an error message and return false for convenient.
	 *
	 * @param message the message
	 * @return false
	 */
	bool setErrorMessage(const std::string &message)
	{
		m_error = message;
		return false;
	}

	/**
	 * Close the socket.
	 */
	void close(void);
};

} // !irccd

#endif // !_SOCKET_H_
