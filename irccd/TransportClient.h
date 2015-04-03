/*
 * TransportClient.h -- client connected to irccd
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

#ifndef _IRCCD_TRANSPORT_CLIENT_H_
#define _IRCCD_TRANSPORT_CLIENT_H_

/**
 * @file TransportClient.h
 * @brief Client connected to irccd
 */

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "SocketTcp.h"

namespace irccd {

/**
 * @class TransportClient
 * @brief Client connected to irccd
 */
class TransportClientAbstract {
private:
	std::string m_input;
	std::string m_output;
	mutable std::mutex m_mutex;
	std::function<void (const std::string &)> m_onComplete;
	std::function<void ()> m_onDie;
	std::function<void ()> m_onWrite;

	void receive();
	void send();

public:
	/**
	 * Virtual destructor defaulted.
	 */
	virtual ~TransportClientAbstract() = default;

	/**
	 * Set the onComplete callback. The callback will be called for each
	 * message received that is complete.
	 *
	 * @param func the function to move
	 */
	void setOnComplete(std::function<void (const std::string &)> func);

	/**
	 * Set the onWrite callback. The callback will be called when some
	 * output has been queued.
	 *
	 * @param func the funciton to move
	 */
	void setOnWrite(std::function<void ()> func);

	/**
	 * Set the onDie callback. The callback will be called when the
	 * client is marked disconnected.
	 *
	 * @param func the function to move
	 * @note It is still safe to use the object as it is a shared_ptr
	 */
	void setOnDie(std::function<void ()> func);

	/**
	 * Flush pending data to send and try to receive if possible.
	 *
	 * @note This function is called from the TransportManager thread and should not be used somewhere else
	 */
	void process(int direction);

	/**
	 * Send some data, it will be pushed to the outgoing buffer.
	 *
	 * This function appends "\r\n\r\n" after the message so you don't have
	 * to do it manually.
	 *
	 * @note Thread-safe
	 * @param message the message
	 */
	void send(std::string message);

	/**
	 * Tell if the client has data pending for output.
	 *
	 * @note Thread-safe
	 * @return true if has pending data to write
	 */
	bool hasOutput() const noexcept;

	/**
	 * Get the underlying socket as abstract.
	 *
	 * @return the abstract socket
	 */
	virtual SocketAbstractTcp &socket() noexcept = 0;
};

/**
 * @class TransportClient
 * @brief Template class for Tcp and Ssl sockets
 */
template <typename Sock>
class TransportClient : public TransportClientAbstract {
private:
	Sock m_socket;

public:
	/**
	 * Create a client.
	 *
	 * @param sock the socket
	 */
	inline TransportClient(Sock socket)
		: m_socket(std::move(socket))
	{
	}

	/**
	 * @copydoc TransportClientAbstract::socket
	 */
	SocketAbstractTcp &socket() noexcept override
	{
		return m_socket;
	}
};

} // !irccd

#endif // !_IRCCD_TRANSPORT_CLIENT_H_
