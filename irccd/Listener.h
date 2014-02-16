/*
 * Listener.h -- listener for irccdctl clients
 *
 * Copyright (c) 2013, 2014 David Demelier <markand@malikania.fr>
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

#ifndef _LISTENER_H_
#define _LISTENER_H_

#include <map>
#include <string>

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
class Listener {
private:
	using MasterSockets	= std::vector<Socket>;
	using StreamClients	= std::map<Socket, Message>;
	using DatagramClients	= std::map<SocketAddress, Message>;

	// List of listening sockets and the listener
	static MasterSockets m_socketServers;			//! socket servers
	static SocketListener m_listener;			//! socket listener

	// Clients (both TCP and UDP)
	static StreamClients m_streamClients;			//! tcp based clients
	static DatagramClients m_dgramClients;			//! udp based "clients"

	static void clientAdd(Socket &client);
	static void clientRead(Socket &client);
	static void peerRead(Socket &s);
	static void execute(const std::string &cmd,
			    Socket s,
			    const SocketAddress &info = SocketAddress());
	static void notifySocket(const std::string &message,
				 Socket s,
				 const SocketAddress &info);

public:
	/**
	 * Add a new listener for irccd client processing.
	 *
	 * @param s the server socket
	 */
	static void add(Socket s);

	/**
	 * Get the number of listeners enabled.
	 *
	 * @return the number
	 */
	static int count();

	/**
	 * Process clients: accept them, execute and respond.
	 */
	static void process();

	/**
	 * Close all listeners.
	 */
	static void close();
};

} // !irccd

#endif // !_LISTENER_H_
