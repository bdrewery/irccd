/*
 * Listener.h -- listener for irccdctl clients
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

#ifndef _IRCCD_LISTENER_H_
#define _IRCCD_LISTENER_H_

/**
 * @file Listener.h
 * @brief Listener for irccdctl
 */

#include <map>
#include <string>

#include <Singleton.h>
#include <Socket.h>
#include <SocketAddress.h>
#include <SocketListener.h>

#include "Message.h"

namespace irccd {

/**
 * @class Listener
 * @brief Listen for irccd clients
 *
 * This class is used to manage irccd via sockets or irccdctl. It supports
 * Unix and Internet sockets.
 *
 * Listeners are currently not thread safe and should only be used
 * in the main thread.
 */
class Listener final : public Singleton<Listener> {
private:
	friend class Singleton<Listener>;

	using Servers		= std::vector<Socket>;
	using Clients		= std::map<Socket, Message>;

	SocketListener m_listener;
	Servers m_servers;
	Clients m_clients;

	void clientAdd(Socket &client);
	void clientRead(Socket &client);
	void execute(const std::string &cmd, Socket &s);
	void notifySocket(const std::string &message, Socket &s);

public:
	/**
	 * Add a new listener for irccd client processing.
	 *
	 * @param s the server socket
	 */
	void add(Socket s);

	/**
	 * Get the number of listeners enabled.
	 *
	 * @return the number
	 */
	int count();

	/**
	 * Process clients: accept them, execute and respond.
	 */
	void process();

	/**
	 * Close all listeners.
	 */
	void close();
};

} // !irccd

#endif // !_IRCCD_LISTENER_H_
